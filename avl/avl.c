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
//   1. should return new top level root of entire tree
//   2. needs to perform rotations
void insert(Node **root, Node *node) {
  if (node == NULL) {
    return;
  }

  while (*root != NULL) {
    if (node->val < (*root)->val) {
      (*root)->balance--;
      root = &(*root)->left;
    } else {
      (*root)->balance++;
      root = &(*root)-> right;
    }
  }

  *root = node;
}

// void delete(Node **root, Node *node) {
//   // TODO
// }

void rotate_right() {

}

void rotate_left() {

}

