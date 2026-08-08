#include "banglish.h"

typedef struct {
    const char *word;
    TokenType type;
} KeywordEntry;

static KeywordEntry keywords[] = {
    {"dhoro", TOK_DHORO},
    {"sthir", TOK_STHIR},
    {"kaj", TOK_KAJ},
    {"ferot", TOK_FEROT},
    {"jodi", TOK_JODI},
    {"nahole_jodi", TOK_NAHOLE_JODI},
    {"nahole", TOK_NAHOLE},
    {"jotokhon", TOK_JOTOKHON},
    {"ghuro", TOK_GHURO},
    {"thamok", TOK_THAMOK},
    {"chaliye_jao", TOK_CHALIYE_JAO},
    {"shotto", TOK_SHOTTO},
    {"mitha", TOK_MITHA},
    {"kichu_na", TOK_KICHU_NA},
    {"ebong", TOK_EBONG},
    {"othoba", TOK_OTHOBA},
    {"noi", TOK_NOI},
    {NULL, TOK_EOF}
};

void lexer_init(Lexer *lx, const char *source) {
    lx->src = source;
    lx->current = source;
    lx->start = source;
    lx->line = 1;
}

static bool is_at_end(Lexer *lx) { return *lx->current == '\0'; }
static char advance(Lexer *lx) { return *lx->current++; }
static char peek(Lexer *lx) { return *lx->current; }
static char peek_next(Lexer *lx) { return is_at_end(lx) ? '\0' : lx->current[1]; }

static bool match(Lexer *lx, char expected) {
    if (is_at_end(lx)) return false;
    if (*lx->current != expected) return false;
    lx->current++;
    return true;
}

static Token make_token(Lexer *lx, TokenType type) {
    Token t;
    t.type = type;
    t.start = lx->start;
    t.length = (int)(lx->current - lx->start);
    t.line = lx->line;
    t.number_value = 0;
    return t;
}

static Token error_token(Lexer *lx, const char *msg) {
    Token t;
    t.type = TOK_ERROR;
    t.start = msg;
    t.length = (int)strlen(msg);
    t.line = lx->line;
    t.number_value = 0;
    return t;
}

static void skip_whitespace(Lexer *lx) {
    for (;;) {
        char c = peek(lx);
        switch (c) {
            case ' ': case '\r': case '\t':
                advance(lx);
                break;
            case '\n':
                lx->line++;
                advance(lx);
                break;
            case '/':
                if (peek_next(lx) == '/') {
                    while (peek(lx) != '\n' && !is_at_end(lx)) advance(lx);
                } else if (peek_next(lx) == '*') {
                    advance(lx); advance(lx);
                    while (!is_at_end(lx) && !(peek(lx) == '*' && peek_next(lx) == '/')) {
                        if (peek(lx) == '\n') lx->line++;
                        advance(lx);
                    }
                    if (!is_at_end(lx)) { advance(lx); advance(lx); }
                } else {
                    return;
                }
                break;
            default:
                return;
        }
    }
}

static bool is_digit(char c) { return c >= '0' && c <= '9'; }
static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static Token number_token(Lexer *lx) {
    while (is_digit(peek(lx))) advance(lx);
    if (peek(lx) == '.' && is_digit(peek_next(lx))) {
        advance(lx);
        while (is_digit(peek(lx))) advance(lx);
    }
    Token t = make_token(lx, TOK_NUMBER);
    char buf[128];
    int len = t.length < 127 ? t.length : 127;
    memcpy(buf, t.start, (size_t)len);
    buf[len] = '\0';
    t.number_value = atof(buf);
    return t;
}

static Token string_token(Lexer *lx) {
    /* lx->start points at opening quote; we build into a dynamic buffer to handle escapes */
    advance(lx); /* consume opening quote */
    char *buf = malloc(256);
    size_t cap = 256, len = 0;
    while (peek(lx) != '"' && !is_at_end(lx)) {
        char c = advance(lx);
        if (c == '\\') {
            char esc = advance(lx);
            switch (esc) {
                case 'n': c = '\n'; break;
                case 't': c = '\t'; break;
                case 'r': c = '\r'; break;
                case '"': c = '"'; break;
                case '\\': c = '\\'; break;
                case '0': c = '\0'; break;
                default: c = esc; break;
            }
        }
        if (c == '\n') lx->line++;
        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[len++] = c;
    }
    if (is_at_end(lx)) {
        free(buf);
        return error_token(lx, "Unterminated string.");
    }
    advance(lx); /* closing quote */
    buf[len] = '\0';
    Token t;
    t.type = TOK_STRING;
    t.start = buf; /* NOTE: heap-owned; parser must copy out then this leaks small amount (acceptable for tree-walk lifetime) */
    t.length = (int)len;
    t.line = lx->line;
    t.number_value = 0;
    return t;
}

