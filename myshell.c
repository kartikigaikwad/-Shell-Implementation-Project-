/*********************************************************************************************
This is a template for assignment on writing a custom Shell. 

Students may change the return types and arguments of the functions given in this template,
but do not change the names of these functions.

Though use of any extra functions is not recommended, students may use new functions if they need to, 
but that should not make code unnecessorily complex to read.

Students should keep names of declared variable (and any new functions) self explanatory,
and add proper comments for every logical step.

Students need to be careful while forking a new process (no unnecessory process creations) 
or while inserting the signal handler code (which should be added at the correct places).

Finally, keep your filename as myshell.c, do not change this name (not even myshell.cpp, 
as you dp not need to use any features for this assignment that are supported by C++ but not by C).
*********************************************************************************************/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>			// exit()
#include <unistd.h>			// fork(), getpid(), exec()
#include <sys/wait.h>		// wait()
#include <signal.h>			// signal()
#include <fcntl.h>			// close(), open()
#define MAX_INPUT_SIZE 1024
#define MAX_TOKEN_SIZE 64
#define MAX_NUM_TOKENS 64

//utils
int order=0;    // to check for redirection and sequential inputs
void changedir(char ** chartoken){
    if(chdir(chartoken[1])!=0){
//        printf("chdir to %s failed\n", chartoken[1]);
        printf("Shell: Incorrect command\n");
    }
}   // when cd is called

//premade functions
char ** parseInput(char * line)
{
	// This function will parse the input string into multiple commands or a single command with arguments depending on the delimiter (&&, ##, >, or spaces).


    char **tokens = (char **)malloc(MAX_NUM_TOKENS * sizeof(char *));
    char *token = (char *)malloc(MAX_TOKEN_SIZE * sizeof(char));
    int i, tokenIndex = 0, tokenNo = 0;

    for(i =0; i < strlen(line); i++){

        char readChar = line[i];
        if(readChar=='&')
            order+=2;
        if(readChar=='#')
            order-=1;
        if(readChar=='>')
            order+=1;
        if (readChar == ' ' || readChar == '\n' || readChar == '\t'){
            token[tokenIndex] = '\0';
            if (tokenIndex != 0){
                tokens[tokenNo] = (char*)malloc(MAX_TOKEN_SIZE*sizeof(char));
                strcpy(tokens[tokenNo++], token);
                tokenIndex = 0;
            }
        } else {
            token[tokenIndex++] = readChar;
        }
    }

    free(token);
    tokens[tokenNo] = NULL ;
    return tokens;

}

void executeCommand(char ** chartoken)
{
	// This function will fork a new process to execute a command
    if(strcmp(chartoken[0],"cd")==0){
        changedir(chartoken);
    }else{
        pid_t pid;

        int  status;

        if ((pid = fork()) < 0) {
//            printf("*** ERROR: forking child process failed\n");
            exit(1);
        }
        else if (pid == 0) {
            if (execvp(chartoken[0], chartoken) < 0) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
        }
        else
            while (wait(&status) != pid)
                ;
    }
}

void executeParallelCommands(char ** words, int orde_r)
{
	// This function will run multiple commands in parallel
    pid_t pid, pidt;
    int  status=0;

    int index=0;
    int previndex=index;
    while(words[index]!=NULL&&orde_r>=0){
        while (words[index]!=NULL&&strcmp(words[index],"&&")!=0){
            index++;
        }
        words[index]=NULL;
        pid = fork();
        if (pid  < 0) {
//            printf("*** ERROR: forking child process failed\n");
            exit(1);
        }
        else if (pid == 0) {
            if (execvp(words[previndex], &words[previndex]) < 0) {
                printf("Shell: Incorrect command\n");
                exit(1);
            }
//                int count=0;
//                while (count<10000000){
//                    printf("%d",count);
//                    count++;
//                }
        }
        index++;
        previndex=index;
        orde_r-=4;
    }
    while((pidt=wait(&status))>0);

}

void executeSequentialCommands(char ** words, int orde_r)
{	
	// This function will run multiple commands in parallel
	int index=0;
	int previndex=index;
	while(words[index]!=NULL&&orde_r<=0){
//	    printf("%d",index);
	    while (words[index]!=NULL&&strcmp(words[index],"##")!=0){
	        index++;
	    }
	    words[index]=NULL;
	    executeCommand(&words[previndex]);
	    index++;
	    previndex=index;
	    orde_r+=2;
	}
}

void executeCommandRedirection(char **words)
{
	// This function will run a single command with output redirected to an output file specificed by user
    // index+1 points to the path of file to be opened
	int index=0;
	while (words[index]!=NULL&&strcmp(words[index],">")!=0) {
        index++;
    }
	words[index]=NULL;


	pid_t pid;
	int  status;
	if ((pid = fork()) < 0) {
//	    printf("*** ERROR: forking child process failed\n");
	    exit(1);
	}
	else if (pid == 0) {

        //setting up file descriptor
	    int fd = open(words[index+1],O_RDWR | O_CREAT | O_APPEND, 0600);
	    if(fd==-1){
//	        perror("opening cout.log");
	    }
	    int save_out = dup(fileno(stdout));
	    if(-1==dup2(fd,fileno(stdout))){
//	        perror("cannot redirect stdout");
	    }

        //executing command
        if (execvp(words[0], words) < 0) {
            printf("Shell: Incorrect command\n");
            exit(1);
        }

        //closing file descriptor
	    fflush(stdout); close(fd);
	    dup2(save_out, fileno(stdout));
	    close(save_out);
	}
	else
	    while (wait(&status) != pid)
	        ;

}

int main()
{
    // Blocking Signals
    sigset_t block_set;
    sigemptyset(&block_set);
    sigaddset(&block_set, SIGINT);
    sigaddset(&block_set, SIGTSTP);
    sigprocmask(SIG_BLOCK, &block_set, NULL);


	// Initial declarations

	while(1)	// This loop will keep your shell running until user exits.
	{
        // To reset the order
	    order=0;

		// Print the prompt in format - currentWorkingDirectory$
        char cwd[FILENAME_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            printf("%s$", cwd);
        }

		//create string buffer to hold path
        char *buffer;
        size_t bufsize = MAX_INPUT_SIZE;
        size_t characters;
        buffer = (char *)malloc(bufsize * sizeof(char));
        if( buffer == NULL){
//            perror("Unable to allocate buffer");
            exit(1);
        }

		// accept input with 'getline()'
        characters = getline(&buffer,&bufsize,stdin);

        // Parse input with 'strsep()' for different symbols (&&, ##, >) and for spaces.
        // Instead of using strsep(), I store the input line in an array of strings/char *, making words a double array
		char ** words=parseInput(buffer);

        // When user uses exit command.
		if(strcmp(words[0],"exit")==0||strcmp(words[0],"EXIT")==0)
		{
			printf("Exiting shell...\n");
			break;
		}

		// order is set during parseInput()
		if(order>1)
			executeParallelCommands(words, order);		// This function is invoked when user wants to run multiple commands in parallel (commands separated by &&)
		else if(order<0)
			executeSequentialCommands(words, order);	// This function is invoked when user wants to run multiple commands sequentially (commands separated by ##)
		else if(order==1)
			executeCommandRedirection(words);	// This function is invoked when user wants redirect output of a single command to and output file specificed by user
		else
			executeCommand(words);		// This function is invoked when user wants to run a single commands

        // Freeing the allocated memory
        for(int i=0;words[i]!=NULL;i++){
            free(words[i]);
        }
        free(words);
	}
	
	return 0;
}
