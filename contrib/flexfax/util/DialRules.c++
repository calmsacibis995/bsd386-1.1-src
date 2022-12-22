/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/DialRules.c++,v 1.1.1.1 1994/01/14 23:10:47 torek Exp $
/*
 * Copyright (c) 1993 Sam Leffler
 * Copyright (c) 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * FlexFAX Dialing String Rule Support.
 */
#include "DialRules.h"
#include "RegExArray.h"
#include "Dictionary.h"

#include <InterViews/regexp.h>
#include <ctype.h>

/*
 * Interpolation points in replacement strings have
 * the META bit or'd together with their match identifier.
 * That is ``\1'' is META|1, ``\2'' is META|2, etc.
 * The ``&'' interpolation is treated as ``\0''.
 */
const u_int META = 0200;		// interpolation marker

struct DialRule {
    RegExPtr	pat;			// pattern to match
    fxStr	replace;		// replacement string

    DialRule();
    ~DialRule();

    int compare(const DialRule *) const;
};
DialRule::DialRule(){}
DialRule::~DialRule(){}
int DialRule::compare(const DialRule*) const { return 0; }

fxDECLARE_ObjArray(RuleArray, DialRule);
fxIMPLEMENT_ObjArray(RuleArray, DialRule);
fxDECLARE_StrKeyDictionary(VarDict, fxStr);
fxIMPLEMENT_StrKeyObjValueDictionary(VarDict, fxStr);
fxDECLARE_StrKeyDictionary(RulesDict, RuleArrayPtr);
fxIMPLEMENT_StrKeyObjValueDictionary(RulesDict, RuleArrayPtr);

DialStringRules::DialStringRules(const char* file) : filename(file)
{
    verbose = FALSE;
    fp = NULL;
    vars = new VarDict;
    regex = new RegExArray;
    rules = new RulesDict;
}

DialStringRules::~DialStringRules()
{
    delete rules;
    delete regex;
    delete vars;
}

void DialStringRules::setVerbose(fxBool b) { verbose = b; }

/*
 * Parse the dialing string rules file
 * to create the in-memory rules.
 */
fxBool
DialStringRules::parse()
{
    fxBool ok = FALSE;
    fp = fopen(filename, "r");
    if (fp) {
	ok = parseRules();
	fclose(fp);
    }
    return (ok);
}

void
DialStringRules::def(const fxStr& var, const fxStr& value)
{
    if (verbose)
	printf("Define %s = \"%s\"\n", (char*) var, (char*) value);
    (*vars)[var] = value;
}

void
DialStringRules::undef(const fxStr& var)
{
    if (verbose)
	printf("Undefine %s\n", (char*) var);
    vars->remove(var);
}

fxBool
DialStringRules::parseRules()
{
    lineno = 0;
    char line[1024];
    char* cp;
    while (cp = nextLine(line, sizeof (line))) {
	// collect token
	if (!isalpha(*cp)) {
	    parseError("Syntax error, expecting identifier");
	    return (FALSE);
	}
	const char* tp = cp;
	for (cp++; isalnum(*cp); cp++)
	    ;
	fxStr var(tp, cp-tp);
	while (isspace(*cp))
	    cp++;
	if (*cp == ':' && cp[1] == '=') {	// rule set definition
	    for (cp += 2; *cp != '['; cp++)
		if (*cp == '\0') {
		    parseError("Missing '[' while parsing rule set");
		    return (FALSE);
		}
	    if (verbose)
		printf("%s := [\n", (char*) var);
	    RuleArray* ra = new RuleArray;
	    if (!parseRuleSet(*ra)) {
		delete ra;
		return (FALSE);
	    }
	    (*rules)[var] = ra;
	    if (verbose)
		printf("]\n");
	} else if (*cp == '=') {		// variable definition
	    fxStr value;
	    if (parseToken(cp+1, value) == NULL)
		return (FALSE);
	    def(var, value);
	} else {				// an error
	    parseError("Missing '=' or ':=' after \"%s\"", (char*) var);
	    return (FALSE);
	}
    }
    if (verbose) {
	RuleArray* ra = (*rules)["CanonicalNumber"];
	if (ra == 0)
	    fprintf(stderr, "Warning, no \"CanonicalNumber\" rules.\n");
	ra = (*rules)["DialString"];
	if (ra == 0)
	    fprintf(stderr, "Warning, no \"DialString\" rules.\n");
    }
    return (TRUE);
}

/*
 * Locate the next non-blank line in the file
 * and return a pointer to the first non-whitespace
 * character on the line.  Leading white space and
 * comments are stripped.  NULL is returned on EOF.
 */
char*
DialStringRules::nextLine(char* line, int lineSize)
{
    char* cp;
    do {
	if (!fgets(line, lineSize, fp))
	    return (NULL);
	lineno++;
	cp = strchr(line, '!');
	if (cp)
	    *cp = '\0';
	else if (cp = strchr(line, '\n'))
	    *cp = '\0';
	for (cp = line; isspace(*cp); cp++)
	    ;
    } while (*cp == '\0');
    return (cp);
}

/*
 * Parse the next token on the input line and return
 * the result in the string v.  Variable references
 * are interpolated here, though they might be better
 * done at a later time.
 */
