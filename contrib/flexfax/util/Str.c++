/*	$Header: /bsdi/MASTER/BSDI_OS/contrib/flexfax/util/Str.c++,v 1.1.1.1 1994/01/14 23:10:48 torek Exp $
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
#include "Str.h"
#include <stdlib.h>
#include <ctype.h>

#define NUMBUFSIZE 2048
char fxStr::emptyString = '\0';

fxStr::fxStr(u_int l)
{
    slength = l+1;
    if (l>0) {
	data = new char[slength];
    } else
	data = &emptyString;
    memset(data,0,slength);
}

fxStr::fxStr(const char *s)
{
    u_int l = strlen(s)+1;
    if (l>1) {
	data = new char[l];
	memcpy(data,s,l);
    } else {
	data = &emptyString;
    }
    slength = l;
}

fxStr::fxStr(const char *s, u_int len)
{
    if (len>0) {
	data = new char[len+1];
	memcpy(data,s,len);
    } else
	data = &emptyString;
    slength = len+1;
    data[len] = 0;
}

fxStr::fxStr(const fxStr&s)
{
    slength = s.slength;
    if (slength > 1) {
	data = new char[slength];
	memcpy(data,s.data,slength);
    } else {
	data = &emptyString;
    }
}

fxStr::fxStr(const fxTempStr &t)
{
    slength = t.slength;
    if (t.slength>1) {
	data = new char[slength];
	memcpy(data,t.data,slength);
    } else {
	data = &emptyString;
    }
}

fxStr::fxStr(int a, const char * format)
{
    char buffer[NUMBUFSIZE];
    if (!format) format = "%d";
    sprintf(buffer,format,a);
    slength = strlen(buffer) + 1;
    data = new char[slength];
    memcpy(data,buffer,slength);
}

fxStr::fxStr(long a, const char * format)
{
    char buffer[NUMBUFSIZE];
    if (!format) format = "%ld";
    sprintf(buffer,format,a);
    slength = strlen(buffer) + 1;
    data = new char[slength];
    memcpy(data,buffer,slength);
}

fxStr::fxStr(float a, const char * format)
{
    char buffer[NUMBUFSIZE];
    if (!format) format = "%g";
    sprintf(buffer,format,a);
    slength = strlen(buffer) + 1;
    fxAssert(slength>1, "Str::Str(float): bogus conversion");
    data = new char[slength];
    memcpy(data,buffer,slength);
}

fxStr::fxStr(double a, const char * format)
{
    char buffer[NUMBUFSIZE];
    if (!format) format = "%lg";
    sprintf(buffer,format,a);
    slength = strlen(buffer) + 1;
    fxAssert(slength>1, "Str::Str(double): bogus conversion");
    data = new char[slength]; // XXX assume slength>1
    memcpy(data,buffer,slength);
}

fxStr::~fxStr()
{
    assert(data);
    if (data != &emptyString) delete data;
}

fxStr fxStr::extract(u_int start, u_int chars) const
{
    fxAssert(start+chars<slength, "Str::extract: Invalid range");
    return fxStr(data+start,chars);
}

fxStr fxStr::head(u_int chars) const
{
    fxAssert(chars<slength, "Str::head: Invalid size");
    return fxStr(data,chars);
}

fxStr fxStr::tail(u_int chars) const
{
    fxAssert(chars<slength, "Str::tail: Invalid size");
    return fxStr(data+slength-chars-1,chars);
}

void fxStr::lowercase()
{
    char* cp = data;
    for (int i = slength; i > 0; i--, cp++) {
#ifdef _tolower
	char c = *cp;
	if (isupper(c))
	    *cp = _tolower(c);
#else
	*cp = tolower(*cp);
#endif
    }
}

void fxStr::raisecase()
{
    char* cp = data;
    for (int i = slength; i > 0; i--, cp++) {
#ifdef _toupper
	char c = *cp;
	if (islower(c))
	    *cp = _toupper(c);
#else
	*cp = toupper(*cp);
#endif
    }
}

fxStr fxStr::copy() const
{
    return fxStr(data,slength-1);
}

void fxStr::remove(u_int start, u_int chars)
{
    fxAssert(start+chars<slength,"Str::remove: Invalid range");
    long move = slength-start-chars;		// we always move at least 1
    assert(move > 0);
    if (slength - chars <= 1) {
	resizeInternal(0);
	slength = 1;
    } else {
	memmove(data+start, data+start+chars, (u_int)move);
	slength -= chars;
    }
}

fxStr fxStr::cut(u_int start, u_int chars)
{
    fxAssert(start+chars<slength,"Str::cut: Invalid range");
    fxStr a(data+start, chars);
    remove(start, chars);
    return a;
}

void fxStr::insert(const char * v, u_int posn, u_int len)
{
    if (!len) len = strlen(v);
    if (!len) return;
    fxAssert(posn<slength, "Str::insert: Invalid index");
    u_int move = slength - posn;
    u_int nl = slength + len;
    resizeInternal(nl);
    /*
     * When move is one we are always moving \0; but beware
     * that the previous string might have been null before
     * the call to resizeInternal; so set the byte explicitly.
     */
    if (move == 1)
	data[posn+len] = '\0';
    else
	memmove(data+posn+len, data+posn, move);
    memcpy(data+posn, v, len);
    slength = nl;
}

