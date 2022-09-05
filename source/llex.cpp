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

#include <array>
#include <cassert>
#include <cctype>
#include <iostream>

// TODO: refactor
#define IS_DIGIT(c) ((c) >= '0' && (c) <= '9')

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

static l_noret lexerror(LexState* ls, const char* msg, int token);

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

const char* luaX_token2str(LexState* ls, int token)
{
    if (token < FIRST_RESERVED) { /* single-byte symbols? */
        assert(token == cast_uchar(token));
        return luaO_pushfstring(ls->L, "'%c'", token);
    } else {
        const char* s = luaX_tokens[token - FIRST_RESERVED];
        if (token < TK_EOS) /* fixed format (symbols and reserved words)? */
            return luaO_pushfstring(ls->L, "'%s'", s);
        else /* names, strings, and numerals */
            return s;
    }
}

#if 0
static const char* txtToken(LexState* ls, int token)
{
    switch (token) {
    case TK_NAME:
    case TK_STRING:
    case TK_FLT:
    case TK_INT:
        // return luaO_pushfstring(ls->L, "'%s'", ls->buff->GetBuffer());
        assert(0 && "TODO");
        break;
    default:
        return luaX_token2str(ls, token);
    }
}
#endif

static l_noret lexerror(LexState* ls, const char* msg, int token)
{
    msg = luaG_addinfo(ls->L, msg, ls->source, ls->linenumber);
    if (token) {
        // luaO_pushfstring(ls->L, "%s near %s", msg, txtToken(ls, token));
        // assert(0 && "TODO");
    }
    luaD_throw(ls->L, LUA_ERRSYNTAX);
}

