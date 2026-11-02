#include <stdint.h>

/* A walk over a tree of nodes: the stack use depends on the data. */
struct node {
    struct node *left;
    struct node *right;
    int16_t value;
};

int32_t sum_tree(const struct node *n)
{
    if (n == 0) {
        return 0;
    }
    return n->value + sum_tree(n->left) + sum_tree(n->right);
}

int32_t sum_array(const int16_t *a, unsigned count)
{
    int16_t scratch[count];
    int32_t total = 0;
    for (unsigned i = 0; i < count; i++) {
        scratch[i] = a[i];
        total += scratch[i];
    }
    return total;
}
