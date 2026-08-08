#define _DEFAULT_SOURCE
#include "banglish.h"
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>

static const char *c_green(void) { return getenv("NO_COLOR") ? "" : "\033[1;32m"; }
static const char *c_red(void)   { return getenv("NO_COLOR") ? "" : "\033[1;31m"; }
static const char *c_yellow(void){ return getenv("NO_COLOR") ? "" : "\033[1;33m"; }
static const char *c_reset(void) { return getenv("NO_COLOR") ? "" : "\033[0m"; }
static const char *status_color(const char *s) {
    if (!strcmp(s, "PASSED")) return c_green();
    if (!strcmp(s, "FAILED")) return c_red();
    return c_yellow();
}

static void loading_animation(const char *message) {
    if (!isatty(STDOUT_FILENO)) return;

    static const char *frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    const int frame_count = (int)(sizeof(frames) / sizeof(frames[0]));
    const char *color = getenv("NO_COLOR") ? "" : "\033[1;36m";
    const char *reset = getenv("NO_COLOR") ? "" : "\033[0m";

    for (int i = 0; i < 12; ++i) {
        printf("\r%s%s%s %s", color, frames[i % frame_count], reset, message);
        fflush(stdout);
        usleep(60000);
    }

    printf("\r\033[2K");
    fflush(stdout);
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "Could not open file '%s'.\n", path); exit(74); }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); fprintf(stderr, "Out of memory.\n"); exit(71); }
    size_t read = fread(buf, 1, (size_t)size, f);
    buf[read] = '\0';
    fclose(f);
    return buf;
}

static int lexical_error_count(const char *source, int *token_count) {
    Lexer lx; lexer_init(&lx, source);
    int errors = 0, count = 0;
    for (;;) {
        Token t = lexer_next(&lx); count++;
        if (t.type == TOK_ERROR) errors++;
        if (t.type == TOK_EOF) break;
    }
    if (token_count) *token_count = count;
    return errors;
}

static int ast_node_count(Node *n) {
    if (!n) return 0;
    int c = 1;
    c += ast_node_count(n->a) + ast_node_count(n->b) + ast_node_count(n->c) + ast_node_count(n->d);
    for (int i=0;i<n->list_count;i++) c += ast_node_count(n->list[i]);
    return c;
}

static double interpret_source(const char *source, int *runtime_error) {
    Node *program = parse_program(source);
    Interpreter interp;
    interp_init(&interp);
    clock_t start = clock();
    interp_run(&interp, program);
    clock_t end = clock();
    double ms = 1000.0 * (double)(end - start) / (double)CLOCKS_PER_SEC;
    *runtime_error = interp.had_runtime_error ? 1 : 0;
    if (*runtime_error) fprintf(stderr, "[Runtime Error] %s\n", interp.error_msg);
    obj_sweep_all();
    return ms;
}

static double interpret_ast(Node *program, int *runtime_error) {
    Interpreter interp;
    interp_init(&interp);
    clock_t start = clock();
    interp_run(&interp, program);
    clock_t end = clock();
    double ms = 1000.0 * (double)(end - start) / (double)CLOCKS_PER_SEC;
    *runtime_error = interp.had_runtime_error ? 1 : 0;
    if (*runtime_error) fprintf(stderr, "[Runtime Error] %s\n", interp.error_msg);
    obj_sweep_all();
    return ms;
}

static void usage(const char *exe) {
    printf("BanglishScript compiler/interpreter diagnostics\n\n");
    printf("Usage:\n");
    printf("  %s <source.bs>                 Run program\n", exe);
    printf("  %s --token <source.bs>         Lexical analysis / token table\n", exe);
    printf("  %s --ast <source.bs>           Syntax analysis / AST tree\n", exe);
    printf("  %s --semantic <source.bs>      Semantic analysis\n", exe);
    printf("  %s --diagnosis <source.bs>     Full short diagnostic report\n", exe);
    printf("\nAliases: --tokens, --lexical, --syntax, --diagnose\n");
}

static int cmd_token(const char *source) {
    loading_animation("Performing lexical analysis...");
    report_tokens(source);
    return lexical_error_count(source, NULL) ? 65 : 0;
}

static int cmd_ast(const char *source) {
    loading_animation("Building abstract syntax tree...");
    Diagnostic *d = NULL; int count = 0;
    Node *program = parse_program_diagnose(source, &d, &count);
    report_stage_header("SYNTAX ANALYSIS");
    report_diagnostics(d, count);
    report_ast(program);
    free(d);
    return count ? 65 : 0;
}

static int cmd_semantic(const char *source) {
    loading_animation("Performing semantic analysis...");
    Diagnostic *sd = NULL; int scount = 0;
    Node *program = parse_program_diagnose(source, &sd, &scount);
    if (scount) {
        report_stage_header("SYNTAX ANALYSIS");
        report_diagnostics(sd, scount);
        printf("\nSemantic analysis skipped because syntax analysis failed.\n");
        free(sd);
        return 65;
    }
    free(sd);
    print_semantic_report(program);
    Diagnostic *sem = NULL; int sem_count = semantic_analyze(program, &sem);
    free(sem);
    return sem_count ? 66 : 0;
}