static TokenType ident_type(const char *start, int length) {
    for (int i = 0; keywords[i].word != NULL; i++) {
        size_t klen = strlen(keywords[i].word);
        if ((int)klen == length && memcmp(start, keywords[i].word, (size_t)length) == 0) {
            return keywords[i].type;
        }
    }
    return TOK_IDENT;
}

static Token identifier_token(Lexer *lx) {
    while (is_alpha(peek(lx)) || is_digit(peek(lx))) advance(lx);
    Token t = make_token(lx, TOK_IDENT);
    t.type = ident_type(t.start, t.length);
    return t;
}

Token lexer_next(Lexer *lx) {
    skip_whitespace(lx);
    lx->start = lx->current;
    if (is_at_end(lx)) return make_token(lx, TOK_EOF);

    char c = peek(lx);
    if (is_digit(c)) return number_token(lx);
    if (is_alpha(c)) { advance(lx); return identifier_token(lx); }
    if (c == '"') return string_token(lx);

    advance(lx);
    switch (c) {
        case '(': return make_token(lx, TOK_LPAREN);
        case ')': return make_token(lx, TOK_RPAREN);
        case '{': return make_token(lx, TOK_LBRACE);
        case '}': return make_token(lx, TOK_RBRACE);
        case '[': return make_token(lx, TOK_LBRACKET);
        case ']': return make_token(lx, TOK_RBRACKET);
        case ',': return make_token(lx, TOK_COMMA);
        case ';': return make_token(lx, TOK_SEMI);
        case ':': return make_token(lx, TOK_COLON);
        case '.': return make_token(lx, TOK_DOT);
        case '+':
            if (match(lx, '=')) return make_token(lx, TOK_PLUS_ASSIGN);
            return make_token(lx, TOK_PLUS);
        case '-':
            if (match(lx, '=')) return make_token(lx, TOK_MINUS_ASSIGN);
            return make_token(lx, TOK_MINUS);
        case '*': return make_token(lx, TOK_STAR);
        case '/': return make_token(lx, TOK_SLASH);
        case '%': return make_token(lx, TOK_PERCENT);
        case '=':
            if (match(lx, '=')) return make_token(lx, TOK_EQ);
            return make_token(lx, TOK_ASSIGN);
        case '!':
            if (match(lx, '=')) return make_token(lx, TOK_NEQ);
            return error_token(lx, "Unexpected '!' (use 'noi' for logical not).");
        case '<':
            if (match(lx, '=')) return make_token(lx, TOK_LE);
            return make_token(lx, TOK_LT);
        case '>':
            if (match(lx, '=')) return make_token(lx, TOK_GE);
            return make_token(lx, TOK_GT);
        default:
            return error_token(lx, "Unexpected character.");
    }
}

const char *token_type_name(TokenType t) {
    switch (t) {
        case TOK_NUMBER: return "NUMBER";
        case TOK_STRING: return "STRING";
        case TOK_IDENT: return "IDENT";
        case TOK_DHORO: return "dhoro";
        case TOK_STHIR: return "sthir";
        case TOK_KAJ: return "kaj";
        case TOK_FEROT: return "ferot";
        case TOK_JODI: return "jodi";
        case TOK_NAHOLE_JODI: return "nahole_jodi";
        case TOK_NAHOLE: return "nahole";
        case TOK_JOTOKHON: return "jotokhon";
        case TOK_GHURO: return "ghuro";
        case TOK_THAMOK: return "thamok";
        case TOK_CHALIYE_JAO: return "chaliye_jao";
        case TOK_SHOTTO: return "shotto";
        case TOK_MITHA: return "mitha";
        case TOK_KICHU_NA: return "kichu_na";
        case TOK_EBONG: return "ebong";
        case TOK_OTHOBA: return "othoba";
        case TOK_NOI: return "noi";
        case TOK_LPAREN: return "(";
        case TOK_RPAREN: return ")";
        case TOK_LBRACE: return "{";
        case TOK_RBRACE: return "}";
        case TOK_LBRACKET: return "[";
        case TOK_RBRACKET: return "]";
        case TOK_COMMA: return ",";
        case TOK_SEMI: return ";";
        case TOK_COLON: return ":";
        case TOK_DOT: return ".";
        case TOK_PLUS: return "+";
        case TOK_MINUS: return "-";
        case TOK_STAR: return "*";
        case TOK_SLASH: return "/";
        case TOK_PERCENT: return "%";
        case TOK_ASSIGN: return "=";
        case TOK_EQ: return "==";
        case TOK_NEQ: return "!=";
        case TOK_LT: return "<";
        case TOK_LE: return "<=";
        case TOK_GT: return ">";
        case TOK_GE: return ">=";
        case TOK_PLUS_ASSIGN: return "+=";
        case TOK_MINUS_ASSIGN: return "-=";
        case TOK_EOF: return "EOF";
        case TOK_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
