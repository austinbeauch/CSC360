#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>

int main(){
	char str[80];
	char cwd[256];
	
	if (getcwd(cwd, sizeof(cwd)) == NULL)
		perror("getcwd() error");
		
	// prompt generation
	strcpy (str, "SSI: ");
	strcat (str, cwd);
	strcat (str, " > ");
	
	int bailout = 0;
	while (!bailout) {

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
			bailout = 1;
		} else {
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
	printf("Bye\n"); 
}
