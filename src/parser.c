#include "banglish.h"

/* ============================================================
 *  Parser: recursive descent
 *  Precedence (low -> high):
 *    assignment
 *    logical or  (othoba)
 *    logical and (ebong)
 *    equality    (== !=)
 *    comparison  (< <= > >=)
 *    term        (+ -)
 *    factor      (* / %)
 *    unary       (- noi)
 *    call/index
 *    primary
 * ============================================================ */

static void parser_advance(Parser *p);
static Node *parse_statement(Parser *p);
static Node *parse_block(Parser *p);
static Node *parse_expression(Parser *p);
static Node *parse_assignment(Parser *p);

static char *tok_dup(Token t) {
    char *s = malloc((size_t)t.length + 1);
    memcpy(s, t.start, (size_t)t.length);
    s[t.length] = '\0';
    return s;
}

static void parser_error(Parser *p, const char *msg) {
    p->had_error = true;
    if (p->diagnostic_count == p->diagnostic_capacity) {
        p->diagnostic_capacity = p->diagnostic_capacity ? p->diagnostic_capacity * 2 : 8;
        p->diagnostics = realloc(p->diagnostics, sizeof(Diagnostic) * (size_t)p->diagnostic_capacity);
    }
    Diagnostic *d = &p->diagnostics[p->diagnostic_count++];
    d->line = p->current.line;
    d->severity = DIAG_ERROR;
    snprintf(d->message, sizeof(d->message), "%s (near '%.*s')", msg, p->current.length, p->current.start ? p->current.start : "");
}

static void parser_advance(Parser *p) {
    p->previous = p->current;
    for (;;) {
        p->current = lexer_next(&p->lx);
        if (p->current.type != TOK_ERROR) break;
        parser_error(p, p->current.start);
    }
}

static bool check(Parser *p, TokenType type) { return p->current.type == type; }

static bool match_tok(Parser *p, TokenType type) {
    if (!check(p, type)) return false;
    parser_advance(p);
    return true;
}

static void consume(Parser *p, TokenType type, const char *msg) {
    if (check(p, type)) { parser_advance(p); return; }
    parser_error(p, msg);
}

/* ---------- expression parsing ---------- */

static Node *parse_primary(Parser *p) {
    int line = p->current.line;

    if (match_tok(p, TOK_NUMBER)) {
        Node *n = node_new(ND_NUMBER, line);
        n->number_value = p->previous.number_value;
        return n;
    }
    if (match_tok(p, TOK_STRING)) {
        Node *n = node_new(ND_STRING, line);
        n->string_value = tok_dup(p->previous);
        return n;
    }
    if (match_tok(p, TOK_SHOTTO)) {
        Node *n = node_new(ND_BOOL, line); n->bool_value = true; return n;
    }
    if (match_tok(p, TOK_MITHA)) {
        Node *n = node_new(ND_BOOL, line); n->bool_value = false; return n;
    }
    if (match_tok(p, TOK_KICHU_NA)) {
        return node_new(ND_NULL, line);
    }
    if (match_tok(p, TOK_IDENT)) {
        Node *n = node_new(ND_IDENT, line);
        n->string_value = tok_dup(p->previous);
        return n;
    }
    if (match_tok(p, TOK_LPAREN)) {
        Node *n = parse_expression(p);
        consume(p, TOK_RPAREN, "Expect ')' after expression.");
        return n;
    }
    if (match_tok(p, TOK_LBRACKET)) {
        Node *n = node_new(ND_ARRAY_LIT, line);
        if (!check(p, TOK_RBRACKET)) {
            do {
                node_list_push(n, parse_assignment(p));
            } while (match_tok(p, TOK_COMMA));
        }
        consume(p, TOK_RBRACKET, "Expect ']' after array literal.");
        return n;
    }
    if (match_tok(p, TOK_LBRACE)) {
        Node *n = node_new(ND_MAP_LIT, line);
        int cap = 4, cnt = 0;
        char **keys = malloc(sizeof(char*) * (size_t)cap);
        if (!check(p, TOK_RBRACE)) {
            do {
                if (check(p, TOK_STRING)) {
                    parser_advance(p);
                    if (cnt + 1 > cap) { cap *= 2; keys = realloc(keys, sizeof(char*) * (size_t)cap); }
                    keys[cnt++] = tok_dup(p->previous);
                } else {
                    parser_error(p, "Expect string key in map literal.");
                    if (cnt + 1 > cap) { cap *= 2; keys = realloc(keys, sizeof(char*) * (size_t)cap); }
                    keys[cnt++] = strdup("?");
                }
                consume(p, TOK_COLON, "Expect ':' after map key.");
                node_list_push(n, parse_assignment(p));
            } while (match_tok(p, TOK_COMMA));
        }
        consume(p, TOK_RBRACE, "Expect '}' after map literal.");
        n->keys = keys;
        return n;
    }
    if (match_tok(p, TOK_KAJ)) {
        /* function expression: kaj (params) { body } */
        consume(p, TOK_LPAREN, "Expect '(' after 'kaj'.");
        Node *n = node_new(ND_FUNC_EXPR, line);
        if (!check(p, TOK_RPAREN)) {
            do {
                consume(p, TOK_IDENT, "Expect parameter name.");
                Node *pn = node_new(ND_IDENT, p->previous.line);
                pn->string_value = tok_dup(p->previous);
                node_list_push(n, pn);
            } while (match_tok(p, TOK_COMMA));
        }
        consume(p, TOK_RPAREN, "Expect ')' after parameters.");
        consume(p, TOK_LBRACE, "Expect '{' before function body.");
        n->a = parse_block(p);
        return n;
    }

    parser_error(p, "Expect expression.");
    parser_advance(p);
    return node_new(ND_NULL, line);
}

