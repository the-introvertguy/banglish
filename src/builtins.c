#include "banglish.h"

/* ============================================================
 *  I/O builtins
 * ============================================================ */

static Value native_dekhao(Interpreter *interp, int argc, Value *args) {
    (void)interp;
    for (int i = 0; i < argc; i++) {
        value_print(args[i]);
        if (i < argc - 1) printf(" ");
    }
    printf("\n");
    return make_null();
}

static char *read_line_raw(void) {
    size_t cap = 128, len = 0;
    char *buf = malloc(cap);
    int c;
    while ((c = fgetc(stdin)) != EOF && c != '\n') {
        if (len + 1 >= cap) { cap *= 2; buf = realloc(buf, cap); }
        buf[len++] = (char)c;
    }
    if (len == 0 && c == EOF) { free(buf); return NULL; }
    buf[len] = '\0';
    return buf;
}

static Value native_shono(Interpreter *interp, int argc, Value *args) {
    (void)interp; (void)argc; (void)args;
    char *line = read_line_raw();
    if (!line) return make_string("", 0);
    Value v = make_string_take(line, (int)strlen(line));
    return v;
}

static Value native_shono_shonkhya(Interpreter *interp, int argc, Value *args) {
    (void)interp; (void)argc; (void)args;
    char *line = read_line_raw();
    if (!line) return make_number(0);
    double d = atof(line);
    free(line);
    return make_number(d);
}

/* ============================================================
 *  STL-style algorithms
 * ============================================================ */

static Value native_doirgho(Interpreter *interp, int argc, Value *args) {
    if (argc < 1) { runtime_error(interp, 0, "doirgho() expects 1 argument."); return make_number(0); }
    Value v = args[0];
    if (IS_STRING(v)) return make_number(AS_STRING(v)->length);
    if (IS_ARRAY(v)) return make_number(AS_ARRAY(v)->count);
    if (IS_MAP(v)) return make_number(AS_MAP(v)->count);
    runtime_error(interp, 0, "doirgho() expects a string, array, or map.");
    return make_number(0);
}

static Value native_shesh_jog(Interpreter *interp, int argc, Value *args) {
    if (argc < 2 || !IS_ARRAY(args[0])) { runtime_error(interp, 0, "shesh_jog(arr, val) expects an array."); return make_null(); }
    array_push(AS_ARRAY(args[0]), args[1]);
    return args[0];
}

static Value native_shesh_bad(Interpreter *interp, int argc, Value *args) {
    if (argc < 1 || !IS_ARRAY(args[0])) { runtime_error(interp, 0, "shesh_bad(arr) expects an array."); return make_null(); }
    return array_pop(AS_ARRAY(args[0]));
}

static int compare_values_asc(const void *a, const void *b) {
    const Value *va = (const Value*)a;
    const Value *vb = (const Value*)b;
    if (IS_NUMBER(*va) && IS_NUMBER(*vb)) {
        double d = AS_NUMBER(*va) - AS_NUMBER(*vb);
        return (d < 0) ? -1 : (d > 0 ? 1 : 0);
    }
    if (IS_STRING(*va) && IS_STRING(*vb)) {
        return strcmp(AS_CSTRING(*va), AS_CSTRING(*vb));
    }
    /* mixed types: fall back to type name compare for stability */
    return strcmp(value_type_name(*va), value_type_name(*vb));
}

/* Custom quicksort (explicit implementation per spec, rather than relying solely on qsort)
   operating in-place on the ObjArray's backing store. */
static void quicksort_values(Value *items, int lo, int hi) {
    if (lo >= hi) return;
    Value pivot = items[(lo + hi) / 2];
    int i = lo, j = hi;
    while (i <= j) {
        while (compare_values_asc(&items[i], &pivot) < 0) i++;
        while (compare_values_asc(&items[j], &pivot) > 0) j--;
        if (i <= j) {
            Value tmp = items[i];
            items[i] = items[j];
            items[j] = tmp;
            i++; j--;
        }
    }
    if (lo < j) quicksort_values(items, lo, j);
    if (i < hi) quicksort_values(items, i, hi);
}

static Value native_sajao(Interpreter *interp, int argc, Value *args) {
    if (argc < 1 || !IS_ARRAY(args[0])) { runtime_error(interp, 0, "sajao(arr) expects an array."); return make_null(); }
    ObjArray *arr = AS_ARRAY(args[0]);
    if (arr->count > 1) quicksort_values(arr->items, 0, arr->count - 1);
    return args[0];
}

static Value native_ultho(Interpreter *interp, int argc, Value *args) {
    if (argc < 1) { runtime_error(interp, 0, "ultho() expects 1 argument."); return make_null(); }
    if (IS_ARRAY(args[0])) {
        ObjArray *arr = AS_ARRAY(args[0]);
        for (int i = 0, j = arr->count - 1; i < j; i++, j--) {
            Value tmp = arr->items[i];
            arr->items[i] = arr->items[j];
            arr->items[j] = tmp;
        }
        return args[0];
    }
    if (IS_STRING(args[0])) {
        ObjString *s = AS_STRING(args[0]);
        char *out = malloc((size_t)s->length + 1);
        for (int i = 0; i < s->length; i++) out[i] = s->chars[s->length - 1 - i];
        out[s->length] = '\0';
        return make_string_take(out, s->length);
    }
    runtime_error(interp, 0, "ultho() expects an array or string.");
    return make_null();
}

