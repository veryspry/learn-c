
typedef struct Node Node;

struct Node {
  int val;
  int balance;
  Node *left;
  Node *right;
};

Node *node_new(int val);

void node_free(Node *node);

void tree_free(Node *root);

void insert(Node **root, Node *node);

void rotate_right();

void rotate_left();

