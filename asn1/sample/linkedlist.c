#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


typedef struct bg_pro{
	pid_t pid;
	char command[1024];
	struct bg_pro* next;
} node;

node* head = NULL;
int list_length = 0;
 
void append(pid_t p, char** command){
	node* new = (node*)malloc(sizeof(node));
	new->pid = p;
	int i = 0;
	while(command[i] != NULL){
		strcat(new->command, command[i]);
		strcat(new->command, " ");
		i++;
	}
	
	if(list_length == 0){
		head = new;
	} else{
		node* curr = head;
		while(head->next != NULL){
			curr = curr->next;
		}
		curr->next = new;
	}
}