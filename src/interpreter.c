#include "banglish.h"
#include <stdarg.h>

/* forward */
static Value call_function(Interpreter *interp, Value callee, int arg_count, Value *args, int line);

void interp_init(Interpreter *interp) {
    interp->globals = env_new(NULL);
    interp->current_env = interp->globals;
    interp->signal = SIG_NONE;
    interp->return_value = make_null();
    interp->had_runtime_error = false;
    interp->error_msg[0] = '\0';
    register_builtins(interp);
}

void runtime_error(Interpreter *interp, int line, const char *fmt, ...) {
    if (interp->had_runtime_error) return; /* keep first error */
    char buf[400];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    snprintf(interp->error_msg, sizeof(interp->error_msg), "[Runtime Error] Line %d: %s", line, buf);
    interp->had_runtime_error = true;
    fprintf(stderr, "%s\n", interp->error_msg);
}

/* ============================================================
 *  Expression evaluation
 * ============================================================ */

static Value eval_binary(Interpreter *interp, Node *node) {
    Value l = interp_eval(interp, node->a);
    if (interp->had_runtime_error) return make_null();
    Value r = interp_eval(interp, node->b);
    if (interp->had_runtime_error) return make_null();

    switch (node->op) {
        case TOK_PLUS:
            if (IS_NUMBER(l) && IS_NUMBER(r)) {
                return make_number(AS_NUMBER(l) + AS_NUMBER(r));
            }
            if (IS_STRING(l) || IS_STRING(r)) {
                char *ls = value_to_string(l);
                char *rs = value_to_string(r);
                size_t len = strlen(ls) + strlen(rs);
                char *out = malloc(len + 1);
                snprintf(out, len + 1, "%s%s", ls, rs);
                free(ls); free(rs);
                return make_string_take(out, (int)len);
            }
            runtime_error(interp, node->line, "Operands must be numbers or strings for '+'.");
            return make_null();
        case TOK_MINUS:
            if (!IS_NUMBER(l) || !IS_NUMBER(r)) { runtime_error(interp, node->line, "Operands must be numbers for '-'."); return make_null(); }
            return make_number(AS_NUMBER(l) - AS_NUMBER(r));
        case TOK_STAR:
            if (!IS_NUMBER(l) || !IS_NUMBER(r)) { runtime_error(interp, node->line, "Operands must be numbers for '*'."); return make_null(); }
            return make_number(AS_NUMBER(l) * AS_NUMBER(r));
        case TOK_SLASH:
            if (!IS_NUMBER(l) || !IS_NUMBER(r)) { runtime_error(interp, node->line, "Operands must be numbers for '/'."); return make_null(); }
            if (AS_NUMBER(r) == 0.0) { runtime_error(interp, node->line, "Division by zero."); return make_null(); }
            return make_number(AS_NUMBER(l) / AS_NUMBER(r));
        case TOK_PERCENT:
            if (!IS_NUMBER(l) || !IS_NUMBER(r)) { runtime_error(interp, node->line, "Operands must be numbers for '%%'."); return make_null(); }
            if (AS_NUMBER(r) == 0.0) { runtime_error(interp, node->line, "Modulo by zero."); return make_null(); }
            return make_number(fmod(AS_NUMBER(l), AS_NUMBER(r)));
        case TOK_EQ:
            return make_bool(values_equal(l, r));
        case TOK_NEQ:
            return make_bool(!values_equal(l, r));
        case TOK_LT:
            if (IS_NUMBER(l) && IS_NUMBER(r)) return make_bool(AS_NUMBER(l) < AS_NUMBER(r));
            if (IS_STRING(l) && IS_STRING(r)) return make_bool(strcmp(AS_CSTRING(l), AS_CSTRING(r)) < 0);
            runtime_error(interp, node->line, "Operands not comparable with '<'.");
            return make_null();
        case TOK_LE:
            if (IS_NUMBER(l) && IS_NUMBER(r)) return make_bool(AS_NUMBER(l) <= AS_NUMBER(r));
            if (IS_STRING(l) && IS_STRING(r)) return make_bool(strcmp(AS_CSTRING(l), AS_CSTRING(r)) <= 0);
            runtime_error(interp, node->line, "Operands not comparable with '<='.");
            return make_null();
        case TOK_GT:
            if (IS_NUMBER(l) && IS_NUMBER(r)) return make_bool(AS_NUMBER(l) > AS_NUMBER(r));
            if (IS_STRING(l) && IS_STRING(r)) return make_bool(strcmp(AS_CSTRING(l), AS_CSTRING(r)) > 0);
            runtime_error(interp, node->line, "Operands not comparable with '>'.");
            return make_null();
        case TOK_GE:
            if (IS_NUMBER(l) && IS_NUMBER(r)) return make_bool(AS_NUMBER(l) >= AS_NUMBER(r));
            if (IS_STRING(l) && IS_STRING(r)) return make_bool(strcmp(AS_CSTRING(l), AS_CSTRING(r)) >= 0);
            runtime_error(interp, node->line, "Operands not comparable with '>='.");
            return make_null();
        default:
            runtime_error(interp, node->line, "Unknown binary operator.");
            return make_null();
    }
}