static Node *finish_call(Parser *p, Node *callee) {
    Node *n = node_new(ND_CALL, p->previous.line);
    n->a = callee;
    if (!check(p, TOK_RPAREN)) {
        do {
            node_list_push(n, parse_assignment(p));
        } while (match_tok(p, TOK_COMMA));
    }
    consume(p, TOK_RPAREN, "Expect ')' after arguments.");
    return n;
}

static Node *parse_call_index(Parser *p) {
    Node *expr = parse_primary(p);
    for (;;) {
        if (match_tok(p, TOK_LPAREN)) {
            expr = finish_call(p, expr);
        } else if (match_tok(p, TOK_LBRACKET)) {
            Node *idx = parse_expression(p);
            consume(p, TOK_RBRACKET, "Expect ']' after index.");
            Node *n = node_new(ND_INDEX_GET, p->previous.line);
            n->a = expr; n->b = idx;
            expr = n;
        } else if (match_tok(p, TOK_DOT)) {
            /* support obj.field sugar -> treated as string index */
            consume(p, TOK_IDENT, "Expect property name after '.'.");
            Node *keyNode = node_new(ND_STRING, p->previous.line);
            keyNode->string_value = tok_dup(p->previous);
            Node *n = node_new(ND_INDEX_GET, p->previous.line);
            n->a = expr; n->b = keyNode;
            expr = n;
        } else {
            break;
        }
    }
    return expr;
}

static Node *parse_unary(Parser *p) {
    if (check(p, TOK_MINUS) || check(p, TOK_NOI)) {
        TokenType op = p->current.type;
        int line = p->current.line;
        parser_advance(p);
        Node *n = node_new(ND_UNARY, line);
        n->op = op;
        n->a = parse_unary(p);
        return n;
    }
    return parse_call_index(p);
}

static Node *parse_factor(Parser *p) {
    Node *expr = parse_unary(p);
    while (check(p, TOK_STAR) || check(p, TOK_SLASH) || check(p, TOK_PERCENT)) {
        TokenType op = p->current.type;
        int line = p->current.line;
        parser_advance(p);
        Node *right = parse_unary(p);
        Node *n = node_new(ND_BINARY, line);
        n->op = op; n->a = expr; n->b = right;
        expr = n;
    }
    return expr;
}

static Node *parse_term(Parser *p) {
    Node *expr = parse_factor(p);
    while (check(p, TOK_PLUS) || check(p, TOK_MINUS)) {
        TokenType op = p->current.type;
        int line = p->current.line;
        parser_advance(p);
        Node *right = parse_factor(p);
        Node *n = node_new(ND_BINARY, line);
        n->op = op; n->a = expr; n->b = right;
        expr = n;
    }
    return expr;
}

static Node *parse_comparison(Parser *p) {
    Node *expr = parse_term(p);
    while (check(p, TOK_LT) || check(p, TOK_LE) || check(p, TOK_GT) || check(p, TOK_GE)) {
        TokenType op = p->current.type;
        int line = p->current.line;
        parser_advance(p);
        Node *right = parse_term(p);
        Node *n = node_new(ND_BINARY, line);
        n->op = op; n->a = expr; n->b = right;
        expr = n;
    }
    return expr;
}

static Node *parse_equality(Parser *p) {
    Node *expr = parse_comparison(p);
    while (check(p, TOK_EQ) || check(p, TOK_NEQ)) {
        TokenType op = p->current.type;
        int line = p->current.line;
        parser_advance(p);
        Node *right = parse_comparison(p);
        Node *n = node_new(ND_BINARY, line);
        n->op = op; n->a = expr; n->b = right;
        expr = n;
    }
    return expr;
}

