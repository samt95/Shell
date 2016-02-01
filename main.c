/*
 * main.c
 *
 * Shell Program
 *
 * Sam Taylor
 * 31-01-2016 
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <errno.h>


// background process struct
typedef struct {
	char *name;
	pid_t pid;
	char status;
} bgProcess;


/* removeFromBGList(terminatedProcessNum, bglist, bglistSize)
 *
 * frees allocated space of terminated process
 *
 */
void removeFromBGList(int terminatedProcessNum, bgProcess *bglist[], int *bglistSize) {
	printf("Terminated process %d: %s %X\n", terminatedProcessNum, bglist[terminatedProcessNum]->name, bglist[terminatedProcessNum]->pid);

	free(bglist[terminatedProcessNum]->name);
	free(bglist[terminatedProcessNum]);
	(*bglistSize)--;

	// shift the array if a bgProcess was removed that was not the last one in the list
	if(terminatedProcessNum != *bglistSize) { 
		int k;
		for(k = terminatedProcessNum; k < *bglistSize; k++) {
			bglist[k] = bglist[k+1];
		}
		bglist[*bglistSize] = NULL;
	}
}


/* freeTerminatedBGProcesses(bglist, bglistSize)
 *
 * check if any background process has terminated
 *
 */
void freeTerminatedBGProcesses(bgProcess *bglist[], int *bglistSize) {
	int i;
	for(i = 0; i < *bglistSize; i++) {
		int status;
		int r = waitpid(bglist[i]->pid, &status, WNOHANG);

		if(r != 0) {
			removeFromBGList(i, bglist, bglistSize);
		}	
	}
}


/* changeDirectory(numArgs, args, cwd)
 *
 */
void changeDirectory(int *numArgs, char *args[]) {
 	int returnVal = 0;

	if(*numArgs == 1 || strcmp(args[1], "~") == 0) {
		returnVal = chdir(getenv("HOME"));
	}
	else {
		returnVal = chdir(args[1]);
	}

	// Check if directory change was successful
	if(returnVal != 0) {
		if(errno == EACCES){
			printf("Permission Denied\n");
		}
		else {
			printf("Not a valid directory\n");
		}
		
	}
}


/* executeBGProcess(args, numArgs, bglist, bglistSize)
 *
 * create a new process to execute external program in background
 * parent process does not wait
 *
 */
int executeBGProcess(char *args[], int *numArgs, bgProcess *bglist[], int *bglistSize) {
	if(*numArgs == 1) {
		printf("bg requires an argument to run a program in the background\n");
	}
	else if(*bglistSize == 5) {
		printf("maximum number of background processes already running.\n");
	}
	else {
		pid_t child_pid = fork();

		char ** bgArgs = &args[1];

		if(child_pid == -1) { 
			perror("Fork failed\n");
		}
	 	else if(child_pid == 0) { // in child process
	 		execvp(bgArgs[0], bgArgs);
	 		// only reach here if execvp fails
	 		printf("Command not recognized\n"); 
	 		return -1;
	 	}
	 	else {			 		
		 	bglist[*bglistSize] = (bgProcess *)malloc(sizeof(bgProcess));
		 	if(bglist[*bglistSize] == NULL) {
		 		printf("Failed to allocated space for bglist[%d]", *bglistSize);
		 		return -1;
		 	}

		 	bglist[*bglistSize]->pid = child_pid;
		 	bglist[*bglistSize]->status = 'R';
		 	bglist[*bglistSize]->name = (char *)malloc(strlen(bgArgs[0]));
			if(bglist[*bglistSize]->name == NULL) {
		 		printf("Failed to allocated space for bglist[%d]->name", *bglistSize);
		 		return -1;
		 	}

		 	strcpy(bglist[*bglistSize]->name, bgArgs[0]);

			(*bglistSize)++;				 	
	 	}	
	}
	return 0;
}


/* listBGProcesses(bglist, bglistSize)
 *
 * output a list of running background processes
 *
 */
void listBGProcesses(bgProcess *bglist[], int *bglistSize) {
	if(*bglistSize == 0) {
		printf("No background jobs running\n");
		return;
	}
	int i;
	for(i = 0; i < *bglistSize; i++) {
		printf("%d [%c]: %s %X\n", i, bglist[i]->status, bglist[i]->name, bglist[i]->pid);
	}	
	printf("Total Background jobs: %d\n", *bglistSize);
}


/* BGProcessAction(args, numArgs, bglist, bglistSize, commandName, commandAction, SIG)
 *
 * terminates, resumes, or stops (suspends) a process currently running in the backgroudn
 * action depends on input paramenter SIG (SIGTERM, SIGSTOP, or SIGCONT)
 *
 */
