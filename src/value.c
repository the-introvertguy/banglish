#include "banglish.h"

/* ============================================================
 *  Global tracking list (simple arena/sweep allocator)
 * ============================================================ */
static Obj *g_obj_head = NULL;

void obj_track(Obj *o) {
    o->next = g_obj_head;
    g_obj_head = o;
}

static void free_map_entries(ObjMap *m) {
    for (int i = 0; i < m->capacity; i++) {
        if (m->entries[i].used && !m->entries[i].tombstone) {
            free(m->entries[i].key);
        }
    }
    free(m->entries);
}

void obj_sweep_all(void) {
    Obj *cur = g_obj_head;
    while (cur) {
        Obj *next = cur->next;
        switch (cur->type) {
            case OBJ_STRING: {
                ObjString *s = (ObjString*)cur;
                free(s->chars);
                free(s);
                break;
            }
            case OBJ_ARRAY: {
                ObjArray *a = (ObjArray*)cur;
                free(a->items);
                free(a);
                break;
            }
            case OBJ_MAP: {
                ObjMap *m = (ObjMap*)cur;
                free_map_entries(m);
                free(m);
                break;
            }
            case OBJ_FUNCTION: {
                ObjFunction *f = (ObjFunction*)cur;
                free(f->name);
                for (int i = 0; i < f->param_count; i++) free(f->params[i]);
                free(f->params);
                free(f);
                break;
            }
            case OBJ_NATIVE: {
                ObjNative *n = (ObjNative*)cur;
                free(n->name);
                free(n);
                break;
            }
        }
        cur = next;
    }
    g_obj_head = NULL;
}

/* ============================================================
 *  Value constructors
 * ============================================================ */
Value make_number(double n) {
    Value v; v.type = VAL_NUMBER; v.as.number = n; return v;
}
Value make_bool(bool b) {
    Value v; v.type = VAL_BOOL; v.as.boolean = b; return v;
}
Value make_null(void) {
    Value v; v.type = VAL_NULL; v.as.number = 0; return v;
}

Value make_string(const char *chars, int length) {
    ObjString *s = malloc(sizeof(ObjString));
    s->obj.type = OBJ_STRING;
    s->chars = malloc((size_t)length + 1);
    memcpy(s->chars, chars, (size_t)length);
    s->chars[length] = '\0';
    s->length = length;
    obj_track((Obj*)s);
    Value v; v.type = VAL_STRING; v.as.obj = (Obj*)s;
    return v;
}

Value make_string_take(char *chars, int length) {
    ObjString *s = malloc(sizeof(ObjString));
    s->obj.type = OBJ_STRING;
    s->chars = chars;
    s->length = length;
    obj_track((Obj*)s);
    Value v; v.type = VAL_STRING; v.as.obj = (Obj*)s;
    return v;
}

Value make_array(void) {
    ObjArray *a = malloc(sizeof(ObjArray));
    a->obj.type = OBJ_ARRAY;
    a->items = NULL;
    a->count = 0;
    a->capacity = 0;
    obj_track((Obj*)a);
    Value v; v.type = VAL_ARRAY; v.as.obj = (Obj*)a;
    return v;
}

Value make_map(void) {
    ObjMap *m = malloc(sizeof(ObjMap));
    m->obj.type = OBJ_MAP;
    m->entries = NULL;
    m->capacity = 0;
    m->count = 0;
    obj_track((Obj*)m);
    Value v; v.type = VAL_MAP; v.as.obj = (Obj*)m;
    return v;
}

Value make_function(char *name, char **params, int param_count, Node *body, Env *closure) {
    ObjFunction *f = malloc(sizeof(ObjFunction));
    f->obj.type = OBJ_FUNCTION;
    f->name = name;
    f->params = params;
    f->param_count = param_count;
    f->body = body;
    f->closure = closure;
    obj_track((Obj*)f);
    Value v; v.type = VAL_FUNCTION; v.as.obj = (Obj*)f;
    return v;
}

Value make_native(const char *name, NativeFn fn) {
    ObjNative *n = malloc(sizeof(ObjNative));
    n->obj.type = OBJ_NATIVE;
    n->name = strdup(name);
    n->fn = fn;
    obj_track((Obj*)n);
    Value v; v.type = VAL_NATIVE_FUNC; v.as.obj = (Obj*)n;
    return v;
}

