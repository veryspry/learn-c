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

  Node **curr = root;

  while (*curr != NULL) {
    if (node->val < (*curr)->val) {
      curr = &(*curr)->left;
    } else {
      curr = &(*curr)->right;
    }
  }

  *curr = node;
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

