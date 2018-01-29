#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "linkedlist.h"

extern int list_length;

void background(char** tokens){
	if (tokens[1] == NULL){
			printf("Enter process to run in background\n");
		} else {
			pid_t p = fork();
			if(p == 0) { // in child process
				execvp(tokens[1], tokens + 1);
				perror("EXECVP ERROR");
				exit(1);
			} else if (p > 0) { // in parent process
				append(p, tokens);
			} else {
				perror("ERROR IN FORK");
			}
		}
}

void create_fork(char** tokens){
	pid_t p = fork();
	if(p == 0) { // in child process
		execvp(tokens[0], tokens);
		perror("EXECVP ERROR");
		exit(1);
	} else if (p > 0) { // in parent process
		waitpid(p, NULL, 0);
	} else {
		perror("ERROR IN FORK");
	}
}

int main(){
	int bailout = 0;
	while (!bailout) {
		char str[256];
		char cwd[256];

		if (getcwd(cwd, sizeof(cwd)) == NULL)
			perror("getcwd() error");

		// prompt generation
		strcpy (str, "SSI: ");
		strcat (str, cwd);
		strcat (str, " > ");
		char* reply = readline(str);

		// check for terminating processes
		if(list_length>0){
			pid_t ter = waitpid(0, NULL, WNOHANG);
			while(ter > 0){  // a process has terminated
				deleting(ter);
				ter = waitpid(0, NULL, WNOHANG);
			}
		}

		// string tokenization
		char* arguments = strtok(reply, " \n");
		char* tokens[256];
		int index = 0;
		while (arguments) {
			tokens[index] = arguments;
			index++;
			arguments = strtok(NULL, " \n");
		}
		tokens[index] = NULL;

		//if the input is nothing, or contains only whitespace
		if(reply[0] == '\0' || (isspace(reply[0]) && tokens[0] == NULL)) {
			// continue;
		}

		else if (!strcmp(tokens[0], "exit")) {
			exit(0);
		}

		else if(!strcmp(tokens[0], "cd")){
			//chdir(getenv("HOME"));

			int x;
			if(tokens[1] == NULL || !strcmp(tokens[1], "~")){
				x = chdir(getenv("HOME"));
			} else {
				x = chdir(tokens[1]);
			}
			if(x == -1){
				perror("CHDIR ERROR\n");
			}
		}

		else if(!strcmp(tokens[0], "bg")){
			background(tokens);
		}
		else if(!strcmp(tokens[0], "bglist")){
			print_list();
		}
		else {
			create_fork(tokens);
		}
		free(reply);
	}
	exit(0);
}
