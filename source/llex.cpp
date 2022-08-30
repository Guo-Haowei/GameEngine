/*
** $Id: llex.c,v 2.96.1.1 2017/04/19 17:20:42 roberto Exp $
** Lexical Analyzer
** See Copyright Notice in lua.h
*/

#define llex_c
#define LUA_CORE

#include "lprefix.h"

#include <locale.h>
#include <string.h>

#include "lua.h"

#include "lctype.h"
#include "ldebug.h"
#include "ldo.h"
#include "lgc.h"
#include "llex.h"
#include "lobject.h"
#include "lparser.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "lzio.h"

#include <cassert>

bool LexState::IsCurNewline() 
{
    char c = m_cur[0];
    return c == '\n' || c == '\r';
}

/* ORDER RESERVED */
static const char* const luaX_tokens[] = {
    "and", "break", "do", "else", "elseif",
    "end", "false", "for", "function", "goto", "if",
    "in", "local", "nil", "not", "or", "repeat",
    "return", "then", "true", "until", "while",
    "//", "..", "...", "==", ">=", "<=", "~=",
    "<<", ">>", "::", "<eof>",
    "<number>", "<integer>", "<name>", "<string>"
};


void LexState::Save(int c)
{
    Buffer& b = *buff;
    if (b.GetLength() + 1 > b.GetCapacity()) {
        if (b.GetCapacity() >= MAX_SIZE / 2)
        {
            LexError("lexical element too long", 0);
        }
        b.Resize(L, b.GetCapacity() * 2);
    }
    const size_t length = b.GetLength();
    b.GetBuffer()[length] = static_cast<char>(c);
    b.SetLength(length + 1);
}

void LexState::Next() {
    ++m_cur;
}

void LexState::SaveAndNext() {
    Save(*m_cur);
    Next();
}

void luaX_init(lua_State* L)
{
    int i;
    TString* e = luaS_newliteral(L, LUA_ENV); /* create env name */
    luaC_fix(L, obj2gco(e));                  /* never collect this name */
    for (i = 0; i < NUM_RESERVED; i++) {
        TString* ts = luaS_new(L, luaX_tokens[i]);
        luaC_fix(L, obj2gco(ts));     /* reserved words are never collected */
        ts->extra = cast_byte(i + 1); /* reserved word */
    }
}

const char* LexState::TokenToStr(int token)
{
    if (token < FIRST_RESERVED) { /* single-byte symbols? */
        lua_assert(token == cast_uchar(token));
        return luaO_pushfstring(this->L, "'%c'", token);
    }

    const char* s = luaX_tokens[token - FIRST_RESERVED];
    // fixed format (symbols and reserved words)?
    if (token < TK_EOS) {
        return luaO_pushfstring(this->L, "'%s'", s);
    }

    // names, strings, and numerals
    return s;
}

static const char* txtToken(LexState* ls, int token)
{
    switch (token) {
    case TK_NAME:
    case TK_STRING:
    case TK_FLT:
    case TK_INT:
        ls->Save('\0');
        return luaO_pushfstring(ls->L, "'%s'", ls->buff->GetBuffer());
    default:
        return ls->TokenToStr(token);
    }
}

l_noret LexState::LexError(const char* msg, int token)
{
    msg = luaG_addinfo(this->L, msg, this->source, this->linenumber);
    if (token) {
        luaO_pushfstring(this->L, "%s near %s", msg, txtToken(this, token));
    }
    luaD_throw(this->L, LUA_ERRSYNTAX);
}

l_noret LexState::SyntaxError(const char* msg)
{
    LexError(msg, this->t.token);
}

/*
** creates a new string and anchors it in scanner's table so that
** it will not be collected until the end of the compilation
** (by that time it should be anchored somewhere)
*/
TString* luaX_newstring(LexState* ls, const char* str, size_t l)
{
    lua_State* L = ls->L;
    TValue* o;                             /* entry for 'str' */
    TString* ts = luaS_newlstr(L, str, l); /* create new string */
    setsvalue2s(L, L->top++, ts);          /* temporarily anchor it in stack */
    o = luaH_set(L, ls->h, L->top - 1);
    if (ttisnil(o)) { /* not in use yet? */
        /* boolean value does not need GC barrier;
       table has no metatable, so it does not need to invalidate cache */
        setbvalue(o, 1); /* t[string] = true */
        luaC_checkGC(L);
    } else {                         /* string already present */
        ts = tsvalue(keyfromval(o)); /* re-use value previously stored */
    }
    L->top--; /* remove string from stack */
    return ts;
}

