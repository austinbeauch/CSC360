#ifndef QUEUE_H_
#define QUEUE_H_

#include <unistd.h>
#include "p2.h"

typedef struct node {
    t* data;
    // Higher values indicate higher priority
    int priority;
    struct node* next;
} Node;


void push(Node** head, t* data);
int isEmpty(Node** head);
t* peek(Node** head);
int peek_priority(Node** head);
void pop(Node** head);

#endif