/* ============================================================
 *  Array operations (amortized-growth dynamic array)
 * ============================================================ */
static void array_grow(ObjArray *arr, int needed) {
    if (arr->capacity >= needed) return;
    int new_cap = arr->capacity < 8 ? 8 : arr->capacity * 2;
    while (new_cap < needed) new_cap *= 2;
    arr->items = realloc(arr->items, sizeof(Value) * (size_t)new_cap);
    arr->capacity = new_cap;
}

void array_push(ObjArray *arr, Value v) {
    array_grow(arr, arr->count + 1);
    arr->items[arr->count++] = v;
}

Value array_pop(ObjArray *arr) {
    if (arr->count == 0) return make_null();
    return arr->items[--arr->count];
}

Value array_get(ObjArray *arr, int index) {
    if (index < 0 || index >= arr->count) return make_null();
    return arr->items[index];
}

void array_set(ObjArray *arr, int index, Value v) {
    if (index < 0) return;
    if (index >= arr->capacity) array_grow(arr, index + 1);
    if (index >= arr->count) {
        for (int i = arr->count; i < index; i++) arr->items[i] = make_null();
        arr->count = index + 1;
    }
    arr->items[index] = v;
}

/* ============================================================
 *  Map operations (open addressing, linear probing, string keys)
 * ============================================================ */
static unsigned long hash_string(const char *s) {
    unsigned long h = 2166136261u;
    while (*s) {
        h ^= (unsigned char)(*s++);
        h *= 16777619u;
    }
    return h;
}

static void map_grow(ObjMap *m);

static MapEntry *map_find_entry(MapEntry *entries, int capacity, const char *key) {
    unsigned long idx = hash_string(key) % (unsigned long)capacity;
    MapEntry *tombstone = NULL;
    for (;;) {
        MapEntry *e = &entries[idx];
        if (!e->used) {
            return tombstone ? tombstone : e;
        } else if (e->tombstone) {
            if (!tombstone) tombstone = e;
        } else if (strcmp(e->key, key) == 0) {
            return e;
        }
        idx = (idx + 1) % (unsigned long)capacity;
    }
}

static void map_grow(ObjMap *m) {
    int new_cap = m->capacity < 8 ? 8 : m->capacity * 2;
    MapEntry *new_entries = calloc((size_t)new_cap, sizeof(MapEntry));
    int new_count = 0;
    for (int i = 0; i < m->capacity; i++) {
        MapEntry *e = &m->entries[i];
        if (e->used && !e->tombstone) {
            MapEntry *dest = map_find_entry(new_entries, new_cap, e->key);
            dest->key = e->key;
            dest->value = e->value;
            dest->used = true;
            dest->tombstone = false;
            new_count++;
        }
    }
    free(m->entries);
    m->entries = new_entries;
    m->capacity = new_cap;
    m->count = new_count;
}

void map_set(ObjMap *map, const char *key, Value v) {
    if ((map->count + 1) > (map->capacity * 3) / 4) {
        map_grow(map);
    }
    MapEntry *e = map_find_entry(map->entries, map->capacity, key);
    bool is_new = !e->used;
    if (is_new) {
        e->key = strdup(key);
        map->count++;
    }
    e->value = v;
    e->used = true;
    e->tombstone = false;
}

bool map_get(ObjMap *map, const char *key, Value *out) {
    if (map->capacity == 0) return false;
    MapEntry *e = map_find_entry(map->entries, map->capacity, key);
    if (!e->used || e->tombstone) return false;
    *out = e->value;
    return true;
}

bool map_delete(ObjMap *map, const char *key) {
    if (map->capacity == 0) return false;
    MapEntry *e = map_find_entry(map->entries, map->capacity, key);
    if (!e->used || e->tombstone) return false;
    free(e->key);
    e->key = NULL;
    e->tombstone = true;
    return true;
}

/* ============================================================
 *  Value utilities
 * ============================================================ */
