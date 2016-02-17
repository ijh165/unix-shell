#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/types.h>
//definitions
#define COMMAND_LENGTH 1024
#define NUM_TOKENS (COMMAND_LENGTH / 2+1)
#define HISTORY_DEPTH 10
//global history str arr
int cmd_count;
char* history[HISTORY_DEPTH];
//split the buff into tokens
int tokenize_command(char* buff, char* tokens[])
{
int i = 0;
char* token = strtok(buff, " \t\r\n\a");
while(token != NULL)
{
tokens[i] = token;
i++;
token = strtok(NULL, " \t\r\n\a");
}
tokens[i] = NULL;
return i;
}


//update the history str arr
void update_history(char** history, char* buff)
{
	int index = cmd_count % HISTORY_DEPTH;
	printf("%d\n", index);
	

	if(cmd_count >= 10) {
		printf("free time!\n");
		
		//free(history[index]);
		//history[index] = malloc(sizeof(char)*COMMAND_LENGTH);

	}
	
	//strcpy(history[index], buff);
	(history[index]) = strdup(buff);
	//printf("history: %s index: %d\n", history[index], index);
	cmd_count++;
}







//prints the history
void print_history()
{
if(cmd_count<=10)
for(int i = 0; i<cmd_count; i++)
{
char* num_str = malloc(16);
snprintf(num_str, 16, "%d", i+1);
write(STDOUT_FILENO, num_str, strlen(num_str));
write(STDOUT_FILENO, ". ", strlen(". "));
write(STDOUT_FILENO, history[i], strlen(history[i]));
write(STDOUT_FILENO, "\n", strlen("\n"));
}
else
for(int i = cmd_count-10; i<cmd_count; i++)
{
char* num_str = malloc(16);
snprintf(num_str, 16, "%d", i+1);
write(STDOUT_FILENO, num_str, strlen(num_str));
write(STDOUT_FILENO, ". ", strlen(". "));
//printf("WERWERWEWER: %d sdfsdfsdfsd: %s\n", i%HISTORY_DEPTH, history[i]);
write(STDOUT_FILENO, history[i % HISTORY_DEPTH], strlen(history[i % HISTORY_DEPTH]));
write(STDOUT_FILENO, "\n", strlen("\n"));
}
}
/**
* Read a command from the keyboard into the buffer 'buff' and tokenize it
* such that 'tokens[i]' points into 'buff' to the i'th token in the command.
* buff: Buffer allocated by the calling code. Must be at least
* COMMAND_LENGTH bytes long.
* tokens[]: Array of character pointers which point into 'buff'. Must be at
* least NUM_TOKENS long. Will strip out up to one final '&' token.
* 'tokens' will be NULL terminated.
* in_background: pointer to a boolean variable. Set to true if user entered
* an & as their last token; otherwise set to false.
* internal_cmd: pointer to a boolean variable. Set to true if user entered
* internal commands.
*/
void read_command(char* buff, char* tokens[], _Bool* in_background, _Bool* internal_cmd)
{
*in_background = false;
*internal_cmd = false;
// Read input
int length = read(STDIN_FILENO, buff, COMMAND_LENGTH-1);
if ( (length < 0) && (errno !=EINTR) )
{
perror("Unable to read command. Terminating.\n");
exit(-1); /* terminate with error */
}
// Null terminate and strip \n.
buff[length] = '\0';
if (buff[strlen(buff) - 1] == '\n') {
buff[strlen(buff) - 1] = '\0';
}
if(strlen(buff) != '\0')
update_history(history, buff);
// Tokenize (saving original command string)
int token_count = tokenize_command(buff, tokens);
if (token_count == 0) {
return;
}
// internal commands
if (strcmp(tokens[0], "exit") == 0) {
*internal_cmd = true;
write(STDOUT_FILENO, "exiting shell\n", strlen("exiting shell\n"));
exit(0);
}
else if (strcmp(tokens[0], "cwd") == 0) {
*internal_cmd = true;
char cwd[COMMAND_LENGTH];
if (getcwd(cwd, sizeof(cwd)) != NULL) {
write(STDOUT_FILENO, cwd, strlen(cwd));
write(STDOUT_FILENO, "\n", strlen("\n"));
}
else {
perror("getcwd() error\n");
}
return;
}
else if (strcmp(tokens[0], "cd") == 0) {
*internal_cmd = true;
if (chdir(tokens[1]) != 0) {
perror("SHELL ERROR");
}
}
else if (strcmp(tokens[0], "history") == 0) {
*internal_cmd = true;
print_history();
}
// Extract if running in background:
if (token_count > 0 && strcmp(tokens[token_count - 1], "&") == 0) {
*in_background = true;
tokens[token_count - 1] = 0;
}
}
/**
* Main and Execute Commands
*/
int main(int argc, char* argv[])
{
char input_buffer[COMMAND_LENGTH];
char* tokens[NUM_TOKENS];
cmd_count = 0;
while (true)
{
// Get command
// Use write because we need to use read()/write() to work with
// signals, and they are incompatible with printf().
char cwd[COMMAND_LENGTH];
if (getcwd(cwd, sizeof(cwd)) != NULL) {
write(STDOUT_FILENO, cwd, strlen(cwd));
}
else {
perror("getcwd() error");
}
write(STDOUT_FILENO, "> ", strlen( "> "));
_Bool in_background = false, internal_cmd = false;
read_command(input_buffer, tokens, &in_background, &internal_cmd);
/**
* Steps For Basic Shell:
* 1. Fork a child process
* 2. Child process invokes execvp() using results in token array.
* 3. If in_background is false, parent waits for
* child to finish. Otherwise, parent loops back to
* read_command() again immediately.
*/
if(!internal_cmd) {
int stat_val;
pid_t pid = fork();
if (pid < 0) {
perror("SHELL ERROR (FORKING FAIL)");
}
else if (pid == 0) {
if (execvp(tokens[0], tokens) == -1) {
perror("SHELL ERROR");
}
return 0;
}
else if(!in_background) {
do {
pid_t wait_pid = waitpid(pid, &stat_val, WUNTRACED);
} while (!WIFEXITED(stat_val) && !WIFSIGNALED(stat_val));
// Cleanup zombies processes
while (waitpid(-1, NULL, WNOHANG) > 0);
}
}
}
return 0;
}