static Value eval_logical(Interpreter *interp, Node *node) {
    Value l = interp_eval(interp, node->a);
    if (interp->had_runtime_error) return make_null();
    if (node->op == TOK_OTHOBA) { /* or */
        if (value_truthy(l)) return l;
        return interp_eval(interp, node->b);
    } else { /* ebong / and */
        if (!value_truthy(l)) return l;
        return interp_eval(interp, node->b);
    }
}

static Value eval_unary(Interpreter *interp, Node *node) {
    Value v = interp_eval(interp, node->a);
    if (interp->had_runtime_error) return make_null();
    if (node->op == TOK_MINUS) {
        if (!IS_NUMBER(v)) { runtime_error(interp, node->line, "Operand must be a number for unary '-'."); return make_null(); }
        return make_number(-AS_NUMBER(v));
    }
    if (node->op == TOK_NOI) {
        return make_bool(!value_truthy(v));
    }
    runtime_error(interp, node->line, "Unknown unary operator.");
    return make_null();
}

static Value eval_array_lit(Interpreter *interp, Node *node) {
    Value arrVal = make_array();
    ObjArray *arr = AS_ARRAY(arrVal);
    for (int i = 0; i < node->list_count; i++) {
        Value item = interp_eval(interp, node->list[i]);
        if (interp->had_runtime_error) return make_null();
        array_push(arr, item);
    }
    return arrVal;
}

static Value eval_map_lit(Interpreter *interp, Node *node) {
    Value mapVal = make_map();
    ObjMap *m = AS_MAP(mapVal);
    for (int i = 0; i < node->list_count; i++) {
        Value v = interp_eval(interp, node->list[i]);
        if (interp->had_runtime_error) return make_null();
        map_set(m, node->keys[i], v);
    }
    return mapVal;
}

static Value eval_index_get(Interpreter *interp, Node *node) {
    Value container = interp_eval(interp, node->a);
    if (interp->had_runtime_error) return make_null();
    Value key = interp_eval(interp, node->b);
    if (interp->had_runtime_error) return make_null();

    if (IS_ARRAY(container)) {
        if (!IS_NUMBER(key)) { runtime_error(interp, node->line, "Array index must be a number."); return make_null(); }
        int idx = (int)AS_NUMBER(key);
        ObjArray *arr = AS_ARRAY(container);
        if (idx < 0) idx = arr->count + idx; /* negative indexing support */
        return array_get(arr, idx);
    }
    if (IS_MAP(container)) {
        if (!IS_STRING(key)) { runtime_error(interp, node->line, "Map key must be a string."); return make_null(); }
        Value out;
        if (map_get(AS_MAP(container), AS_CSTRING(key), &out)) return out;
        return make_null();
    }
    if (IS_STRING(container)) {
        if (!IS_NUMBER(key)) { runtime_error(interp, node->line, "String index must be a number."); return make_null(); }
        ObjString *s = AS_STRING(container);
        int idx = (int)AS_NUMBER(key);
        if (idx < 0) idx = s->length + idx;
        if (idx < 0 || idx >= s->length) return make_null();
        char c = s->chars[idx];
        return make_string(&c, 1);
    }
    runtime_error(interp, node->line, "Cannot index a value of type '%s'.", value_type_name(container));
    return make_null();
}