/*
** increment line number and skips newline sequence (any of
** \n, \r, \n\r, or \r\n)
*/
static void inclinenumber(LexState* ls)
{
    int old = ls->m_cur[0];
    // lua_assert(currIsNewline(ls));
    ls->Next(); /* skip '\n' or '\r' */
    if (ls->IsCurNewline() && ls->m_cur[0] != old) {
        ls->Next(); /* skip '\n\r' or '\r\n' */
    }
    if (++ls->linenumber >= MAX_INT) {
        ls->LexError("chunk has too many lines", 0);
    }
}

void luaX_setinput(lua_State* L, LexState* ls, TString* source, int firstchar, const char* text)
{
    ls->t.token = 0;
    ls->L = L;
    ls->lookahead.token = TK_EOS; /* no look-ahead token */
    ls->fs = NULL;
    ls->linenumber = 1;
    ls->lastline = 1;
    ls->source = source;
    ls->envn = luaS_newliteral(L, LUA_ENV); /* get env name */
    ls->buff->Resize(ls->L, LUA_MINBUFFER); /* initialize buffer */

    ls->m_text = text;
    ls->m_cur = text;
}

/*
** =======================================================
** LEXICAL ANALYZER
** =======================================================
*/

bool LexState::Expect(char c)
{
    if (m_cur[0] == c) {
        Next();
        return true;
    }

    return false;
}

/*
** Check whether current char is in set 'set' (with two chars) and
** saves it
*/
static int check_next2(LexState* ls, const char* set)
{
    lua_assert(set[2] == '\0');
    char c = ls->m_cur[0];
    if (c == set[0] || c == set[1]) {
        ls->SaveAndNext();
        return 1;
    } else
        return 0;
}

/* LUA_NUMBER */
/*
** this function is quite liberal in what it accepts, as 'luaO_str2num'
** will reject ill-formed numerals.
*/
static int read_numeral(LexState* ls, SemInfo* seminfo)
{
    TValue obj;
    const char* expo = "Ee";
    int first = ls->m_cur[0];
    lua_assert(lisdigit(ls->m_p));
    ls->SaveAndNext();
    if (first == '0' && check_next2(ls, "xX")) /* hexadecimal? */
        expo = "Pp";
    for (;;) {
        if (check_next2(ls, expo)) /* exponent part? */
            check_next2(ls, "-+"); /* optional exponent sign */
        if (lisxdigit(ls->m_cur[0]))
            ls->SaveAndNext();
        else if (ls->m_cur[0] == '.')
            ls->SaveAndNext();
        else
            break;
    }
    ls->Save('\0');
    if (luaO_str2num(ls->buff->GetBuffer(), &obj) == 0) {
        /* format error? */
        ls->LexError("malformed number", TK_FLT);
    }
    if (ttisinteger(&obj)) {
        seminfo->i = ivalue(&obj);
        return TK_INT;
    } else {
        lua_assert(ttisfloat(&obj));
        seminfo->r = fltvalue(&obj);
        return TK_FLT;
    }
}

/*
** reads a sequence '[=*[' or ']=*]', leaving the last bracket.
** If sequence is well formed, return its number of '='s + 2; otherwise,
** return 1 if there is no '='s or 0 otherwise (an unfinished '[==...').
*/
static size_t skip_sep(LexState* ls)
{
    size_t count = 0;
    char s = ls->m_cur[0];
    lua_assert(s == '[' || s == ']');
    ls->SaveAndNext();
    while (ls->m_cur[0] == '=') {
        ls->SaveAndNext();
        count++;
    }
    return (ls->m_cur[0] == s) ? count + 2
        : (count == 0)        ? 1
                              : 0;
}

static void read_long_string(LexState* ls, SemInfo* seminfo, size_t sep)
{
    int line = ls->linenumber; /* initial line (for error message) */
    ls->SaveAndNext(); /* skip 2nd '[' */
    if (ls->IsCurNewline())     /* string starts with a newline? */
        inclinenumber(ls);     /* skip it */
    for (;;) {
        switch (ls->m_cur[0]) {
        case EOZ: { /* error */
            const char* what = (seminfo ? "string" : "comment");
            const char* msg = luaO_pushfstring(ls->L,
                "unfinished long %s (starting at line %d)", what, line);
            ls->LexError(msg, TK_EOS);
            break; /* to avoid warnings */
        }
        case ']': {
            if (skip_sep(ls) == sep) {
                ls->SaveAndNext(); /* skip 2nd ']' */
                goto endloop;
            }
            break;
        }
        case '\n':
        case '\r': {
            ls->Save('\n');
            inclinenumber(ls);
            if (!seminfo) {
                ls->buff->Reset();
                /* avoid wasting space */
            }
            break;
        }
        default: {
            if (seminfo) {
                ls->SaveAndNext();
            } else {
                ls->Next();
            }
        }
        }
    }
endloop:
    if (seminfo)
    {
        seminfo->ts = luaX_newstring(ls, ls->buff->GetBuffer() + sep, ls->buff->GetLength() - 2 * sep);
    }
}

