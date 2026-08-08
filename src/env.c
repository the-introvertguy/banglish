#include "banglish.h"

static unsigned long hash_str(const char *s) {
    unsigned long h = 2166136261u;
    while (*s) { h ^= (unsigned char)(*s++); h *= 16777619u; }
    return h;
}

Env *env_new(Env *parent) {
    Env *e = malloc(sizeof(Env));
    e->entries = NULL;
    e->capacity = 0;
    e->count = 0;
    e->parent = parent;
    return e;
}

static EnvEntry *env_find_slot(EnvEntry *entries, int capacity, const char *name) {
    unsigned long idx = hash_str(name) % (unsigned long)capacity;
    EnvEntry *tombstone = NULL;
    for (;;) {
        EnvEntry *e = &entries[idx];
        if (!e->used) {
            return tombstone ? tombstone : e;
        } else if (e->tombstone) {
            if (!tombstone) tombstone = e;
        } else if (strcmp(e->name, name) == 0) {
            return e;
        }
        idx = (idx + 1) % (unsigned long)capacity;
    }
}

static void env_grow(Env *env) {
    int new_cap = env->capacity < 8 ? 8 : env->capacity * 2;
    EnvEntry *new_entries = calloc((size_t)new_cap, sizeof(EnvEntry));
    for (int i = 0; i < env->capacity; i++) {
        EnvEntry *e = &env->entries[i];
        if (e->used && !e->tombstone) {
            EnvEntry *dest = env_find_slot(new_entries, new_cap, e->name);
            *dest = *e;
        }
    }
    free(env->entries);
    env->entries = new_entries;
    env->capacity = new_cap;
}

bool env_define(Env *env, const char *name, Value v, bool is_const) {
    if ((env->count + 1) > (env->capacity * 3) / 4) {
        env_grow(env);
    }
    EnvEntry *e = env_find_slot(env->entries, env->capacity, name);
    bool is_new = !e->used || e->tombstone;
    if (is_new) {
        e->name = strdup(name);
        env->count++;
    }
    e->value = v;
    e->is_const = is_const;
    e->used = true;
    e->tombstone = false;
    return true;
}

bool env_assign(Env *env, const char *name, Value v) {
    Env *cur = env;
    while (cur) {
        if (cur->capacity > 0) {
            EnvEntry *e = env_find_slot(cur->entries, cur->capacity, name);
            if (e->used && !e->tombstone) {
                if (e->is_const) return false; /* cannot reassign const */
                e->value = v;
                return true;
            }
        }
        cur = cur->parent;
    }
    return false; /* undefined variable */
}

bool env_get(Env *env, const char *name, Value *out) {
    Env *cur = env;
    while (cur) {
        if (cur->capacity > 0) {
            EnvEntry *e = env_find_slot(cur->entries, cur->capacity, name);
            if (e->used && !e->tombstone) {
                *out = e->value;
                return true;
            }
        }
        cur = cur->parent;
    }
    return false;
}

void env_free_shallow(Env *env) {
    if (!env) return;
    for (int i = 0; i < env->capacity; i++) {
        if (env->entries[i].used && !env->entries[i].tombstone) {
            free(env->entries[i].name);
        }
    }
    free(env->entries);
    free(env);
}