bool values_equal(Value a, Value b) {
    if (a.type != b.type) {
        /* allow number-ish comparisons only strictly of same type */
        return false;
    }
    switch (a.type) {
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_BOOL:   return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NULL:   return true;
        case VAL_STRING: {
            ObjString *sa = AS_STRING(a), *sb = AS_STRING(b);
            if (sa->length != sb->length) return false;
            return memcmp(sa->chars, sb->chars, (size_t)sa->length) == 0;
        }
        case VAL_ARRAY:  return AS_OBJ(a) == AS_OBJ(b);
        case VAL_MAP:    return AS_OBJ(a) == AS_OBJ(b);
        case VAL_FUNCTION: return AS_OBJ(a) == AS_OBJ(b);
        case VAL_NATIVE_FUNC: return AS_OBJ(a) == AS_OBJ(b);
    }
    return false;
}

bool value_truthy(Value v) {
    switch (v.type) {
        case VAL_NULL: return false;
        case VAL_BOOL: return AS_BOOL(v);
        case VAL_NUMBER: return AS_NUMBER(v) != 0.0;
        case VAL_STRING: return AS_STRING(v)->length > 0;
        default: return true;
    }
}

static void print_number(double n) {
    if (n == (long long)n && fabs(n) < 1e15) {
        printf("%lld", (long long)n);
    } else {
        printf("%g", n);
    }
}

void value_print(Value v) {
    switch (v.type) {
        case VAL_NUMBER: print_number(AS_NUMBER(v)); break;
        case VAL_BOOL: printf(AS_BOOL(v) ? "shotto" : "mitha"); break;
        case VAL_NULL: printf("kichu_na"); break;
        case VAL_STRING: printf("%s", AS_CSTRING(v)); break;
        case VAL_ARRAY: {
            ObjArray *a = AS_ARRAY(v);
            printf("[");
            for (int i = 0; i < a->count; i++) {
                if (a->items[i].type == VAL_STRING) {
                    printf("\"%s\"", AS_CSTRING(a->items[i]));
                } else {
                    value_print(a->items[i]);
                }
                if (i < a->count - 1) printf(", ");
            }
            printf("]");
            break;
        }
        case VAL_MAP: {
            ObjMap *m = AS_MAP(v);
            printf("{");
            int printed = 0;
            for (int i = 0; i < m->capacity; i++) {
                MapEntry *e = &m->entries[i];
                if (e->used && !e->tombstone) {
                    if (printed > 0) printf(", ");
                    printf("\"%s\": ", e->key);
                    if (e->value.type == VAL_STRING) {
                        printf("\"%s\"", AS_CSTRING(e->value));
                    } else {
                        value_print(e->value);
                    }
                    printed++;
                }
            }
            printf("}");
            break;
        }
        case VAL_FUNCTION: {
            ObjFunction *f = AS_FUNCTION(v);
            printf("<kaj %s>", f->name ? f->name : "anonymous");
            break;
        }
        case VAL_NATIVE_FUNC: {
            ObjNative *n = AS_NATIVE(v);
            printf("<native %s>", n->name);
            break;
        }
    }
}

const char *value_type_name(Value v) {
    switch (v.type) {
        case VAL_NUMBER: return "shonkhya";
        case VAL_STRING: return "shobdo";
        case VAL_BOOL: return "boolean";
        case VAL_NULL: return "kichu_na";
        case VAL_ARRAY: return "array";
        case VAL_MAP: return "map";
        case VAL_FUNCTION: return "kaj";
        case VAL_NATIVE_FUNC: return "native_kaj";
    }
    return "unknown";
}

char *value_to_string(Value v) {
    char buf[64];
    switch (v.type) {
        case VAL_NUMBER: {
            double n = AS_NUMBER(v);
            if (n == (long long)n && fabs(n) < 1e15) {
                snprintf(buf, sizeof(buf), "%lld", (long long)n);
            } else {
                snprintf(buf, sizeof(buf), "%g", n);
            }
            return strdup(buf);
        }
        case VAL_BOOL: return strdup(AS_BOOL(v) ? "shotto" : "mitha");
        case VAL_NULL: return strdup("kichu_na");
        case VAL_STRING: return strdup(AS_CSTRING(v));
        default: {
            /* fallback: capture via snprintf-less simple repr */
            return strdup(value_type_name(v));
        }
    }
}
