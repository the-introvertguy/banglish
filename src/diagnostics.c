#include "banglish.h"
#include <stdarg.h>

static const char *safe_text(Node *n) {
    return (n && n->string_value) ? n->string_value : "";
}

static const char *node_name(NodeType t) {
    switch (t) {
        case ND_NUMBER: return "NUMBER";
        case ND_STRING: return "STRING";
        case ND_BOOL: return "BOOL";
        case ND_NULL: return "NULL";
        case ND_IDENT: return "IDENT";
        case ND_ARRAY_LIT: return "ARRAY";
        case ND_MAP_LIT: return "MAP";
        case ND_BINARY: return "BINARY";
        case ND_UNARY: return "UNARY";
        case ND_LOGICAL: return "LOGICAL";
        case ND_ASSIGN: return "ASSIGN";
        case ND_INDEX_ASSIGN: return "INDEX_ASSIGN";
        case ND_INDEX_GET: return "INDEX_GET";
        case ND_CALL: return "CALL";
        case ND_FUNC_EXPR: return "FUNC_EXPR";
        case ND_VAR_DECL: return "VAR_DECL";
        case ND_CONST_DECL: return "CONST_DECL";
        case ND_EXPR_STMT: return "EXPR_STMT";
        case ND_BLOCK: return "BLOCK";
        case ND_IF: return "IF";
        case ND_WHILE: return "WHILE";
        case ND_FOR: return "FOR";
        case ND_FUNC_DECL: return "FUNC_DECL";
        case ND_RETURN: return "RETURN";
        case ND_BREAK: return "BREAK";
        case ND_CONTINUE: return "CONTINUE";
        case ND_PROGRAM: return "PROGRAM";
        default: return "UNKNOWN";
    }
}

static const char *op_text(TokenType t) {
    switch (t) {
        case TOK_PLUS: return "+"; case TOK_MINUS: return "-"; case TOK_STAR: return "*";
        case TOK_SLASH: return "/"; case TOK_PERCENT: return "%";
        case TOK_ASSIGN: return "="; case TOK_EQ: return "=="; case TOK_NEQ: return "!=";
        case TOK_LT: return "<"; case TOK_LE: return "<="; case TOK_GT: return ">"; case TOK_GE: return ">=";
        case TOK_PLUS_ASSIGN: return "+="; case TOK_MINUS_ASSIGN: return "-=";
        case TOK_EBONG: return "ebong"; case TOK_OTHOBA: return "othoba"; case TOK_NOI: return "noi";
        default: return token_type_name(t);
    }
}

static void tree_prefix(const char *prefix, bool last) {
    printf("%s%s", prefix, last ? "└── " : "├── ");
}

static void dump_node(Node *n, const char *prefix, bool last) {
    if (!n) return;
    tree_prefix(prefix, last);
    printf("%s", node_name(n->type));
    if (n->line > 0) printf(" [L%d]", n->line);
    if (n->type == ND_NUMBER) printf(": %g", n->number_value);
    else if (n->type == ND_STRING || n->type == ND_IDENT || n->type == ND_VAR_DECL || n->type == ND_CONST_DECL || n->type == ND_FUNC_DECL)
        printf(": %s", safe_text(n));
    else if (n->type == ND_BOOL) printf(": %s", n->bool_value ? "shotto" : "mitha");
    else if (n->type == ND_BINARY || n->type == ND_UNARY || n->type == ND_LOGICAL || n->type == ND_ASSIGN)
        printf(" (%s)", op_text(n->op));
    printf("\n");

    char child_prefix[1024];
    snprintf(child_prefix, sizeof(child_prefix), "%s%s", prefix, last ? "    " : "│   ");

    int total = 0;
    if (n->a) total++;
    if (n->b) total++;
    if (n->c) total++;
    if (n->d) total++;
    total += n->list_count;
    int seen = 0;

    if (n->a) { dump_node(n->a, child_prefix, ++seen == total); }
    if (n->b) { dump_node(n->b, child_prefix, ++seen == total); }
    if (n->c) { dump_node(n->c, child_prefix, ++seen == total); }
    if (n->d) { dump_node(n->d, child_prefix, ++seen == total); }
    for (int i = 0; i < n->list_count; i++) dump_node(n->list[i], child_prefix, ++seen == total);
}

void report_ast(Node *program) {
    report_stage_header("SYNTAX ANALYSIS / ABSTRACT SYNTAX TREE");
    if (!program) { printf("<no AST>\n"); return; }
    dump_node(program, "", true);
}

