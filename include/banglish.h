#ifndef BANGLISH_H
#define BANGLISH_H

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>
#include <ctype.h>

/* ============================================================
 *  Forward declarations
 * ============================================================ */
typedef struct Value Value;
typedef struct Env Env;
typedef struct Node Node;
typedef struct Obj Obj;
typedef struct ObjString ObjString;
typedef struct ObjArray ObjArray;
typedef struct ObjMap ObjMap;
typedef struct ObjFunction ObjFunction;
typedef struct ObjNative ObjNative;
typedef struct Interpreter Interpreter;

/* ============================================================
 *  Allocator tracking (simple sweep list, GC-less arena)
 * ============================================================ */
typedef enum {
    OBJ_STRING,
    OBJ_ARRAY,
    OBJ_MAP,
    OBJ_FUNCTION,
    OBJ_NATIVE
} ObjType;

struct Obj {
    ObjType type;
    struct Obj *next;   /* intrusive linked list for sweep/free at exit */
};

void obj_track(Obj *o);
void obj_sweep_all(void);

/* ============================================================
 *  Value tagged union
 * ============================================================ */
typedef enum {
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOL,
    VAL_NULL,
    VAL_ARRAY,
    VAL_MAP,
    VAL_FUNCTION,
    VAL_NATIVE_FUNC
} ValueType;

struct Value {
    ValueType type;
    union {
        double number;
        bool boolean;
        Obj *obj;
    } as;
};

struct ObjString {
    Obj obj;
    char *chars;
    int length;
};

struct ObjArray {
    Obj obj;
    Value *items;
    int count;
    int capacity;
};

typedef struct MapEntry {
    char *key;          /* owned copy */
    Value value;
    bool used;
    bool tombstone;
} MapEntry;

struct ObjMap {
    Obj obj;
    MapEntry *entries;
    int capacity;
    int count;
};

struct ObjFunction {
    Obj obj;
    char *name;
    char **params;
    int param_count;
    Node *body;          /* block statement node */
    Env *closure;
};

typedef Value (*NativeFn)(Interpreter *interp, int arg_count, Value *args);

struct ObjNative {
    Obj obj;
    char *name;
    NativeFn fn;
};

#define AS_OBJ(v)      ((v).as.obj)
#define AS_NUMBER(v)   ((v).as.number)
#define AS_BOOL(v)     ((v).as.boolean)
#define AS_STRING(v)   ((ObjString*)((v).as.obj))
#define AS_CSTRING(v)  (((ObjString*)((v).as.obj))->chars)
#define AS_ARRAY(v)    ((ObjArray*)((v).as.obj))
#define AS_MAP(v)      ((ObjMap*)((v).as.obj))
#define AS_FUNCTION(v) ((ObjFunction*)((v).as.obj))
#define AS_NATIVE(v)   ((ObjNative*)((v).as.obj))

#define IS_NUMBER(v)   ((v).type == VAL_NUMBER)
#define IS_STRING(v)   ((v).type == VAL_STRING)
#define IS_BOOL(v)     ((v).type == VAL_BOOL)
#define IS_NULL(v)     ((v).type == VAL_NULL)
#define IS_ARRAY(v)    ((v).type == VAL_ARRAY)
#define IS_MAP(v)      ((v).type == VAL_MAP)
#define IS_FUNCTION(v) ((v).type == VAL_FUNCTION)
#define IS_NATIVE(v)   ((v).type == VAL_NATIVE_FUNC)

Value make_number(double n);
Value make_bool(bool b);
Value make_null(void);
Value make_string(const char *chars, int length);
Value make_string_take(char *chars, int length); /* takes ownership */
Value make_array(void);
Value make_map(void);
Value make_function(char *name, char **params, int param_count, Node *body, Env *closure);
Value make_native(const char *name, NativeFn fn);

bool values_equal(Value a, Value b);
bool value_truthy(Value v);
void value_print(Value v);
const char *value_type_name(Value v);
char *value_to_string(Value v); /* heap alloc'd, caller frees */

/* Array helpers */
void array_push(ObjArray *arr, Value v);
Value array_pop(ObjArray *arr);
Value array_get(ObjArray *arr, int index);
void array_set(ObjArray *arr, int index, Value v);

/* Map helpers */
void map_set(ObjMap *map, const char *key, Value v);
bool map_get(ObjMap *map, const char *key, Value *out);
bool map_delete(ObjMap *map, const char *key);

/* ============================================================
 *  Lexer
 * ============================================================ */