static Node *parse_and(Parser *p) {
    Node *expr = parse_equality(p);
    while (check(p, TOK_EBONG)) {
        int line = p->current.line;
        parser_advance(p);
        Node *right = parse_equality(p);
        Node *n = node_new(ND_LOGICAL, line);
        n->op = TOK_EBONG; n->a = expr; n->b = right;
        expr = n;
    }
    return expr;
}

static Node *parse_or(Parser *p) {
    Node *expr = parse_and(p);
    while (check(p, TOK_OTHOBA)) {
        int line = p->current.line;
        parser_advance(p);
        Node *right = parse_and(p);
        Node *n = node_new(ND_LOGICAL, line);
        n->op = TOK_OTHOBA; n->a = expr; n->b = right;
        expr = n;
    }
    return expr;
}

static Node *parse_assignment(Parser *p) {
    Node *expr = parse_or(p);

    if (check(p, TOK_ASSIGN) || check(p, TOK_PLUS_ASSIGN) || check(p, TOK_MINUS_ASSIGN)) {
        TokenType op = p->current.type;
        int line = p->current.line;
        parser_advance(p);
        Node *value = parse_assignment(p);

        if (op != TOK_ASSIGN) {
            /* desugar x += v  ->  x = x + v */
            Node *bin = node_new(ND_BINARY, line);
            bin->op = (op == TOK_PLUS_ASSIGN) ? TOK_PLUS : TOK_MINUS;
            bin->a = expr;
            bin->b = value;
            value = bin;
        }

        if (expr->type == ND_IDENT) {
            Node *n = node_new(ND_ASSIGN, line);
            n->string_value = strdup(expr->string_value);
            n->a = value;
            return n;
        } else if (expr->type == ND_INDEX_GET) {
            Node *n = node_new(ND_INDEX_ASSIGN, line);
            n->a = expr->a; /* target container */
            n->b = expr->b; /* index/key */
            n->c = value;
            return n;
        } else {
            parser_error(p, "Invalid assignment target.");
            return expr;
        }
    }
    return expr;
}

static Node *parse_expression(Parser *p) {
    return parse_assignment(p);
}

/* ---------- statement parsing ---------- */

static void consume_semi(Parser *p) {
    consume(p, TOK_SEMI, "Expect ';' after statement.");
}

static Node *parse_var_decl(Parser *p, bool is_const) {
    int line = p->current.line;
    consume(p, TOK_IDENT, "Expect variable name.");
    char *name = tok_dup(p->previous);
    Node *n = node_new(is_const ? ND_CONST_DECL : ND_VAR_DECL, line);
    n->string_value = name;
    if (match_tok(p, TOK_ASSIGN)) {
        n->a = parse_expression(p);
    } else {
        n->a = NULL;
    }
    consume_semi(p);
    return n;
}

static Node *parse_if(Parser *p) {
    int line = p->current.line;
    consume(p, TOK_LPAREN, "Expect '(' after 'jodi'.");
    Node *cond = parse_expression(p);
    consume(p, TOK_RPAREN, "Expect ')' after condition.");
    consume(p, TOK_LBRACE, "Expect '{' before block.");
    Node *then_branch = parse_block(p);

    Node *n = node_new(ND_IF, line);
    n->a = cond;
    n->b = then_branch;
    n->c = NULL; /* else-if chain or else, stored as nested IF/BLOCK */

    if (match_tok(p, TOK_NAHOLE_JODI)) {
        n->c = parse_if(p); /* recursively parse as nested if (without requiring its own 'jodi') */
    } else if (match_tok(p, TOK_NAHOLE)) {
        consume(p, TOK_LBRACE, "Expect '{' before else block.");
        n->c = parse_block(p);
    }
    return n;
}

/* nahole_jodi reuses parse_if's condition parsing (it expects '(' after) */
static Node *parse_if_wrapper(Parser *p) {
    /* called after consuming 'jodi' OR 'nahole_jodi' token */
    return parse_if(p);
}

static Node *parse_while(Parser *p) {
    int line = p->current.line;
    consume(p, TOK_LPAREN, "Expect '(' after 'jotokhon'.");
    Node *cond = parse_expression(p);
    consume(p, TOK_RPAREN, "Expect ')' after condition.");
    consume(p, TOK_LBRACE, "Expect '{' before block.");
    Node *body = parse_block(p);
    Node *n = node_new(ND_WHILE, line);
    n->a = cond; n->b = body;
    return n;
}

