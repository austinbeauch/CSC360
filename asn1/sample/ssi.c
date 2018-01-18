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
		
	strcpy (str, "SSI: ");
	strcat (str, cwd);
	strcat (str, " > ");

	//const char* prompt = "SSI: " + cwd + " > ";
	
	int bailout = 0;
	while (!bailout) {

		char* reply = readline(str);

		if (!strcmp(reply, "exit")) {
			bailout = 1;
		} else {
			printf("You said: %s\n", reply);
			pid_t p = fork();
			if(p == 0) {
				// execpv()
				printf("Inside child process: %d\n", p);
			}
			else if (p > 0) {
				wait(NULL);
				printf("In parent process; child PID: %d\n", p);
				exit(0);
			}
			else {
				perror("ERROR IN FORK");
			}
			
		}
		free(reply);
	}
	printf("Bye\n"); 
}