void fxStr::insert(char a, u_int posn)
{
    u_int nl = slength + 1;
    resizeInternal(nl);
    long move = (long)slength - (long)posn;
    fxAssert(move>0, "Str::insert(char): Invalid index");
    /*
     * When move is one we are always moving \0; but beware
     * that the previous string might have been null before
     * the call to resizeInternal; so set the byte explicitly.
     */
    if (move == 1)
	data[posn+1] = '\0';
    else
	memmove(data+posn+1, data+posn, (size_t) move);	// move string tail
    data[posn] = a;
    slength = nl;
}

void fxStr::resizeInternal(u_int chars)
{
    if (slength > 1) {
	if (chars > 0) {
	    if (chars >= slength)
		data = (char *)realloc(data,chars+1);
	} else {
	    assert(data != &emptyString);
	    free(data);
	    data = &emptyString;
	}
    } else {
	assert(data == &emptyString);
	if (chars)
	    data = new char[chars+1];
    }
}


void fxStr::resize(u_int chars, fxBool)
{
    resizeInternal(chars);
    if (chars>=slength)
	memset(data+slength, 0, chars+1-slength);
    slength = chars+1;
    data[chars] = 0;
}

void fxStr::setMaxLength(u_int len)
{
    if (slength>1) resizeInternal(fxmax(len,slength-1));
}

void fxStr::operator=(const fxTempStr& s)
{
    resizeInternal(s.slength-1);
    memcpy(data,s.data,s.slength);
    slength = s.slength;
}

void fxStr::operator=(const fxStr& s)
{
    resizeInternal(s.slength-1);
    memcpy(data,s.data,s.slength);
    slength = s.slength;
}

void fxStr::operator=(const char *s)
{
    u_int nl = strlen(s) + 1;
    resizeInternal(nl-1);
    slength = nl;
    memcpy(data,s,slength);
}

void fxStr::append(const char * s, u_int l)
{
    if (!l) l = strlen(s);
    if (!l) return;
    u_int nl = slength + l;
    resizeInternal(nl-1);
    memcpy(data+slength-1, s, l);
    slength = nl;
    data[slength-1] = 0;
}

void fxStr::append(char a)
{
    resizeInternal(slength);
    slength++;
    data[slength-2] = a;
    data[slength-1] = 0;
}

fxBool operator==(const fxStr& a,const fxStr& b)
{
    return (a.slength == b.slength) && (memcmp(a.data,b.data,a.slength) == 0);
}

fxBool operator==(const fxStr& a,const char* b)
{
    return (a.slength == strlen(b)+1) && (memcmp(a.data,b,a.slength) == 0);
}

fxBool operator==(const char* b, const fxStr& a)
{
    return (a.slength == strlen(b)+1) && (memcmp(a.data,b,a.slength) == 0);
} 

fxBool operator!=(const fxStr& a,const fxStr& b)
{
    return (a.slength != b.slength) || (memcmp(a.data,b.data,a.slength) != 0);
}

fxBool operator!=(const fxStr& a,const char* b)
{
    return (a.slength != strlen(b)+1) || (memcmp(a.data,b,a.slength) != 0);
}

fxBool operator!=(const char* b, const fxStr& a)
{
    return (a.slength != strlen(b)+1) || (memcmp(a.data,b,a.slength) != 0);
} 

