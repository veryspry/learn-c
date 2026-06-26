#include <stddef.h>

#ifndef LINKED_LIST_H
#define LINKED_LIST_H

typedef struct DLLNode {
	int data;
	struct DLLNode *next;
	struct DLLNode *prev;
} DLLNode;

typedef struct DLL {
	DLLNode *head;
	DLLNode *tail;
	size_t length;
} DLL;

DLL* dll_new();

DLLNode* dllnode_new();

#endif