void report_tokens(const char *source) {
    Lexer lx;
    lexer_init(&lx, source);
    int count = 0, errors = 0;
    report_stage_header("LEXICAL ANALYSIS");
    printf("%-5s %-6s %-20s %-28s\n", "#", "LINE", "TOKEN TYPE", "LEXEME");
    printf("----------------------------------------------------------------\n");
    for (;;) {
        Token t = lexer_next(&lx);
        char lexeme[256];
        int len = t.length < 255 ? t.length : 255;
        if (t.start && len > 0) memcpy(lexeme, t.start, (size_t)len);
        lexeme[len] = '\0';
        if (t.type == TOK_ERROR) {
            errors++;
            printf("%-5d %-6d %-20s %s\n", count + 1, t.line, "LEXICAL_ERROR", lexeme);
        } else {
            printf("%-5d %-6d %-20s %s\n", count + 1, t.line, token_type_name(t.type), lexeme);
        }
        count++;
        if (t.type == TOK_EOF) break;
    }
    printf("\nTokens: %d  |  Lexical errors: %d\n", count, errors);
}

void report_stage_header(const char *name) {
    /* Bold, high-contrast ANSI colors. Disable colors with NO_COLOR=1. */
    const char *no_color = getenv("NO_COLOR");
    if (no_color && *no_color) {
        printf("\n%s\n\n", name);
        return;
    }

    const char *color = "\033[1;37m"; /* bold bright white */
    if (strstr(name, "LEXICAL"))
        color = "\033[1;96m"; /* bold bright cyan */
    else if (strstr(name, "SYNTAX"))
        color = "\033[1;95m"; /* bold bright magenta */
    else if (strstr(name, "SEMANTIC"))
        color = "\033[1;93m"; /* bold bright yellow */
    else if (strstr(name, "INTERPRETATION"))
        color = "\033[1;94m"; /* bold bright blue */
    else if (strstr(name, "DIAGNOSIS"))
        color = "\033[1;97m"; /* bold bright white */

    printf("\n%s%s\033[0m\n\n", color, name);
}

void report_diagnostics(const Diagnostic *d, int count) {
    if (count == 0) {
        printf("No errors.\n");
        return;
    }
    for (int i = 0; i < count; i++) {
        printf("%s[%s]%s Line %d: %s\n", (getenv("NO_COLOR") ? "" : (d[i].severity == DIAG_ERROR ? "\033[1;31m" : "\033[1;33m")), d[i].severity == DIAG_ERROR ? "ERROR" : "WARNING", (getenv("NO_COLOR") ? "" : "\033[0m"), d[i].line, d[i].message);
    }
    printf("\nTotal errors: %d\n", count);
}

/* A conservative semantic checker for this dynamically-typed language.
   It checks name visibility, duplicate declarations, const assignment,
   and control-flow context without imposing static types on expressions. */
typedef struct Sym {
    char *name;
    bool is_const;
    struct Sym *next;
} Sym;
typedef struct Scope {
    Sym *symbols;
    struct Scope *parent;
} Scope;

typedef struct {
    Diagnostic *items;
    int count;
    int cap;
} DiagList;

static void sem_add(DiagList *out, int line, const char *fmt, ...) {
    if (out->count == out->cap) {
        out->cap = out->cap ? out->cap * 2 : 16;
        out->items = realloc(out->items, sizeof(Diagnostic) * (size_t)out->cap);
    }
    Diagnostic *d = &out->items[out->count++];
    d->line = line; d->severity = DIAG_ERROR;
    va_list ap; va_start(ap, fmt); vsnprintf(d->message, sizeof(d->message), fmt, ap); va_end(ap);
}

static Scope *scope_new(Scope *parent) { Scope *s = calloc(1, sizeof(Scope)); s->parent = parent; return s; }
static Sym *scope_find_local(Scope *s, const char *name) { for (Sym *x=s?s->symbols:NULL; x; x=x->next) if (!strcmp(x->name,name)) return x; return NULL; }
static Sym *scope_find(Scope *s, const char *name) { for (; s; s=s->parent) { Sym *x=scope_find_local(s,name); if (x) return x; } return NULL; }
static void scope_add(Scope *s, const char *name, bool is_const) { Sym *x=calloc(1,sizeof(Sym)); x->name=strdup(name); x->is_const=is_const; x->next=s->symbols; s->symbols=x; }
static void scope_free(Scope *s) { if (!s) return; Sym *x=s->symbols; while(x){Sym *n=x->next; free(x->name); free(x); x=n;} free(s); }