l_noret luaX_syntaxerror(LexState* ls, const char* msg)
{
    lexerror(ls, msg, ls->t.token);
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

void LexState::setBuffer(const std::vector<char>& input)
{
    m_buffer = input;
    m_p = m_buffer.data();
}

char LexState::peek() const
{
    if (m_p < m_buffer.data() + m_buffer.size() - NUM_GUARD_ZEROS) {
        return *m_p;
    }
    return EOZ;
}

char LexState::next()
{
    char c = peek();
    if (c != EOZ) {
        ++m_p;
    }
    return c;
}

bool LexState::currIsNewline() const
{
    char c = peek();
    return c == '\r' || c == '\n';
}

void LexState::incLineNumber()
{
    char old = peek();
    assert(currIsNewline());
    next(); /* skip '\n' or '\r' */
    if (currIsNewline() && peek() != old) {
        next(); /* skip '\n\r' or '\r\n' */
    }
    if (++linenumber >= MAX_INT) {
        lexerror(this, "chunk has too many lines", 0);
    }
}

void luaX_setinput(lua_State* L, LexState* ls, TString* source, const std::vector<char>& input)
{
    ls->t.token = 0;
    ls->L = L;

    ls->lookahead.token = TK_EOS; /* no look-ahead token */
    ls->fs = NULL;
    ls->linenumber = 1;
    ls->lastline = 1;
    ls->source = source;
    ls->envn = luaS_newliteral(L, LUA_ENV); /* get env name */

    ls->setBuffer(input);
}

/*
** =======================================================
** LEXICAL ANALYZER
** =======================================================
*/

/* LUA_NUMBER */
/*
** Acceptable format
** 4, .4, 0.4, 0.3e12, 5e+20, 0x7DE
*/
void LexState::readNumeral(Token& tok)
{
    TValue obj;
    const char* start = m_p;
    m_p += *start == '.';

    const char* expo = "Ee";
    assert(lisdigit(peek()));
    if (strncmp(m_p, "0x", 2) == 0 || strncmp(m_p, "0X", 2) == 0) {
        expo = "Pp";
        m_p += 2;
    }
    for (;;) {
        if (strchr(expo, *m_p)) { // exponent part?
            next();
            if (strchr("+-", *m_p)) { // optional exponent sign
                next();
            }
        }

        if (lisxdigit(peek()) || peek() == '.') {
            next();
        } else
            break;
    }

    tok.raw = std::string(start, m_p - start);

    if (luaO_str2num(tok.raw.c_str(), &obj) == 0) {
        /* format error? */
        lexerror(this, "malformed number", TK_FLT);
    }

    if (ttisinteger(&obj)) {
        tok.seminfo.i = ivalue(&obj);
        tok.token = TK_INT;
        return;
    }

    assert(ttisfloat(&obj));
    tok.seminfo.r = fltvalue(&obj);
    tok.token = TK_FLT;
}

/*
** reads a sequence '[=*[' or ']=*]', leaving the last bracket.
** If sequence is well formed, return its number of '='s + 2; otherwise,
** return 1 if there is no '='s or 0 otherwise (an unfinished '[==...').
*/
// @TODO: support long string
size_t LexState::skipSep()
{
    size_t count = 0;
    int s = peek();
    assert(s == '[' || s == ']');
    // save_and_next(ls);
    next();
    while (peek() == '=') {
        // save_and_next(ls);
        next();
        count++;
    }
    return (peek() == s) ? count + 2
        : (count == 0)   ? 1
                         : 0;
}

void LexState::escCheck(int c, const char* msg)
{
    if (!c) {
        if (peek() != EOZ) {
            next();
            // save_and_next(ls); /* add peek() to buffer for error message */
        }
        lexerror(this, msg, TK_STRING);
    }
}

int LexState::getHex()
{
    next();
    char c = peek();
    assert(lisxdigit(c));
    // esccheck(, lisxdigit(ls->peek()), "hexadecimal digit expected");
    return luaO_hexavalue(c);
}

char LexState::readHex()
{
    int r = getHex();
    r = (r << 4) + getHex();
    assert(r >= 0 && r < 256);
    return static_cast<char>(r);
}

void LexState::utf8esc()
{
    assert(0 && "utf8 not supported yet");
}

// @TODO: refactor
int LexState::readDecEsc()
{
    int i;
    int r = 0;                                    /* result accumulator */
    for (i = 0; i < 3 && lisdigit(peek()); i++) { /* read up to 3 digits */
        r = 10 * r + peek() - '0';
        next();
    }
    escCheck(r <= UCHAR_MAX, "decimal escape too large");
    return r;
}

void LexState::readString(int del, SemInfo* seminfo)
{
    std::vector<char> buffer;
    next(); // skip opening ' or "

    for (;;) {
        char c = peek();

        if (c == del) {
            break;
        }
        if (c == EOZ) {
            lexerror(this, "unfinished string", TK_EOS);
        }
        if (c == '\r' || c == '\n') {
            lexerror(this, "unfinished string", TK_STRING);
        }

        if (c == '\\') {
            next(); // skip '\\'
            c = peek();
            if (c == 'x') {
                buffer.push_back(readHex());
                continue;
            }

            if (c == 'u') {
                utf8esc();
                continue;
            }

            if (c == '\r' || c == '\n') {
                incLineNumber();
                buffer.push_back('\n');
                continue;
            }

            if (c == 'z') {
                next(); // skip 'z'
                while (lisspace(peek())) {
                    if (currIsNewline()) {
                        incLineNumber();
                    } else {
                        next();
                    }
                }
                continue;
            }

            if (c >= 'a' && c <= 'z') {
                // clang-format off
                static const char escapes[] = {
                    '\a', '\b', -1, -1, -1, '\f', -1,
                    -1, -1, -1, -1, -1, -1, '\n',
                    -1, -1, -1, '\r', -1, '\t',
                    -1, '\v', -1, -1, -1, -1,
                };
                // clang-format on
                char escaped = escapes[c - 'a'];
                if (escaped != -1) {
                    buffer.push_back(escaped);
                    next();
                    continue;
                }
            }

            if (strchr("\'\"\\", c)) {
                buffer.push_back(c);
                next();
                continue;
            }

            if (c == EOZ) {
                continue; // raise exception
            }

            escCheck(lisdigit(c), "invalid escape sequence");
            buffer.push_back(static_cast<char>(readDecEsc())); // digital escape '\ddd'
            continue;
        }

        buffer.push_back(next());
    }
    next(); // skip closing ' or "

    seminfo->ts = luaX_newstring(this, buffer.data(), buffer.size());
}

bool LexState::tryReadLongPunct(Token& tok)
{
    struct Pair {
        const char* symbol;
        int type;
    };

    // clang-format off
    static const std::array<Pair, 10> puncts = {
        Pair{ "==", TK_EQ }, Pair{ "~=", TK_NE },
        Pair{ "<=", TK_LE }, Pair{ "<<", TK_SHL },
        Pair{ ">=", TK_GE }, Pair{ ">>", TK_SHR },
        Pair{ "//", TK_IDIV },
        Pair{ "::", TK_DBCOLON },
        Pair{ "...", TK_DOTS }, Pair{ "..", TK_CONCAT },
    };
    // clang-format on
    for (const auto& pair : puncts) {
        const size_t len = strlen(pair.symbol);
        if (strncmp(pair.symbol, m_p, len) == 0) {
            tok.token = pair.type;
            tok.raw = pair.symbol;
            m_p += len;
            return true;
        }
    }

    return false;
}

void LexState::readIdent(Token& tok)
{
    const char* start = m_p;
    int len = 1;

    for (;;) {
        int c = start[len];
        if (!lislalnum(c)) {
            break;
        }
        ++len;
    }

    tok.raw = std::string(start, len);
    m_p += len;

    TString* ts = luaX_newstring(this, start, len);
    tok.seminfo.ts = ts;
    if (isreserved(ts)) {
        /* reserved word? */
        tok.token = ts->extra - 1 + FIRST_RESERVED;
    } else {
        tok.token = TK_NAME;
    }

    return;
}

void LexState::lexOne(Token& tok)
{
    SemInfo* seminfo = &tok.seminfo;

    for (;;) {
        int c = peek();

        // white spaces
        if (strchr(" \n\r\f\t\v", c)) {
            next();
            continue;
        }

        // comments
        if (strncmp(m_p, "--[", 3) == 0) {
            printf("long comment not supported");
            exit(-1);
            return;
        }

        if (strncmp(m_p, "--", 2) == 0) {
            m_p += 2;
            while (!currIsNewline() && peek() != EOZ) {
                next();
            }
            continue;
        }

        if (c == EOZ) {
            tok.token = TK_EOS;
            return;
        }

        if (IS_DIGIT(c) || (c == '.' && IS_DIGIT(m_p[1]))) {
            readNumeral(tok);
            return;
        }

        if (strchr("=~<>/:.", c)) {
            if (tryReadLongPunct(tok)) {
                return;
            }
        }

        if (lislalpha(c)) {
            readIdent(tok);
            return;
        }

        // long string or simply '['
        if (c == '[') {
            size_t sep = skipSep();
            if (sep >= 2) {
                assert(0 && "long string not supported");
                exit(0);
                // tok.token = TK_STRING;
                // return;
            } else if (sep == 0) /* '[=...' missing second bracket */
                lexerror(this, "invalid long string delimiter", TK_STRING);
            tok.token = '[';
            return;
        }

        // short literal strings
        if (strchr("\"'", c)) {
            tok.token = TK_STRING;
            readString(c, seminfo);
            return;
        }

        // single-char tokens (+ - / ...)
        tok.token = next();
        return;
    }
}

void luaX_next(LexState* ls)
{
    ls->lastline = ls->linenumber;
    if (ls->lookahead.token != TK_EOS) { /* is there a look-ahead token? */
        ls->t = ls->lookahead;           /* use this one */
        ls->lookahead.token = TK_EOS;    /* and discharge it */
    } else {
        ls->lexOne(ls->t);
    }

#if 0
    const Token& t = ls->t;
    std::cout << "[token] '" << t.raw << "'\n";
#endif
}

int luaX_lookahead(LexState* ls)
{
    assert(ls->lookahead.token == TK_EOS);

    ls->lexOne(ls->lookahead);
    return ls->lookahead.token;
}