static void esccheck(LexState* ls, int c, const char* msg)
{
    if (!c) {
        if (ls->m_cur[0] != EOZ) {
            ls->SaveAndNext(); /* add current to buffer for error message */
        }
        ls->LexError(msg, TK_STRING);
    }
}

static int gethexa(LexState* ls)
{
    ls->SaveAndNext();
    esccheck(ls, lisxdigit(ls->m_cur[0]), "hexadecimal digit expected");
    return luaO_hexavalue(ls->m_cur[0]);
}

static int readhexaesc(LexState* ls)
{
    int r = gethexa(ls);
    r = (r << 4) + gethexa(ls);
    ls->buff->Remove(2); /* remove saved chars from buffer */
    return r;
}

static unsigned long readutf8esc(LexState* ls)
{
    unsigned long r;
    int i = 4;         /* chars to be removed: '\', 'u', '{', and first digit */
    ls->SaveAndNext(); /* skip 'u' */
    esccheck(ls, ls->m_cur[0] == '{', "missing '{'");
    r = gethexa(ls); /* must have at least one digit */
    while ((ls->SaveAndNext(), lisxdigit(ls->m_cur[0]))) {
        i++;
        r = (r << 4) + luaO_hexavalue(ls->m_cur[0]);
        esccheck(ls, r <= 0x10FFFF, "UTF-8 value too large");
    }
    esccheck(ls, ls->m_cur[0] == '}', "missing '}'");
    ls->Next(); /* skip '}' */
    ls->buff->Remove(i); /* remove saved chars from buffer */
    return r;
}

static void utf8esc(LexState* ls)
{
    assert(0);
    char buff[UTF8BUFFSZ];
    int n = luaO_utf8esc(buff, readutf8esc(ls));
    /* add 'buff' to string */
    for (; n > 0; n--) {
        ls->Save(buff[UTF8BUFFSZ - n]);
    }
}

static int readdecesc(LexState* ls)
{
    int i;
    int r = 0;                                         /* result accumulator */
    for (i = 0; i < 3 && lisdigit(ls->m_cur[0]); i++) { /* read up to 3 digits */
        r = 10 * r + ls->m_cur[0] - '0';
        ls->SaveAndNext();
    }
    esccheck(ls, r <= UCHAR_MAX, "decimal escape too large");
    ls->buff->Remove(i); /* remove read digits from buffer */
    return r;
}

static void read_string(LexState* ls, int del, SemInfo* seminfo)
{
    ls->SaveAndNext(); /* keep delimiter (for error messages) */
    while (ls->m_cur[0] != del) {
        switch (ls->m_cur[0]) {
        case EOZ:
            ls->LexError("unfinished string", TK_EOS);
            break; /* to avoid warnings */
        case '\n':
        case '\r':
            ls->LexError("unfinished string", TK_STRING);
            break;             /* to avoid warnings */
        case '\\': {           /* escape sequences */
            int c;             /* final character to be saved */
            ls->SaveAndNext(); /* keep '\\' for error messages */
            switch (ls->m_cur[0]) {
            case 'a':
                c = '\a';
                goto read_save;
            case 'b':
                c = '\b';
                goto read_save;
            case 'f':
                c = '\f';
                goto read_save;
            case 'n':
                c = '\n';
                goto read_save;
            case 'r':
                c = '\r';
                goto read_save;
            case 't':
                c = '\t';
                goto read_save;
            case 'v':
                c = '\v';
                goto read_save;
            case 'x':
                c = readhexaesc(ls);
                goto read_save;
            case 'u':
                utf8esc(ls);
                goto no_save;
            case '\n':
            case '\r':
                inclinenumber(ls);
                c = '\n';
                goto only_save;
            case '\\':
            case '\"':
            case '\'':
                c = ls->m_cur[0];
                goto read_save;
            case EOZ:
                goto no_save;                 /* will raise an error next loop */
            case 'z': {                       /* zap following span of spaces */
                ls->buff->Remove(1);          /* remove '\\' */
                ls->Next(); /* skip the 'z' */
                while (lisspace(ls->m_cur[0])) {
                    if (ls->IsCurNewline()) {
                        inclinenumber(ls);
                    }
                    else {
                        ls->Next();
                    }
                }
                goto no_save;
            }
            default: {
                esccheck(ls, lisdigit(ls->m_cur[0]), "invalid escape sequence");
                c = readdecesc(ls); /* digital escape '\ddd' */
                goto only_save;
            }
            }
        read_save:
            ls->Next();
            /* go through */
        only_save:
            ls->buff->Remove(1); /* remove '\\' */
            ls->Save(c);
            /* go through */
        no_save:
            break;
        }
        default:
            ls->SaveAndNext();
        }
    }
    ls->SaveAndNext(); /* skip delimiter */
    seminfo->ts = luaX_newstring(ls, ls->buff->GetBuffer() + 1, ls->buff->GetLength() - 2);
}