static Value eval_index_assign(Interpreter *interp, Node *node) {
    Value container = interp_eval(interp, node->a);
    if (interp->had_runtime_error) return make_null();
    Value key = interp_eval(interp, node->b);
    if (interp->had_runtime_error) return make_null();
    Value val = interp_eval(interp, node->c);
    if (interp->had_runtime_error) return make_null();

    if (IS_ARRAY(container)) {
        if (!IS_NUMBER(key)) { runtime_error(interp, node->line, "Array index must be a number."); return make_null(); }
        int idx = (int)AS_NUMBER(key);
        ObjArray *arr = AS_ARRAY(container);
        if (idx < 0) idx = arr->count + idx;
        array_set(arr, idx, val);
        return val;
    }
    if (IS_MAP(container)) {
        if (!IS_STRING(key)) { runtime_error(interp, node->line, "Map key must be a string."); return make_null(); }
        map_set(AS_MAP(container), AS_CSTRING(key), val);
        return val;
    }
    runtime_error(interp, node->line, "Cannot index-assign to a value of type '%s'.", value_type_name(container));
    return make_null();
}

static Value eval_call(Interpreter *interp, Node *node) {
    Value callee = interp_eval(interp, node->a);
    if (interp->had_runtime_error) return make_null();

    int argc = node->list_count;
    Value *args = malloc(sizeof(Value) * (size_t)(argc > 0 ? argc : 1));
    for (int i = 0; i < argc; i++) {
        args[i] = interp_eval(interp, node->list[i]);
        if (interp->had_runtime_error) { free(args); return make_null(); }
    }
    Value result = call_function(interp, callee, argc, args, node->line);
    free(args);
    return result;
}

static Value call_function(Interpreter *interp, Value callee, int arg_count, Value *args, int line) {
    if (IS_NATIVE(callee)) {
        ObjNative *n = AS_NATIVE(callee);
        return n->fn(interp, arg_count, args);
    }
    if (IS_FUNCTION(callee)) {
        ObjFunction *fn = AS_FUNCTION(callee);
        Env *call_env = env_new(fn->closure);

        for (int i = 0; i < fn->param_count; i++) {
            Value argv = (i < arg_count) ? args[i] : make_null();
            env_define(call_env, fn->params[i], argv, false);
        }

        Env *prev_env = interp->current_env;
        interp->current_env = call_env;

        interp_exec(interp, fn->body);

        Value ret = (interp->signal == SIG_RETURN) ? interp->return_value : make_null();
        interp->signal = SIG_NONE;
        interp->return_value = make_null();

        interp->current_env = prev_env;
        return ret;
    }
    runtime_error(interp, line, "Attempted to call a non-function value of type '%s'.", value_type_name(callee));
    return make_null();
}

Value interp_eval(Interpreter *interp, Node *node) {
    if (!node || interp->had_runtime_error) return make_null();

    switch (node->type) {
        case ND_NUMBER: return make_number(node->number_value);
        case ND_STRING: return make_string(node->string_value, (int)strlen(node->string_value));
        case ND_BOOL: return make_bool(node->bool_value);
        case ND_NULL: return make_null();
        case ND_IDENT: {
            Value v;
            if (env_get(interp->current_env, node->string_value, &v)) return v;
            runtime_error(interp, node->line, "Undefined variable '%s'.", node->string_value);
            return make_null();
        }
        case ND_ARRAY_LIT: return eval_array_lit(interp, node);
        case ND_MAP_LIT: return eval_map_lit(interp, node);
        case ND_BINARY: return eval_binary(interp, node);
        case ND_UNARY: return eval_unary(interp, node);
        case ND_LOGICAL: return eval_logical(interp, node);
        case ND_INDEX_GET: return eval_index_get(interp, node);
        case ND_INDEX_ASSIGN: return eval_index_assign(interp, node);
        case ND_CALL: return eval_call(interp, node);
        case ND_FUNC_EXPR: {
            int pc = node->list_count;
            char **params = malloc(sizeof(char*) * (size_t)(pc > 0 ? pc : 1));
            for (int i = 0; i < pc; i++) params[i] = strdup(node->list[i]->string_value);
            return make_function(NULL, params, pc, node->a, interp->current_env);
        }
        case ND_ASSIGN: {
            Value v = interp_eval(interp, node->a);
            if (interp->had_runtime_error) return make_null();
            if (!env_assign(interp->current_env, node->string_value, v)) {
                Value existing;
                if (env_get(interp->current_env, node->string_value, &existing)) {
                    runtime_error(interp, node->line, "Cannot assign to constant '%s'.", node->string_value);
                } else {
                    runtime_error(interp, node->line, "Undefined variable '%s'.", node->string_value);
                }
                return make_null();
            }
            return v;
        }
        default:
            runtime_error(interp, node->line, "Unexpected node in expression evaluation.");
            return make_null();
    }
}

