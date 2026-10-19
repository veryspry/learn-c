#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <CUnit/Basic.h>
#include <CUnit/CUnit.h>
#include "./avl.h"

void test_add_one_root_node(void) {
  Node *root = NULL;
  insert(&root, node_new(1));

  CU_ASSERT_EQUAL(root->val, 1);
  CU_ASSERT_PTR_NULL(root->left);
  CU_ASSERT_PTR_NULL(root->right);

  tree_free(root);
}

void test_inserts_left_and_right_nodes(void) {
  Node *root = NULL;

  insert(&root, node_new(2));
  insert(&root, node_new(1));
  insert(&root, node_new(3));

  CU_ASSERT_EQUAL(root->val, 2);
  CU_ASSERT_PTR_NOT_NULL(root->left);
  CU_ASSERT_PTR_NOT_NULL(root->right);
  CU_ASSERT_EQUAL(root->left->val, 1);
  CU_ASSERT_EQUAL(root->right->val, 3);

  tree_free(root);
}

void test_simple_balance(void) {
  Node *root = NULL;
  
  Node *a = node_new(3);
  Node *b = node_new(2);
  Node *c = node_new(1);

  insert(&root, a);
  insert(&root, b);
  insert(&root, c);

  CU_ASSERT_PTR_NOT_NULL(b->left);
  CU_ASSERT_PTR_NOT_NULL(b->right);

  CU_ASSERT_PTR_NULL(a->left);
  CU_ASSERT_PTR_NULL(a->right);

  CU_ASSERT_PTR_NULL(b->left);
  CU_ASSERT_PTR_NULL(b->right);

  CU_ASSERT_EQUAL(root->val, 2);
  CU_ASSERT_EQUAL(root->left->val, 1);
  CU_ASSERT_EQUAL(root->right->val, 3);


  // TODO: tree_free() once it returns top node
  node_free(a);
  node_free(b);
  node_free(c);
}

int main() {
  if (CU_initialize_registry() != CUE_SUCCESS) {
    return CU_get_error();
  }

  CU_pSuite suite = CU_add_suite("AVL", NULL, NULL);

  CU_add_test(suite, "adds node to empty tree", test_add_one_root_node);
  CU_add_test(suite, "inserts nodes left and right", test_inserts_left_and_right_nodes);
  CU_add_test(suite, "balances simple tree", test_simple_balance);

  CU_basic_set_mode(CU_BRM_VERBOSE);
  CU_basic_run_tests();
  CU_cleanup_registry();

  return CU_get_error();
}