static Value native_khojo(Interpreter *interp, int argc, Value *args) {
    if (argc < 2) { runtime_error(interp, 0, "khojo(arr, target) expects 2 arguments."); return make_number(-1); }
    if (IS_ARRAY(args[0])) {
        ObjArray *arr = AS_ARRAY(args[0]);
        for (int i = 0; i < arr->count; i++) {
            if (values_equal(arr->items[i], args[1])) return make_number(i);
        }
        return make_number(-1);
    }
    if (IS_STRING(args[0]) && IS_STRING(args[1])) {
        const char *hay = AS_CSTRING(args[0]);
        const char *needle = AS_CSTRING(args[1]);
        const char *found = strstr(hay, needle);
        if (!found) return make_number(-1);
        return make_number(found - hay);
    }
    runtime_error(interp, 0, "khojo() expects an array or matching string types.");
    return make_number(-1);
}

static Value native_kato(Interpreter *interp, int argc, Value *args) {
    if (argc < 3) { runtime_error(interp, 0, "kato(container, start, end) expects 3 arguments."); return make_null(); }
    if (!IS_NUMBER(args[1]) || !IS_NUMBER(args[2])) { runtime_error(interp, 0, "kato() start/end must be numbers."); return make_null(); }
    int start = (int)AS_NUMBER(args[1]);
    int end = (int)AS_NUMBER(args[2]);

    if (IS_ARRAY(args[0])) {
        ObjArray *arr = AS_ARRAY(args[0]);
        if (start < 0) start = arr->count + start;
        if (end < 0) end = arr->count + end;
        if (start < 0) start = 0;
        if (end > arr->count) end = arr->count;
        Value outVal = make_array();
        ObjArray *out = AS_ARRAY(outVal);
        for (int i = start; i < end; i++) array_push(out, arr->items[i]);
        return outVal;
    }
    if (IS_STRING(args[0])) {
        ObjString *s = AS_STRING(args[0]);
        if (start < 0) start = s->length + start;
        if (end < 0) end = s->length + end;
        if (start < 0) start = 0;
        if (end > s->length) end = s->length;
        if (end < start) end = start;
        return make_string(s->chars + start, end - start);
    }
    runtime_error(interp, 0, "kato() expects an array or string.");
    return make_null();
}

static Value native_bhoro(Interpreter *interp, int argc, Value *args) {
    if (argc < 3 || !IS_NUMBER(args[2])) { runtime_error(interp, 0, "bhoro(val, count) expects a value and a count."); return make_null(); }
    /* Signature per spec: bhoro(arr, val, count) - but since arr doesn't need to pre-exist,
       we treat args[0] as ignored placeholder OR as val if only 2 real args given.
       To match the documented signature exactly: bhoro(arr, val, count) fills `arr` in-place
       with `count` copies of `val`, appended to whatever is already in arr. */
    if (!IS_ARRAY(args[0])) { runtime_error(interp, 0, "bhoro(arr, val, count): first argument must be an array."); return make_null(); }
    ObjArray *arr = AS_ARRAY(args[0]);
    int count = (int)AS_NUMBER(args[2]);
    for (int i = 0; i < count; i++) array_push(arr, args[1]);
    return args[0];
}

/* extra convenience natives (not required but harmless helpers) */
static Value native_shonkhya(Interpreter *interp, int argc, Value *args) {
    /* string -> number cast helper */
    if (argc < 1) return make_number(0);
    if (IS_NUMBER(args[0])) return args[0];
    if (IS_STRING(args[0])) return make_number(atof(AS_CSTRING(args[0])));
    if (IS_BOOL(args[0])) return make_number(AS_BOOL(args[0]) ? 1 : 0);
    (void)interp;
    return make_number(0);
}

static Value native_shobdo(Interpreter *interp, int argc, Value *args) {
    (void)interp;
    if (argc < 1) return make_string("", 0);
    char *s = value_to_string(args[0]);
    Value v = make_string_take(s, (int)strlen(s));
    return v;
}

void register_builtins(Interpreter *interp) {
    Env *g = interp->globals;

    env_define(g, "dekhao", make_native("dekhao", native_dekhao), true);
    env_define(g, "shono", make_native("shono", native_shono), true);
    env_define(g, "shono_shonkhya", make_native("shono_shonkhya", native_shono_shonkhya), true);

    env_define(g, "doirgho", make_native("doirgho", native_doirgho), true);
    env_define(g, "shesh_jog", make_native("shesh_jog", native_shesh_jog), true);
    env_define(g, "shesh_bad", make_native("shesh_bad", native_shesh_bad), true);
    env_define(g, "sajao", make_native("sajao", native_sajao), true);
    env_define(g, "ultho", make_native("ultho", native_ultho), true);
    env_define(g, "khojo", make_native("khojo", native_khojo), true);
    env_define(g, "kato", make_native("kato", native_kato), true);
    env_define(g, "bhoro", make_native("bhoro", native_bhoro), true);

    env_define(g, "shonkhya", make_native("shonkhya", native_shonkhya), true);
    env_define(g, "shobdo", make_native("shobdo", native_shobdo), true);
}