static int cmd_diagnosis(const char *source) {
    loading_animation("Running complete diagnosis...");
    int token_count = 0;
    int lex_errors = lexical_error_count(source, &token_count);

    Diagnostic *syntax = NULL; int syntax_count = 0;
    Node *program = NULL;
    if (!lex_errors) program = parse_program_diagnose(source, &syntax, &syntax_count);

    Diagnostic *sem = NULL; int sem_count = 0;
    if (!lex_errors && !syntax_count) sem_count = semantic_analyze(program, &sem);

    report_stage_header("LEXICAL ANALYSIS");
    printf("Tokens scanned: %d\n", token_count);
    printf("Status: %s%s%s\n", status_color(lex_errors ? "FAILED" : "PASSED"), lex_errors ? "FAILED" : "PASSED", c_reset());
    if (lex_errors) {
        Lexer lx; lexer_init(&lx, source);
        int i=0;
        for (;;) {
            Token t=lexer_next(&lx); i++;
            if (t.type==TOK_ERROR) printf("[ERROR] Line %d: %.*s\n",t.line,t.length,t.start);
            if(t.type==TOK_EOF)break;
        }
    }

    report_stage_header("SYNTAX ANALYSIS");
    if (lex_errors) {
        printf("Status: SKIPPED (lexical analysis failed)\n");
    } else {
        printf("AST nodes: %d\n", ast_node_count(program));
        printf("Status: %s%s%s\n", status_color(syntax_count ? "FAILED" : "PASSED"), syntax_count ? "FAILED" : "PASSED", c_reset());
        report_diagnostics(syntax, syntax_count);
    }

    report_stage_header("SEMANTIC ANALYSIS");
    if (lex_errors || syntax_count) {
        printf("Status: SKIPPED (previous stage failed)\n");
    } else {
        printf("Status: %s%s%s\n", status_color(sem_count ? "FAILED" : "PASSED"), sem_count ? "FAILED" : "PASSED", c_reset());
        report_diagnostics(sem, sem_count);
    }

    bool can_interpret = !lex_errors && !syntax_count && !sem_count;
    int runtime_error = 0;
    double ms = 0.0;
    if (can_interpret) {
        report_stage_header("INTERPRETATION");
        ms = interpret_ast(program, &runtime_error);
        printf("Interpretation time: %.3f ms\n", ms);
        printf("Status: %s%s%s\n", status_color(runtime_error ? "FAILED" : "PASSED"), runtime_error ? "FAILED" : "PASSED", c_reset());
    } else {
        report_stage_header("INTERPRETATION");
        printf("Status: SKIPPED (analysis errors must be fixed first)\n");
    }

    print_diagnosis_summary(source, program, lex_errors, syntax_count, sem_count, ms, can_interpret, runtime_error);
    free(syntax); free(sem);
    return (lex_errors || syntax_count || sem_count || runtime_error) ? 1 : 0;
}

static const char *DEMO_PROGRAM =
"dekhao(\"--- BanglishScript Runtime Demo ---\");\n"
"dhoro nums = [42, 7, 19, 3, 88, 1, 56];\n"
"sajao(nums); dekhao(nums);\n"
"kaj fibonacci(n) { jodi (n < 2) { ferot n; } ferot fibonacci(n-1) + fibonacci(n-2); }\n"
"dekhao(\"fib(10)=\", fibonacci(10));\n";

int main(int argc, char **argv) {
    if (argc == 1) {
        fprintf(stderr, "No source file given; running embedded demo.\n");
        int runtime_error=0; interpret_source(DEMO_PROGRAM,&runtime_error);
        return runtime_error ? 70 : 0;
    }
    if (argc == 2 && (!strcmp(argv[1], "--help") || !strcmp(argv[1], "-h"))) { usage(argv[0]); return 0; }

    const char *mode = NULL, *path = NULL;
    if (argc == 2) path = argv[1];
    else if (argc == 3) { mode = argv[1]; path = argv[2]; }
    else { usage(argv[0]); return 64; }

    char *source = read_file(path);
    int rc = 0;
    if (!mode) {
        int runtime_error=0;
        double ms=interpret_source(source,&runtime_error);
        printf("\n[Interpretation time: %.3f ms]\n",ms);
        rc=runtime_error?70:0;
    } else if (!strcmp(mode,"--token") || !strcmp(mode,"--tokens") || !strcmp(mode,"--lexical")) rc=cmd_token(source);
    else if (!strcmp(mode,"--ast") || !strcmp(mode,"--syntax")) rc=cmd_ast(source);
    else if (!strcmp(mode,"--semantic")) rc=cmd_semantic(source);
    else if (!strcmp(mode,"--diagnosis") || !strcmp(mode,"--diagnose")) rc=cmd_diagnosis(source);
    else { fprintf(stderr,"Unknown option '%s'.\n\n",mode); usage(argv[0]); rc=64; }
    free(source);
    return rc;
}