const char*
DialStringRules::parseToken(const char* cp, fxStr& v)
{
    while (isspace(*cp))
	cp++;
    const char* tp;
    if (*cp == '"') {			// "..."
	tp = ++cp;
	for (;;) {
	    if (*cp == '\0') {
		parseError("String with unmatched '\"'");
		return (NULL);
	    }
	    if (*cp == '\\' && cp[1] == '\0') {
		parseError("Bad '\\' escape sequence");
		return (NULL);
	    }
	    if (*cp == '"' && (cp == tp || cp[-1] != '\\'))
		break;
	    cp++;
	}
	v = fxStr(tp, cp-tp);
	cp++;				// skip trailing ``"''
    } else {				// token terminated by white space
	for (tp = cp; *cp != '\0'; cp++) {
	    if (*cp == '\\' && cp[1] == '\0') {
		parseError("Bad '\\' escape sequence");
		return (NULL);
	    }
	    if (isspace(*cp) && (cp == tp || cp[-1] != '\\'))
		break;
	}
	v = fxStr(tp, cp-tp);
    }
    for (u_int i = 0, n = v.length(); i < n; i++) {
	if (v[i] == '$' && (i+1<n && v[i+1] == '{')) {
	    /*
	     * Handle variable reference.
	     */
	    u_int l = v.next(i, '}');
	    if (l >= v.length()) {
		parseError("Missing '}' for variable reference");
		return (FALSE);
	    }
	    fxStr var = v.cut(i+2,l-(i+2));// variable name
	    v.remove(i, 3);		// remove ${}
	    const fxStr& value = (*vars)[var];
	    v.insert(value, i);		// insert variable's value
	    n = v.length();		// adjust string length
	    i += value.length()-1;	// don't scan substituted string
	} else if (v[i] == '\\')	// pass \<char> escapes unchanged
	    i++;
    }
    return cp;
}

/*
 * Handle \escapes for a token
 * that appears on the RHS of a rewriting rule.
 */
void
DialStringRules::subRHS(fxStr& v)
{
    /*
     * Replace `&' and \n items with (META|<n>) to mark spots
     * in the token where matches should be interpolated.
     */
    for (u_int i = 0, n = v.length(); i < n; i++) {
	if (v[i] == '\\') {		// process \<char> escapes
	    v.remove(i), n--;
	    if (isdigit(v[i]))
		v[i] = META | (v[i] - '0');
	} else if (v[i] == '&')
	    v[i] = META | 0;
    }
}

/*
 * Parse a set of rules and construct the array of
 * rules (regular expressions + replacement strings).
 */
fxBool
DialStringRules::parseRuleSet(RuleArray& rules)
{
    for (;;) {
	char line[1024];
	const char* cp = nextLine(line, sizeof (line));
	if (!cp) {
	    parseError("Missing ']' while parsing rule set");
	    return (FALSE);
	}
	if (*cp == ']')
	    return (TRUE);
	// new rule
	fxStr pat;
	if ((cp = parseToken(cp, pat)) == NULL)
	    return (FALSE);
	while (isspace(*cp))
	    cp++;
	if (*cp != '=') {
	    parseError("Rule pattern without '='");
	    return (FALSE);
	}
	DialRule r;
	if (parseToken(cp+1, r.replace) == NULL)
	    return (FALSE);
	if (verbose)
	    printf("  \"%s\" = \"%s\"\n",
		(char*) pat, (char*) r.replace);
	subRHS(r.replace);
	for (u_int i = 0, n = regex->length(); i < n; i++) {
	    RegEx* re = (*regex)[i];
	    if (strcmp(re->pattern(), pat) == 0)
		break;
	}
	if (i >= n) {
	    r.pat = new RegEx(pat, pat.length());
	    regex->append(r.pat);
	} else
	    r.pat = (*regex)[i];
	rules.append(r);
    }
}

#include <stdarg.h>

void
DialStringRules::parseError(const char* fmt ...)
{
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s: line %u: ", (char*) filename, lineno); 
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    putc('\n', stderr);
}

fxStr
DialStringRules::applyRules(const fxStr& name, const fxStr& s)
{
    fxStr result(s);
    RuleArray* ra = (*rules)[name];
    if (ra) {
	for (u_int i = 0, n = (*ra).length(); i < n; i++) {
	    DialRule& rule = (*ra)[i];
	    u_int len = result.length();
	    u_int start = 0;
	    while (rule.pat->Search(result, len, start, len) >= 0) {
		/*
		 * Regular expression match.
		 */
		int ix = rule.pat->BeginningOfMatch();
		int len = rule.pat->EndOfMatch() - ix;
		if (len == 0)		// avoid looping on zero-length matches
		    break;
		/*
		 * Do ``&'' and ``\n'' interpolations in the
		 * replacement.  NB: & is treated as \0.
		 */
		fxStr replace(rule.replace);
		for (u_int ri = 0, rlen = replace.length(); ri < rlen; ri++)
		    if (replace[ri] & META) {
			u_char mn = replace[ri] &~ META;
			int ms = rule.pat->BeginningOfMatch(mn);
			int mlen = rule.pat->EndOfMatch(mn) - ms;
			replace.remove(ri);	// delete & or \n
			replace.insert(result.extract(ms, mlen), ri);
			rlen = replace.length();// adjust string length ...
			ri += mlen - 1;		// ... and scan index
		    }
		/*
		 * Paste interpolated replacement string over match.
		 */
		result.remove(ix, len);
		result.insert(replace, ix);
		start = ix + replace.length();	// skip replace when searching
	    }
	}
    }
    return result;
}

/*
 * Apply the canonical number rule set to a string.
 */
fxStr DialStringRules::canonicalNumber(const fxStr& s)
    { return applyRules("CanonicalNumber", s); }
/*
 * Apply the dial string rule set to a string.
 */
fxStr DialStringRules::dialString(const fxStr& s)
    { return applyRules("DialString", s); }
/*
 * Apply the display number rule set to a string.
 */
fxStr DialStringRules::displayNumber(const fxStr& s)
    { return applyRules("DisplayNumber", s); }