/* ============================================================
 *  Statement execution
 * ============================================================ */

void interp_exec(Interpreter *interp, Node *node) {
    if (!node || interp->had_runtime_error || interp->signal != SIG_NONE) return;

    switch (node->type) {
        case ND_PROGRAM: {
            for (int i = 0; i < node->list_count; i++) {
                interp_exec(interp, node->list[i]);
                if (interp->had_runtime_error) break;
            }
            break;
        }
        case ND_BLOCK: {
            Env *block_env = env_new(interp->current_env);
            Env *prev = interp->current_env;
            interp->current_env = block_env;
            for (int i = 0; i < node->list_count; i++) {
                interp_exec(interp, node->list[i]);
                if (interp->had_runtime_error || interp->signal != SIG_NONE) break;
            }
            interp->current_env = prev;
            /* NOTE: we intentionally do NOT free block_env's table here because a function
               literal declared inside the block may have captured it as a closure. The env
               remains reachable via ObjFunction->closure and is cleaned up at process exit. */
            break;
        }
        case ND_EXPR_STMT: {
            interp_eval(interp, node->a);
            break;
        }
        case ND_VAR_DECL:
        case ND_CONST_DECL: {
            Value v = node->a ? interp_eval(interp, node->a) : make_null();
            if (interp->had_runtime_error) return;
            env_define(interp->current_env, node->string_value, v, node->type == ND_CONST_DECL);
            break;
        }
        case ND_FUNC_DECL: {
            int pc = node->list_count;
            char **params = malloc(sizeof(char*) * (size_t)(pc > 0 ? pc : 1));
            for (int i = 0; i < pc; i++) params[i] = strdup(node->list[i]->string_value);
            Value fnVal = make_function(strdup(node->string_value), params, pc, node->a, interp->current_env);
            env_define(interp->current_env, node->string_value, fnVal, false);
            break;
        }
        case ND_IF: {
            Value cond = interp_eval(interp, node->a);
            if (interp->had_runtime_error) return;
            if (value_truthy(cond)) {
                interp_exec(interp, node->b);
            } else if (node->c) {
                interp_exec(interp, node->c);
            }
            break;
        }
        case ND_WHILE: {
            while (true) {
                Value cond = interp_eval(interp, node->a);
                if (interp->had_runtime_error) return;
                if (!value_truthy(cond)) break;
                interp_exec(interp, node->b);
                if (interp->had_runtime_error) return;
                if (interp->signal == SIG_BREAK) { interp->signal = SIG_NONE; break; }
                if (interp->signal == SIG_CONTINUE) { interp->signal = SIG_NONE; continue; }
                if (interp->signal == SIG_RETURN) return;
            }
            break;
        }
        case ND_FOR: {
            Env *for_env = env_new(interp->current_env);
            Env *prev = interp->current_env;
            interp->current_env = for_env;

            if (node->a) interp_exec(interp, node->a);

            while (true) {
                if (interp->had_runtime_error) break;
                if (node->b) {
                    Value cond = interp_eval(interp, node->b);
                    if (interp->had_runtime_error) break;
                    if (!value_truthy(cond)) break;
                }
                interp_exec(interp, node->d);
                if (interp->had_runtime_error) break;
                if (interp->signal == SIG_BREAK) { interp->signal = SIG_NONE; break; }
                if (interp->signal == SIG_RETURN) break;
                if (interp->signal == SIG_CONTINUE) interp->signal = SIG_NONE;
                if (node->c) interp_eval(interp, node->c);
                if (interp->had_runtime_error) break;
            }
            interp->current_env = prev;
            break;
        }
        case ND_RETURN: {
            Value v = node->a ? interp_eval(interp, node->a) : make_null();
            if (interp->had_runtime_error) return;
            interp->return_value = v;
            interp->signal = SIG_RETURN;
            break;
        }
        case ND_BREAK: interp->signal = SIG_BREAK; break;
        case ND_CONTINUE: interp->signal = SIG_CONTINUE; break;
        default:
            runtime_error(interp, node->line, "Unexpected node in statement execution.");
            break;
    }
}

void interp_run(Interpreter *interp, Node *program) {
    interp_exec(interp, program);
}
