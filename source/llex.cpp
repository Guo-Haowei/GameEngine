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

#define currIsNewline(ls) (ls->peek() == '\n' || ls->peek() == '\r')

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

#define save_and_next(ls) (save(ls, ls->peek()), ls->next())

static l_noret lexerror(LexState* ls, const char* msg, int token);

static void save(LexState* ls, int c)
{
    Buffer& b = *ls->buff;
    if (b.GetLength() + 1 > b.GetCapacity()) {
        if (b.GetCapacity() >= MAX_SIZE / 2) {
            lexerror(ls, "lexical element too long", 0);
        }
        b.Resize(ls->L, b.GetCapacity() * 2);
    }
    const size_t length = b.GetLength();
    b.GetBuffer()[length] = static_cast<char>(c);
    b.SetLength(length + 1);
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

static const char* txtToken(LexState* ls, int token)
{
    switch (token) {
    case TK_NAME:
    case TK_STRING:
    case TK_FLT:
    case TK_INT:
        save(ls, '\0');
        return luaO_pushfstring(ls->L, "'%s'", ls->buff->GetBuffer());
    default:
        return luaX_token2str(ls, token);
    }
}

static l_noret lexerror(LexState* ls, const char* msg, int token)
{
    msg = luaG_addinfo(ls->L, msg, ls->source, ls->linenumber);
    if (token)
        luaO_pushfstring(ls->L, "%s near %s", msg, txtToken(ls, token));
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

int LexState::peek()
{
    if (m_p < m_buffer.data() + m_buffer.size() - NUM_GUARD_ZEROS) {
        return *m_p;
    }
    return EOZ;
}

int LexState::next()
{
    int c = peek();
    if (c != EOZ) {
        ++m_p;
    }
    return c;
}
static void inclinenumber(LexState* ls)
{
    int old = ls->peek();
    assert(currIsNewline(ls));
    ls->next(); /* skip '\n' or '\r' */
    if (currIsNewline(ls) && ls->peek() != old)
        ls->next(); /* skip '\n\r' or '\r\n' */
    if (++ls->linenumber >= MAX_INT)
        lexerror(ls, "chunk has too many lines", 0);
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
    ls->buff->Resize(ls->L, LUA_MINBUFFER); /* initialize buffer */

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
    bool isFirstDot = *start == '.';
    m_p += static_cast<int>(isFirstDot);

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
static size_t skip_sep(LexState* ls)
{
    size_t count = 0;
    int s = ls->peek();
    assert(s == '[' || s == ']');
    save_and_next(ls);
    while (ls->peek() == '=') {
        save_and_next(ls);
        count++;
    }
    return (ls->peek() == s) ? count + 2
        : (count == 0)       ? 1
                             : 0;
}

static void esccheck(LexState* ls, int c, const char* msg)
{
    if (!c) {
        if (ls->peek() != EOZ)
            save_and_next(ls); /* add peek() to buffer for error message */
        lexerror(ls, msg, TK_STRING);
    }
}

static int gethexa(LexState* ls)
{
    save_and_next(ls);
    esccheck(ls, lisxdigit(ls->peek()), "hexadecimal digit expected");
    return luaO_hexavalue(ls->peek());
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
    save_and_next(ls); /* skip 'u' */
    esccheck(ls, ls->peek() == '{', "missing '{'");
    r = gethexa(ls); /* must have at least one digit */
    while ((save_and_next(ls), lisxdigit(ls->peek()))) {
        i++;
        r = (r << 4) + luaO_hexavalue(ls->peek());
        esccheck(ls, r <= 0x10FFFF, "UTF-8 value too large");
    }
    esccheck(ls, ls->peek() == '}', "missing '}'");
    ls->next();          /* skip '}' */
    ls->buff->Remove(i); /* remove saved chars from buffer */
    return r;
}

static void utf8esc(LexState* ls)
{
    char buff[UTF8BUFFSZ];
    int n = luaO_utf8esc(buff, readutf8esc(ls));
    for (; n > 0; n--) /* add 'buff' to string */
        save(ls, buff[UTF8BUFFSZ - n]);
}

static int readdecesc(LexState* ls)
{
    int i;
    int r = 0;                                        /* result accumulator */
    for (i = 0; i < 3 && lisdigit(ls->peek()); i++) { /* read up to 3 digits */
        r = 10 * r + ls->peek() - '0';
        save_and_next(ls);
    }
    esccheck(ls, r <= UCHAR_MAX, "decimal escape too large");
    ls->buff->Remove(i); /* remove read digits from buffer */
    return r;
}

static void read_string(LexState* ls, int del, SemInfo* seminfo)
{
    save_and_next(ls); /* keep delimiter (for error messages) */
    while (ls->peek() != del) {
        switch (ls->peek()) {
        case EOZ:
            lexerror(ls, "unfinished string", TK_EOS);
            break; /* to avoid warnings */
        case '\n':
        case '\r':
            lexerror(ls, "unfinished string", TK_STRING);
            break;             /* to avoid warnings */
        case '\\': {           /* escape sequences */
            int c;             /* final character to be saved */
            save_and_next(ls); /* keep '\\' for error messages */
            switch (ls->peek()) {
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
                c = ls->peek();
                goto read_save;
            case EOZ:
                goto no_save;        /* will raise an error next loop */
            case 'z': {              /* zap following span of spaces */
                ls->buff->Remove(1); /* remove '\\' */
                ls->next();          /* skip the 'z' */
                while (lisspace(ls->peek())) {
                    if (currIsNewline(ls))
                        inclinenumber(ls);
                    else
                        ls->next();
                }
                goto no_save;
            }
            default: {
                esccheck(ls, lisdigit(ls->peek()), "invalid escape sequence");
                c = readdecesc(ls); /* digital escape '\ddd' */
                goto only_save;
            }
            }
        read_save:
            ls->next();
            /* go through */
        only_save:
            ls->buff->Remove(1); /* remove '\\' */
            save(ls, c);
            /* go through */
        no_save:
            break;
        }
        default:
            save_and_next(ls);
        }
    }
    save_and_next(ls); /* skip delimiter */
    seminfo->ts = luaX_newstring(ls, ls->buff->GetBuffer() + 1, ls->buff->GetLength() - 2);
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

    buff->Reset();
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
            while (!currIsNewline(this) && peek() != EOZ) {
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
            size_t sep = skip_sep(this);
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
            read_string(this, c, seminfo);
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