static Node *parse_for(Parser *p) {
    /* ghuro dhoro i = 0; i < 10; i = i + 1 { ... } */
    int line = p->current.line;
    Node *n = node_new(ND_FOR, line);

    /* init */
    if (check(p, TOK_DHORO)) {
        parser_advance(p);
        n->a = parse_var_decl(p, false); /* consumes trailing ; */
    } else if (!check(p, TOK_SEMI)) {
        Node *e = node_new(ND_EXPR_STMT, p->current.line);
        e->a = parse_expression(p);
        consume_semi(p);
        n->a = e;
    } else {
        consume_semi(p);
        n->a = NULL;
    }

    /* condition */
    if (!check(p, TOK_SEMI)) {
        n->b = parse_expression(p);
    } else {
        n->b = NULL;
    }
    consume_semi(p);

    /* increment (no trailing semicolon, terminated by '{') */
    if (!check(p, TOK_LBRACE)) {
        n->c = parse_expression(p);
    } else {
        n->c = NULL;
    }

    consume(p, TOK_LBRACE, "Expect '{' before for-loop body.");
    n->d = parse_block(p);
    return n;
}

static Node *parse_func_decl(Parser *p) {
    int line = p->current.line;
    consume(p, TOK_IDENT, "Expect function name.");
    char *name = tok_dup(p->previous);
    consume(p, TOK_LPAREN, "Expect '(' after function name.");
    Node *n = node_new(ND_FUNC_DECL, line);
    n->string_value = name;
    if (!check(p, TOK_RPAREN)) {
        do {
            consume(p, TOK_IDENT, "Expect parameter name.");
            Node *pn = node_new(ND_IDENT, p->previous.line);
            pn->string_value = tok_dup(p->previous);
            node_list_push(n, pn);
        } while (match_tok(p, TOK_COMMA));
    }
    consume(p, TOK_RPAREN, "Expect ')' after parameters.");
    consume(p, TOK_LBRACE, "Expect '{' before function body.");
    n->a = parse_block(p);
    return n;
}

static Node *parse_return(Parser *p) {
    int line = p->current.line;
    Node *n = node_new(ND_RETURN, line);
    if (!check(p, TOK_SEMI)) {
        n->a = parse_expression(p);
    } else {
        n->a = NULL;
    }
    consume_semi(p);
    return n;
}

static Node *parse_block(Parser *p) {
    Node *n = node_new(ND_BLOCK, p->previous.line);
    while (!check(p, TOK_RBRACE) && !check(p, TOK_EOF)) {
        node_list_push(n, parse_statement(p));
    }
    consume(p, TOK_RBRACE, "Expect '}' after block.");
    return n;
}

static Node *parse_statement(Parser *p) {
    if (match_tok(p, TOK_DHORO)) return parse_var_decl(p, false);
    if (match_tok(p, TOK_STHIR)) return parse_var_decl(p, true);
    if (match_tok(p, TOK_KAJ)) return parse_func_decl(p);
    if (match_tok(p, TOK_JODI)) return parse_if_wrapper(p);
    if (match_tok(p, TOK_JOTOKHON)) return parse_while(p);
    if (match_tok(p, TOK_GHURO)) return parse_for(p);
    if (match_tok(p, TOK_FEROT)) return parse_return(p);
    if (match_tok(p, TOK_THAMOK)) {
        Node *n = node_new(ND_BREAK, p->previous.line);
        consume_semi(p);
        return n;
    }
    if (match_tok(p, TOK_CHALIYE_JAO)) {
        Node *n = node_new(ND_CONTINUE, p->previous.line);
        consume_semi(p);
        return n;
    }
    if (match_tok(p, TOK_LBRACE)) return parse_block(p);

    /* expression statement */
    Node *n = node_new(ND_EXPR_STMT, p->current.line);
    n->a = parse_expression(p);
    consume_semi(p);
    return n;
}

Node *parse_program_diagnose(const char *source, Diagnostic **out, int *count) {
    Parser p;
    memset(&p, 0, sizeof(p));
    lexer_init(&p.lx, source);
    p.had_error = false;
    parser_advance(&p); /* prime p.current */

    Node *program = node_new(ND_PROGRAM, 1);
    while (!check(&p, TOK_EOF)) {
        node_list_push(program, parse_statement(&p));
    }
    *out = p.diagnostics;
    *count = p.diagnostic_count;
    return program;
}

Node *parse_program(const char *source) {
    Diagnostic *d = NULL;
    int count = 0;
    Node *program = parse_program_diagnose(source, &d, &count);
    if (count) {
        for (int i = 0; i < count; i++)
            fprintf(stderr, "[Parse Error] Line %d: %s\n", d[i].line, d[i].message);
        free(d);
        fprintf(stderr, "Parsing failed due to syntax errors.\n");
        exit(65);
    }
    free(d);
    return program;
}