typedef enum {
    /* literals */
    TOK_NUMBER, TOK_STRING, TOK_IDENT,
    /* keywords */
    TOK_DHORO, TOK_STHIR, TOK_KAJ, TOK_FEROT,
    TOK_JODI, TOK_NAHOLE_JODI, TOK_NAHOLE,
    TOK_JOTOKHON, TOK_GHURO, TOK_THAMOK, TOK_CHALIYE_JAO,
    TOK_SHOTTO, TOK_MITHA, TOK_KICHU_NA,
    TOK_EBONG, TOK_OTHOBA, TOK_NOI,
    /* punctuation */
    TOK_LPAREN, TOK_RPAREN, TOK_LBRACE, TOK_RBRACE,
    TOK_LBRACKET, TOK_RBRACKET,
    TOK_COMMA, TOK_SEMI, TOK_COLON, TOK_DOT,
    /* operators */
    TOK_PLUS, TOK_MINUS, TOK_STAR, TOK_SLASH, TOK_PERCENT,
    TOK_ASSIGN, TOK_EQ, TOK_NEQ,
    TOK_LT, TOK_LE, TOK_GT, TOK_GE,
    TOK_PLUS_ASSIGN, TOK_MINUS_ASSIGN,
    TOK_EOF, TOK_ERROR
} TokenType;

typedef struct {
    TokenType type;
    const char *start;
    int length;
    int line;
    double number_value;
} Token;

typedef struct {
    const char *src;
    const char *current;
    const char *start;
    int line;
} Lexer;

void lexer_init(Lexer *lx, const char *source);
Token lexer_next(Lexer *lx);
const char *token_type_name(TokenType t);

/* ============================================================
 *  AST Node definitions
 * ============================================================ */
typedef enum {
    /* expressions */
    ND_NUMBER, ND_STRING, ND_BOOL, ND_NULL, ND_IDENT,
    ND_ARRAY_LIT, ND_MAP_LIT,
    ND_BINARY, ND_UNARY, ND_LOGICAL,
    ND_ASSIGN, ND_INDEX_ASSIGN, ND_INDEX_GET,
    ND_CALL, ND_FUNC_EXPR,
    /* statements */
    ND_VAR_DECL, ND_CONST_DECL, ND_EXPR_STMT,
    ND_BLOCK, ND_IF, ND_WHILE, ND_FOR,
    ND_FUNC_DECL, ND_RETURN, ND_BREAK, ND_CONTINUE,
    ND_PROGRAM
} NodeType;

struct Node {
    NodeType type;
    int line;

    /* generic children */
    Node *a, *b, *c, *d;

    /* list-style children (block stmts, args, array items, params) */
    Node **list;
    int list_count;
    int list_capacity;

    /* literal payloads */
    double number_value;
    char *string_value;   /* for strings/idents/op text */
    bool bool_value;

    /* map literal: keys array (char*), values via list */
    char **keys;

    TokenType op; /* for binary/unary/logical/assign-op */
};

Node *node_new(NodeType type, int line);
void node_list_push(Node *n, Node *child);

typedef enum { DIAG_ERROR = 1, DIAG_WARNING = 2 } DiagnosticSeverity;
typedef struct {
    int line;
    DiagnosticSeverity severity;
    char message[512];
} Diagnostic;

/* ============================================================
 *  Parser
 * ============================================================ */
typedef struct {
    Lexer lx;
    Token current;
    Token previous;
    bool had_error;
    Diagnostic *diagnostics;
    int diagnostic_count;
    int diagnostic_capacity;
} Parser;

Node *parse_program(const char *source);
Node *parse_program_diagnose(const char *source, Diagnostic **out, int *count);
int semantic_analyze(Node *program, Diagnostic **out);
void report_stage_header(const char *name);
void report_tokens(const char *source);
void report_ast(Node *program);
void print_semantic_report(Node *program);
void print_diagnosis_summary(const char *source, Node *program, int lex_errors, int syntax_errors, int semantic_errors, double ms, bool interpreted, int runtime_error);
void report_diagnostics(const Diagnostic *d, int count);

/* ============================================================
 *  Environment
 * ============================================================ */
typedef struct EnvEntry {
    char *name;
    Value value;
    bool is_const;
    bool used;
    bool tombstone;
} EnvEntry;

struct Env {
    EnvEntry *entries;
    int capacity;
    int count;
    Env *parent;
};

Env *env_new(Env *parent);
void env_free_shallow(Env *env); /* frees table only, not parent chain */
bool env_define(Env *env, const char *name, Value v, bool is_const);
bool env_assign(Env *env, const char *name, Value v);
bool env_get(Env *env, const char *name, Value *out);

/* ============================================================
 *  Interpreter
 * ============================================================ */
typedef enum {
    SIG_NONE,
    SIG_RETURN,
    SIG_BREAK,
    SIG_CONTINUE
} SignalType;

struct Interpreter {
    Env *globals;
    Env *current_env;
    SignalType signal;
    Value return_value;
    bool had_runtime_error;
    char error_msg[512];
};

void interp_init(Interpreter *interp);
void interp_run(Interpreter *interp, Node *program);
Value interp_eval(Interpreter *interp, Node *node);
void interp_exec(Interpreter *interp, Node *node);
void runtime_error(Interpreter *interp, int line, const char *fmt, ...);

/* ============================================================
 *  Builtins registration
 * ============================================================ */
void register_builtins(Interpreter *interp);

#endif