fxBool operator>=(const fxStr& a,const fxStr& b)
{
    return strcmp(a,b) >= 0;
}

fxBool operator>=(const fxStr& a,const char* b)
{
    return strcmp(a,b) >= 0;
}

fxBool operator>=(const char* a, const fxStr& b)
{
    return strcmp(a,b) >= 0;
} 

fxBool operator>(const fxStr& a,const fxStr& b)
{
    return strcmp(a,b) > 0;
}

fxBool operator>(const fxStr& a,const char* b)
{
    return strcmp(a,b) > 0;
}

fxBool operator>(const char* a, const fxStr& b)
{
    return strcmp(a,b) > 0;
} 

fxBool operator<=(const fxStr& a,const fxStr& b)
{
    return strcmp(a,b) <= 0;
}

fxBool operator<=(const fxStr& a,const char* b)
{
    return strcmp(a,b) <= 0;
}

fxBool operator<=(const char* a, const fxStr& b)
{
    return strcmp(a,b) <= 0;
} 

fxBool operator<(const fxStr& a,const fxStr& b)
{
    return strcmp(a,b) < 0;
}

fxBool operator<(const fxStr& a,const char* b)
{
    return strcmp(a,b) < 0;
}

fxBool operator<(const char* a, const fxStr& b)
{
    return strcmp(a,b) < 0;
} 

int compare(const fxStr&a, const fxStr&b)
{
    return strcmp(a,b);
}

int compare(const fxStr&a, const char*b)
{
    return strcmp(a,b);
}

int compare(const char *a, const char *b)
{
    return strcmp(a,b);
}


static int quickFind(char a, const char * buf, u_int buflen)
{
    while (buflen--)
	if (*buf++ == a) return 1;
    return 0;
}

u_int fxStr::next(u_int posn, char a) const
{
    fxAssert(posn<slength, "Str::next: invalid index");
    char * buf = data+posn;
    u_int counter = slength-1-posn;
    while (counter--) {
	if (*buf == a) return (buf-data);
	buf++;
    }
    return slength-1;
}

u_int fxStr::next(u_int posn, const char * c, u_int clen) const
{
    fxAssert(posn<slength, "Str::next: invalid index");
    char * buf = data + posn;
    u_int counter = slength-1-posn;
    if (!clen) clen = strlen(c);
    while (counter--) {
	if (quickFind(*buf,c,clen)) return (buf-data);
	buf++;
    }
    return slength-1;
}

u_int fxStr::nextR(u_int posn, char a) const
{
    fxAssert(posn<slength, "Str::nextR: invalid index");
    char * buf = data + posn - 1;
    u_int counter = posn;
    while (counter--) {
	if (*buf == a) return (buf-data+1);
	buf--;
    }
    return 0;
}

u_int fxStr::nextR(u_int posn, const char * c, u_int clen) const
{
    fxAssert(posn<slength, "Str::nextR: invalid index");
    char * buf = data + posn - 1;
    u_int counter = posn;
    if (!clen) clen = strlen(c);
    while (counter--) {
	if (quickFind(*buf,c,clen)) return (buf-data+1);
	buf--;
    }
    return 0;
}

u_int fxStr::skip(u_int posn, char a) const
{
    fxAssert(posn<slength, "Str::skip: invalid index");
    char * buf = data+posn;
    u_int counter = slength-1-posn;
    while (counter--) {
	if (*buf != a) return (buf-data);
	buf++;
    }
    return slength-1;
}

u_int fxStr::skip(u_int posn, const char * c, u_int clen) const
{
    fxAssert(posn<slength, "Str::skip: invalid index");
    char * buf = data + posn;
    u_int counter = slength-1-posn;
    if (!clen) clen = strlen(c);
    while (counter--) {
	if (!quickFind(*buf,c,clen)) return (buf-data);
	buf++;
    }
    return slength-1;
}

u_int fxStr::skipR(u_int posn, char a) const
{
    fxAssert(posn<slength, "Str::skipR: invalid index");
    char * buf = data + posn - 1;
    u_int counter = posn;
    while (counter--) {
	if (*buf != a) return (buf-data+1);
	buf--;
    }
    return 0;
}

