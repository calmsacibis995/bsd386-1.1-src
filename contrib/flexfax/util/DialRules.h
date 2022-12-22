/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/DialRules.h,v 1.1.1.1 1994/01/14 23:10:48 torek Exp $
/*
 * Copyright (c) 1990, 1991, 1992, 1993 Sam Leffler
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
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
#ifndef _DialStringRules_
#define	_DialStringRules_
/*
 * FlexFAX Dialing String Processing Rules.
 *
 * This file describes how to process user-specified dialing strings
 * to create two items:
 *
 * CanonicalNumber: a unique string that is derived from all dialing
 * strings to the same destination phone number.  This string is used
 * by the fax server for ``naming'' the destination. 
 *
 * DialString: the string passed to the modem for use in dialing the
 * telephone.
 */
#include "Str.h"

class RuleArray;
class RegExArray;
class VarDict;
class RulesDict;

class DialStringRules {
private:
    fxStr	filename;	// source of rules
    u_int	lineno;		// current line number during parsing
    FILE*	fp;		// open file during parsing
    VarDict*	vars;		// defined variables during parsing
    fxBool	verbose;	// trace parsing of rules file
    RegExArray*	regex;		// regular expressions
    RulesDict*	rules;		// rules defined in the file

    fxBool parseRules();
    fxBool parseRuleSet(RuleArray& rules);
    const char* parseToken(const char* cp, fxStr& v);
    char* nextLine(char* line, int lineSize);
    void parseError(const char* fmt ...);
    void subRHS(fxStr& v);
public:
    DialStringRules(const char* filename);
    ~DialStringRules();

    void setVerbose(fxBool b);

    void def(const fxStr& var, const fxStr& value);
    void undef(const fxStr& var);

    fxBool parse();

    fxStr applyRules(const fxStr& name, const fxStr& s);
    fxStr canonicalNumber(const fxStr&);
    fxStr dialString(const fxStr&);
    fxStr displayNumber(const fxStr&);
};
#endif /* _DialStringRules_ */