static bool builtin_name(const char *name) {
    static const char *b[] = {"dekhao","shono","shono_shonkhya","doirgho","shesh_jog","shesh_bad","sajao","ultho","khojo","kato","bhoro","shonkhya","shobdo",NULL};
    for (int i=0;b[i];i++) if(!strcmp(name,b[i])) return true;
    return false;
}

static void sem_expr(Node *n, Scope *s, DiagList *out, int loop_depth, int func_depth);
static void sem_stmt(Node *n, Scope *s, DiagList *out, int loop_depth, int func_depth);

static void sem_expr(Node *n, Scope *s, DiagList *out, int loop_depth, int func_depth) {
    (void)loop_depth; (void)func_depth;
    if (!n) return;
    switch(n->type) {
        case ND_IDENT: {
            if (!scope_find(s,n->string_value) && !builtin_name(n->string_value))
                sem_add(out,n->line,"Undefined identifier '%s'.",n->string_value);
            break;
        }
        case ND_ASSIGN: {
            Sym *sym=scope_find(s,n->string_value);
            if (!sym) sem_add(out,n->line,"Assignment to undefined variable '%s'.",n->string_value);
            else if(sym->is_const) sem_add(out,n->line,"Cannot assign to constant '%s'.",n->string_value);
            sem_expr(n->a,s,out,loop_depth,func_depth); break;
        }
        case ND_INDEX_ASSIGN: sem_expr(n->a,s,out,loop_depth,func_depth); sem_expr(n->b,s,out,loop_depth,func_depth); sem_expr(n->c,s,out,loop_depth,func_depth); break;
        case ND_INDEX_GET: case ND_BINARY: case ND_UNARY: case ND_LOGICAL:
            sem_expr(n->a,s,out,loop_depth,func_depth); sem_expr(n->b,s,out,loop_depth,func_depth); break;
        case ND_CALL:
            sem_expr(n->a,s,out,loop_depth,func_depth); for(int i=0;i<n->list_count;i++) sem_expr(n->list[i],s,out,loop_depth,func_depth); break;
        case ND_ARRAY_LIT: case ND_MAP_LIT:
            for(int i=0;i<n->list_count;i++) {
                sem_expr(n->list[i],s,out,loop_depth,func_depth);
            }
            break;
        case ND_FUNC_EXPR: {
            Scope *fs=scope_new(s);
            for(int i=0;i<n->list_count;i++) { const char *p=n->list[i]->string_value; if(scope_find_local(fs,p)) sem_add(out,n->list[i]->line,"Duplicate parameter '%s'.",p); else scope_add(fs,p,false); }
            sem_stmt(n->a,fs,out,0,func_depth+1); scope_free(fs); break;
        }
        default: break;
    }
}

static void sem_block(Node *n, Scope *parent, DiagList *out, int loop_depth, int func_depth) {
    if (!n) return;
    Scope *bs=scope_new(parent);
    for(int i=0;i<n->list_count;i++) sem_stmt(n->list[i],bs,out,loop_depth,func_depth);
    scope_free(bs);
}

