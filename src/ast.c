#include "banglish.h"

Node *node_new(NodeType type, int line) {
    Node *n = calloc(1, sizeof(Node));
    n->type = type;
    n->line = line;
    n->list = NULL;
    n->list_count = 0;
    n->list_capacity = 0;
    n->keys = NULL;
    return n;
}

void node_list_push(Node *n, Node *child) {
    if (n->list_count + 1 > n->list_capacity) {
        int new_cap = n->list_capacity < 4 ? 4 : n->list_capacity * 2;
        n->list = realloc(n->list, sizeof(Node*) * (size_t)new_cap);
        n->list_capacity = new_cap;
    }
    n->list[n->list_count++] = child;
}