u_int fxStr::skipR(u_int posn, const char * c, u_int clen) const
{
    fxAssert(posn<slength, "Str::skipR: invalid index");
    char * buf = data + posn - 1;
    u_int counter = posn;
    if (!clen) clen = strlen(c);
    while (counter--) {
	if (!quickFind(*buf,c,clen)) return (buf-data+1);
	buf--;
    }
    return 0;
}

fxStr fxStr::token(u_int & posn, const char * delim, u_int dlen) const
{
    fxAssert(posn<slength, "Str::token: invalid index");
    if (!dlen) dlen = strlen(delim);
    u_int end = next(posn, delim, dlen);
    u_int old = posn;
    posn = skip(end, delim, dlen);
    return extract(old,end-old);
}

fxStr fxStr::token(u_int & posn, char a) const
{
    fxAssert(posn<slength, "Str::token: invalid index");
    u_int end = next(posn, a);
    u_int old = posn;
    posn = skip(end, a);
    return extract(old,end-old);
}

fxStr fxStr::tokenR(u_int & posn, const char * delim, u_int dlen) const
{
    fxAssert(posn<slength, "Str::tokenR: invalid index");
    if (!dlen) dlen = strlen(delim);
    u_int begin = nextR(posn, delim, dlen);
    u_int old = posn;
    posn = skipR(begin, delim, dlen);
    return extract(begin, old-begin);
}

fxStr fxStr::tokenR(u_int & posn, char a) const
{
    fxAssert(posn<slength, "Str::tokenR: invalid index");
    u_int begin = nextR(posn, a);
    u_int old = posn;
    posn = skipR(begin, a);
    return extract(begin,old-begin);
}

u_long fxStr::hash() const
{
    char * elementc = data;
    u_int slen = slength - 1;
    u_long k = 0;
    if (slen < 2*sizeof(k)) {
	if (slen <= sizeof(k)) {
	    memcpy((char *)&k + (sizeof(k) - slen), elementc, slen);
	    k<<=3;
	} else {
	    memcpy((char *)&k + (sizeof(k)*2 - slen), elementc, slen-sizeof(k));
	    k<<=3;
	    k ^= *(u_long *)elementc;
	}
    } else {
	k = *(u_long *)(elementc + sizeof(k));
	k<<=3;
	k ^= *(u_long *)elementc;
    }
    return k;
}

//--- concatenation support ----------------------------------

fxTempStr::fxTempStr(const char *d1, u_int l1, const char *d2, u_int l2)
{
    slength = l1 + l2 + 1;
    if (slength <= sizeof(indata)) {
	data = &indata[0];
    } else {
	data = new char[slength];
    }
    memcpy(data,d1,l1);
    memcpy(data+l1,d2,l2);
    data[l1+l2] = 0;
}

fxTempStr::fxTempStr(fxTempStr const &other)
{
    slength = other.slength;
    if (slength <= sizeof (indata)) {
	data = &indata[0];
    } else {
	data = new char[slength];
    }
    memcpy(data, other.data, slength);
    data[slength] = 0;
}

fxTempStr::~fxTempStr()
{
    if (data != indata) delete data;
}

fxTempStr& operator|(const fxTempStr& ts, const fxStr &b)
{
    return ((fxTempStr &)ts).concat(b.data, b.slength-1);
}

fxTempStr& operator|(const fxTempStr& ts, const char *b)
{
    return ((fxTempStr &)ts).concat(b, strlen(b));
}

fxTempStr& fxTempStr::concat(const char* b, u_int bl)
{
    if (slength <= sizeof(indata)) {
	// Current temporary is in the internal buffer.  See if the
	// concatenation will fit too.
	if (slength + bl > sizeof(indata)) {
	    // Have to malloc.
	    data = (char*) malloc(slength + bl);
	    memcpy(data, indata, slength - 1);
	}
    } else {
	// Temporary is already too large.
	data = (char*) realloc(data, slength + bl);
    }

    // concatenate data
    memcpy(data+slength-1, b, bl);
    slength += bl;
    data[slength-1] = 0;
    return *this;
}

fxTempStr operator|(const fxStr &a, const fxStr &b)
{
    return fxTempStr(a.data, a.slength-1, b.data, b.slength-1);
}

fxTempStr operator|(const fxStr &a, const char *b)
{
    return fxTempStr(a.data, a.slength-1, b, strlen(b));
}

fxTempStr operator|(const char *a, const fxStr &b)
{
    return fxTempStr(a, strlen(a), b.data, b.slength-1);
}
