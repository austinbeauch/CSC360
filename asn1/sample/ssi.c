#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "linkedlist.h"

int main(){
	int bailout = 0;
	while (!bailout) {
		char str[80];
		char cwd[256];

		if (getcwd(cwd, sizeof(cwd)) == NULL)
			perror("getcwd() error");

		// prompt generation
		strcpy (str, "SSI: ");
		strcat (str, cwd);
		strcat (str, " > ");
		char* reply = readline(str);

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
			continue;
		}

		else if (!strcmp(tokens[0], "exit")) {
			exit(0);
		}

		else if(!strcmp(tokens[0], "cd")){
			int x;
			if(tokens[1] == NULL || !strcmp(tokens[1], "~")){
				int x = chdir(getenv("HOME"));
			} else {
				int x = chdir(tokens[1]);
			}
			if(x == -1){
				perror("CHDIR ERROR\n");
			}
		}
		else if(!strcmp(tokens[0], "bg")){
			pid_t p = fork();
			if(p == 0) { // in child process
				execvp(tokens[1], tokens + 1);
				perror("EXECVP ERROR");
				exit(1);
			} else if (p > 0) { // in parent process
				append(p, tokens);
				waitpid(p, NULL, WNOHANG);
			} else {
				perror("ERROR IN FORK");
				exit(-1);
			}
		}
		else if(!strcmp(tokens[0], "bglist")){
			print_list();
		}
		else {
			pid_t p = fork();
			if(p == 0) { // in child process
				execvp(tokens[0], tokens);
				perror("EXECVP ERROR");
				exit(1);
			} else if (p > 0) { // in parent process
				waitpid(p, NULL, 0);
			} else {
				perror("ERROR IN FORK");
				// exit(-1);
			}

		}
		free(reply);
	}
	exit(0);
}