void BGProcessAction(char *args[], int *numArgs, bgProcess *bglist[], int *bglistSize, char *commandName, char *commandAction, int SIG) {
	
	// VALIDATION
	if(*numArgs == 1) {
		printf("Usage: %s [processNum]\n", commandName);
		return;
	}
	int processNum;
	if(strcmp(args[1], "0") == 0){ // have to do this check b/c strtol() returns 0 on failure
		processNum = 0;
	}
	else {
		processNum = (int)strtol(args[1], NULL, 10);
		if(processNum == 0) { //failure
			printf("Usage: %s [processNum]\n", commandName);
			return;
		}
	}

	if(processNum >= *bglistSize || processNum < 0) {
		printf("No background process with processNum [%d] running\n", processNum);
		return;
	}

	// DONE VALIDATION - now perform action

	int returnVal = kill(bglist[processNum]->pid, SIG);
	if(returnVal != 0) {
		printf("%s failed\n", commandName);
	}
	else if(SIG == SIGTERM) {
		removeFromBGList(processNum, bglist, bglistSize);
	}
	else if(SIG == SIGSTOP) {
		bglist[processNum]->status = 'S';
	}
	else if(SIG == SIGCONT) {
		bglist[processNum]->status = 'R';
	}
}


/* executeProcess(args)
 *
 * create a new process to execute external program
 * parent process waits for child process to finish execution
 *
 */
int executeProcess(char *args[]) {
 	pid_t child_pid = fork();

 	if(child_pid == -1) {
 		perror("Fork failed\n");
 	}
 	else if(child_pid == 0) { // in child process
 		execvp(args[0], args);
		// only reach here if execvp fails
 		printf("Command not recognized\n");
 		return -1;
 	}
 	else {
 		int status;
 		waitpid(child_pid, &status, 0); // wait for child process to finish
 	}
 	return 0;
}


/* getInputArgs(input, output)
 *
 * parse input string to output array of strings
 *
 * returns the number of args read
 */
int getInputArgs(char *input, char *output[]) {
	char *token = strtok(input, " ");
	int index = 0;
 	while(token) {
 		output[index] = (char *)malloc( strlen(token) );
 		if(output[index] == NULL) {
	 		printf("Failed to allocated space for output[%d]", index);
	 	}
 		strcpy(output[index], token);

 		token = strtok(NULL, " ");
 		index++;
 		if(index == 15) { // max capacity
 			break;
 		}
 	}
 	output[index] = NULL; // args array is null terminated 
	return index;
}


int main (void) {

	// list of background process structs
	// maximum 5 background processes
	bgProcess *bglist[5]; 
	int bglistSize = 0;

	for (;;) {
		char buffer[PATH_MAX + 1];
		char *cwd = getcwd(buffer, PATH_MAX + 1);
		char *cmd = readline(strcat(cwd, ">"));

	 	// max 16 = command + max 14 command args + null terminator
	 	char *args[16]; 

	 	int numArgs = getInputArgs(cmd, args);

	 	if(numArgs == 0) {
	 		// do nothing
	 	}
	 	else if(strcmp(args[0], "quit") == 0) {
	 		return 0;
	 	}
	 	else if(strcmp(args[0], "pwd") == 0) {
	 		printf("%s\n", getcwd(buffer, PATH_MAX + 1));
	 	}
	 	else if(strcmp(args[0], "cd") == 0) {
	 		changeDirectory(&numArgs, args);
	 	}
	 	else if(strcmp(args[0], "bg") == 0) {
			int returnVal = executeBGProcess(args, &numArgs, bglist, &bglistSize);
			if(returnVal != 0) return -1;
	 	}
	 	else if(strcmp(args[0], "bgkill") == 0) {
	 		BGProcessAction(args, &numArgs, bglist, &bglistSize, "kill", "killing", SIGTERM);
	 	}
	 	else if(strcmp(args[0], "bglist") == 0) {
	 		listBGProcesses(bglist,  &bglistSize);
	 	}
	 	else if(strcmp(args[0], "stop") == 0) {
	 		BGProcessAction(args, &numArgs, bglist, &bglistSize, "stop", "stopping", SIGSTOP);
	 	}
	 	else if(strcmp(args[0], "start") == 0) {
	 		BGProcessAction(args, &numArgs, bglist, &bglistSize, "resume", "resuming", SIGCONT);
	 	}
	 	else {
	 		int returnVal = executeProcess(args);
	 		if(returnVal != 0) return -1;
		}

		freeTerminatedBGProcesses(bglist, &bglistSize);	 	
		free(cmd);
		int i;
		for(i = 0; i < numArgs; i++) {
			free(args[i]);
		}
	}
}



