#include <stdlib.h>
#include <stdio.h>
#include "./avl.h"

Node *node_new(int val) {
  Node *new_node;
  new_node = malloc(sizeof(Node));

  if (new_node == NULL) {
    fprintf(stderr, "Failed to allocate %zu bytes", sizeof(Node));
    return NULL;
  }

  new_node->val = val;
  new_node->balance= 0;
  new_node->left = NULL;
  new_node->right = NULL;

  return new_node;
}

void node_free(Node *node) {
  free(node);
}

void tree_free(Node *root) {
  while (root != NULL) {
    // rotate right to get a tree with only one branch.
    if (root->left) {
      Node *left = root->left;
      root->left = left->right;
      left->right = root;
      root = left;
    } else {
      Node *next = root->right;
      node_free(root);
      root = next;
    }
  }
}

// TODO:
//   1. needs to perform rotations
//   3. Need to fix balanace -- currenlty doesn't work correctly
//      (e.g inserting a left child on a node that alreaedy has a right child
//      should not change height)
void insert(Node **root, Node *node) {
  if (node == NULL) {
    return;
  }

  Node **path[64];
  int depth = 0;

  Node **curr = root;

  while (*curr != NULL) {
    path[depth] = curr;
    depth++;

    if (node->val < (*curr)->val) {
      curr = &(*curr)->left;
    } else {
      curr = &(*curr)->right;
    }
  }

  // add new node to tree and link to left/right of parent
  *curr = node;

  while (depth > 0) {
    depth--;
    Node **slot = path[depth];
    Node *n = *slot;

    if (node->val < n->val) {
      n->balance--;
    } else {
      n->balance++;
    }

    if (n->balance == 0) {
      // tree is in acceeptable balance range
      break;
    }

    if (n->balance == 1 || n->balance == -1) {
      // tree is in acceeptable balance range
      continue;
    }

    if (n->balance == -2) {
      rotate_right(slot);
    } else {
      rotate_left(slot);
    }

    break;
  }
}

// void delete(Node **root, Node *node) {
//   // TODO
// }

void rotate_right(Node **root) {
  Node *a = *root;
  Node *b = a->right;

  a->left = b->right;
  b->right = a;

  *root = b;

  a->balance = a->balance + 1 - (b->balance < 0 ? b->balance: 0);
  b->balance = b->balance + 1 + (a->balance > 0 ? a->balance: 0);
}

void rotate_left(Node **root) {
  Node *a = *root;
  Node *b = a->left;

  a->right = b->left;
  b->left = a;
  *root = b;

  a->balance = a->balance - 1 - (b->balance > 0 ? b->balance : 0);
  b->balance = b->balance - 1 + (a->balance < 0 ? a->balance : 0);
}