static void sem_stmt(Node *n, Scope *s, DiagList *out, int loop_depth, int func_depth) {
    if (!n) return;
    switch(n->type) {
        case ND_PROGRAM: for(int i=0;i<n->list_count;i++) sem_stmt(n->list[i],s,out,loop_depth,func_depth); break;
        case ND_BLOCK: sem_block(n,s,out,loop_depth,func_depth); break;
        case ND_VAR_DECL: case ND_CONST_DECL:
            if (scope_find_local(s,n->string_value)) sem_add(out,n->line,"Duplicate declaration of '%s'.",n->string_value);
            else scope_add(s,n->string_value,n->type==ND_CONST_DECL);
            sem_expr(n->a,s,out,loop_depth,func_depth); break;
        case ND_FUNC_DECL: {
            if (scope_find_local(s,n->string_value)) sem_add(out,n->line,"Duplicate declaration of '%s'.",n->string_value);
            else scope_add(s,n->string_value,false);
            Scope *fs=scope_new(s);
            for(int i=0;i<n->list_count;i++){ const char *p=n->list[i]->string_value; if(scope_find_local(fs,p)) sem_add(out,n->list[i]->line,"Duplicate parameter '%s'.",p); else scope_add(fs,p,false); }
            sem_stmt(n->a,fs,out,0,func_depth+1); scope_free(fs); break;
        }
        case ND_EXPR_STMT: sem_expr(n->a,s,out,loop_depth,func_depth); break;
        case ND_IF: sem_expr(n->a,s,out,loop_depth,func_depth); sem_stmt(n->b,s,out,loop_depth,func_depth); sem_stmt(n->c,s,out,loop_depth,func_depth); break;
        case ND_WHILE: sem_expr(n->a,s,out,loop_depth,func_depth); sem_stmt(n->b,s,out,loop_depth+1,func_depth); break;
        case ND_FOR: {
            Scope *fs=scope_new(s);
            if(n->a) sem_stmt(n->a,fs,out,loop_depth+1,func_depth);
            sem_expr(n->b,fs,out,loop_depth+1,func_depth); sem_expr(n->c,fs,out,loop_depth+1,func_depth);
            sem_stmt(n->d,fs,out,loop_depth+1,func_depth); scope_free(fs); break;
        }
        case ND_RETURN: if(func_depth==0) sem_add(out,n->line,"'ferot' can only be used inside a function."); sem_expr(n->a,s,out,loop_depth,func_depth); break;
        case ND_BREAK: if(loop_depth==0) sem_add(out,n->line,"'thamok' can only be used inside a loop."); break;
        case ND_CONTINUE: if(loop_depth==0) sem_add(out,n->line,"'chaliye_jao' can only be used inside a loop."); break;
        default: sem_expr(n,s,out,loop_depth,func_depth); break;
    }
}

int semantic_analyze(Node *program, Diagnostic **out) {
    DiagList d={0}; Scope *global=scope_new(NULL);
    const char *builtins[] = {"dekhao","shono","shono_shonkhya","doirgho","shesh_jog","shesh_bad","sajao","ultho","khojo","kato","bhoro","shonkhya","shobdo",NULL};
    for(int i=0;builtins[i];i++) scope_add(global,builtins[i],true);
    sem_stmt(program,global,&d,0,0); scope_free(global);
    *out=d.items; return d.count;
}

void print_semantic_report(Node *program) {
    report_stage_header("SEMANTIC ANALYSIS");
    Diagnostic *d=NULL; int count=semantic_analyze(program,&d);
    report_diagnostics(d,count);
    free(d);
}

void print_diagnosis_summary(const char *source, Node *program, int lex_errors, int syntax_errors, int semantic_errors, double ms, bool interpreted, int runtime_error) {
    report_stage_header("DIAGNOSIS SUMMARY");
    const bool no_color = getenv("NO_COLOR") && *getenv("NO_COLOR");
    const char *green = no_color ? "" : "\033[1;92m";
    const char *red = no_color ? "" : "\033[1;91m";
    const char *yellow = no_color ? "" : "\033[1;93m";
    const char *reset = no_color ? "" : "\033[0m";
    const char *lex = lex_errors ? "FAILED" : "PASSED";
    printf("Lexical analysis : %s%s%s (%d error%s)\n", lex_errors ? red : green, lex, reset, lex_errors, lex_errors==1?"":"s");
    if (lex_errors) printf("Syntax analysis  : %sSKIPPED%s (lexical errors)\n", yellow, reset);
    else { const char *st = syntax_errors ? "FAILED" : "PASSED"; printf("Syntax analysis  : %s%s%s (%d error%s)\n", syntax_errors ? red : green, st, reset, syntax_errors, syntax_errors==1?"":"s"); }
    if (lex_errors || syntax_errors) printf("Semantic analysis: %sSKIPPED%s (previous stage failed)\n", yellow, reset);
    else { const char *st = semantic_errors ? "FAILED" : "PASSED"; printf("Semantic analysis: %s%s%s (%d error%s)\n", semantic_errors ? red : green, st, reset, semantic_errors, semantic_errors==1?"":"s"); }
    if (interpreted) printf("Interpretation    : %s%s%s\n", runtime_error ? red : green, runtime_error ? "FAILED" : "PASSED", reset);
    else printf("Interpretation    : %sSKIPPED%s (analysis errors)\n", yellow, reset);
    if (interpreted) printf("Interpretation time: %.3f ms\n", ms);
    (void)source; (void)program;
    const bool failed = lex_errors || syntax_errors || semantic_errors || runtime_error;
    printf("\nOverall status    : %s%s%s\n", failed ? red : green, failed ? "FAILED" : "PASSED", reset);
}