int LexState::Lex(SemInfo* seminfo)
{
    buff->Reset();
    for (;;) {
        switch (m_cur[0]) {
        case '\n':
        case '\r': { /* line breaks */
            inclinenumber(this);
            break;
        }
        case ' ':
        case '\f':
        case '\t':
        case '\v': { /* spaces */
            Next();
            break;
        }
        case '-': { /* '-' or '--' (comment) */
            Next();
            if (m_cur[0] != '-')
                return '-';
            /* else is a comment */
            Next();
            if (m_cur[0] == '[') { /* long comment? */
                size_t sep = skip_sep(this);
                this->buff->Reset(); /* 'skip_sep' may dirty the buffer */
                if (sep >= 2) {
                    read_long_string(this, NULL, sep); /* skip long comment */
                    this->buff->Reset();               /* previous call may dirty the buff. */
                    break;
                }
            }
            /* else short comment */
            while (!IsCurNewline() && m_cur[0] != EOZ) {
                /* skip until end of line (or end of file) */
                Next();
            }
            break;
        }
        case '[': { /* long string or simply '[' */
            size_t sep = skip_sep(this);
            if (sep >= 2) {
                read_long_string(this, seminfo, sep);
                return TK_STRING;
            }

            /* '[=...' missing second bracket */
            if (sep == 0) {
                this->LexError("invalid long string delimiter", TK_STRING);
            }
            return '[';
        }
        case '=': {
            Next();
            if (Expect('='))
                return TK_EQ;
            else
                return '=';
        }
        case '<': {
            Next();
            if (Expect('='))
                return TK_LE;
            else if (Expect('<'))
                return TK_SHL;
            else
                return '<';
        }
        case '>': {
            Next();
            if (Expect('='))
                return TK_GE;
            else if (Expect('>'))
                return TK_SHR;
            else
                return '>';
        }
        case '/': {
            Next();
            if (Expect('/'))
                return TK_IDIV;
            else
                return '/';
        }
        case '~': {
            Next();
            if (Expect('='))
                return TK_NE;
            else
                return '~';
        }
        case ':': {
            Next();
            if (Expect(':'))
                return TK_DBCOLON;
            else
                return ':';
        }
        case '"':
        case '\'': { /* short literal strings */
            read_string(this, m_cur[0], seminfo);
            return TK_STRING;
        }
        case '.': { /* '.', '..', '...', or number */
            SaveAndNext();
            if (Expect('.')) {
                if (Expect('.'))
                    return TK_DOTS; /* '...' */
                else
                    return TK_CONCAT; /* '..' */
            } else if (!lisdigit(m_cur[0]))
                return '.';
            else
                return read_numeral(this, seminfo);
        }
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            return read_numeral(this, seminfo);
        }
        case EOZ: {
            return TK_EOS;
        }
        default: {
            if (lislalpha(m_cur[0])) { /* identifier or reserved word? */
                TString* ts;
                do {
                    SaveAndNext();
                } while (lislalnum(m_cur[0]));
                ts = luaX_newstring(this, buff->GetBuffer(), buff->GetLength());
                seminfo->ts = ts;
                if (isreserved(ts)) /* reserved word? */
                    return ts->extra - 1 + FIRST_RESERVED;
                else {
                    return TK_NAME;
                }
            } else { /* single-char tokens (+ - / ...) */
                int c = m_cur[0];
                Next();
                return c;
            }
        }
        }
    }
}

void luaX_next(LexState* ls)
{
    ls->lastline = ls->linenumber;
    if (ls->lookahead.token != TK_EOS) { /* is there a look-ahead token? */
        ls->t = ls->lookahead;           /* use this one */
        ls->lookahead.token = TK_EOS;    /* and discharge it */
    } else {
        ls->t.token = ls->Lex(&ls->t.seminfo); /* read next token */
    }
}

int luaX_lookahead(LexState* ls)
{
    lua_assert(ls->lookahead.token == TK_EOS);
    ls->lookahead.token = ls->Lex(&ls->lookahead.seminfo);
    return ls->lookahead.token;
}
