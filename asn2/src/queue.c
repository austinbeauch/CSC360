#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include "p2.h"
#include "queue.h"

/* PQ IMPLIMENTATION FROM
 * https://www.geeksforgeeks.org/priority-queue-using-linked-list/ */

// Function to push according to priority
void push(Node** head, t* data) {

    Node* curr = (*head);
    Node* temp = (Node*)malloc(sizeof(Node));
    temp->data = data;

    if (temp->data->direction == 'E' || temp->data->direction == 'W') {
        temp->priority = 1;
    } else {
        temp->priority = 0;
    }
    
    temp->next = NULL;

    // if the head is null, make the head our new node and return
    if (*head == NULL) {
        *head = temp;
        return;
    }

    // If the new node has higher priority than the head node, or if the head as the same loading time but has a higher train number, make it the new head
    if ((*head)->priority < temp->priority ||
        ((*head)->data->loading_time == temp->data->loading_time && (*head)->data->train_number > temp->data->train_number)) {

        temp->next = *head;
        (*head) = temp;
    } else {
        // list traversal. If priorities are equal, put node at the back
        while (curr->next != NULL &&
               curr->next->priority >= temp->priority) {

            // Check loading time to see if they loaded at the same time if they did, take the lower train number to be first up
            if (curr->next->data->loading_time == temp->data->loading_time &&
                curr->next->data->train_number > temp->data->train_number){

                temp->next = curr->next;
                curr->next = temp;
                return;
            }
            curr = curr->next;
        }
        temp->next = curr->next;
        curr->next = temp;
    }
}

int isEmpty(Node** head) {
    return (*head) == NULL;
}

t* peek(Node** head) {
    return (*head)->data;
}

int peek_priority(Node** head) {
    return (*head)->priority;
}

void pop(Node** head) {
    Node* temp = *head;
    (*head) = (*head)->next;
    free(temp);
}
