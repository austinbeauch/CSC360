#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

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
		char* token = strtok(reply, " ");
		char* tokens[256];
		int index = 0;
		while (token) {
			tokens[index] = token;
			index++;
			token = strtok(NULL, " ");
		}
		tokens[index] = NULL;

		if (!strcmp(reply, "exit")) {
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
				printf("chdir error\n");
			}
		}
		else if(!strcmp(tokens[0], "bg")){
			pid_t p = fork();
			if(p == 0) { // in child process
				execvp(tokens[1], tokens + 1);
			} else if (p > 0) { // in parent process
				waitpid(p, NULL, WNOHANG);
			} else {
				perror("ERROR IN FORK");
				exit(-1);
			}
		}
		else if(!strcmp(tokens[0], "bglist")){

		}
		else {
			pid_t p = fork();
			if(p == 0) { // in child process
				execvp(tokens[0], tokens);
			} else if (p > 0) { // in parent process
				wait(NULL);
			} else {
				perror("ERROR IN FORK");
				exit(-1);
			}

		}
		free(reply);
	}
	exit(0);
}
