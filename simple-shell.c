#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/stat.h>
#include <wait.h>
#define TRUE 1
#define FALSE 0
#define MAX_BUF_SIZE 300
#define MAX_TOKENS 30

char* read_input();
char** split_input(char* input);
void main_loop();
void process_handler(char** args);
void pipe_handler(char** args, char** args2);
void pwd();
void cd(char* path);

int main(){
    main_loop();
    return 0;
}

// reads user input
char* read_input(){
    char* buffer = (char*)malloc(sizeof(char)*MAX_BUF_SIZE);

    // checking for allocation problems
    if (buffer == NULL) {
        perror("allocation error in read_input\n");
        return NULL;
    }

    fgets(buffer, MAX_BUF_SIZE, stdin);
    return buffer;
}

// receives a string and returns an array with the arguments(commands) entered
char** split_input(char* input){
    char** tokens = (char**)malloc(sizeof(char*)*MAX_TOKENS);

    // checking for allocation problems
    if (tokens == NULL) {
        perror("allocation error in split_input\n");
        return NULL;
    }

    char* token;
    int i = 0;

    token = strtok(input, " \t\n\r");
  
    // while the string hasn't ended and a '|' isn't found
    while (token != NULL && strcmp(token, "|") != 0) {
        tokens[i] = token;
        i++;

        //strtok receives NULL as a parameter to continue splitting the string.
        token = strtok(NULL, " \t\n\r");
    }

    //after all the words have been split, NULL will be the last value.
    tokens[i] = NULL;

    return tokens;
}

// main part of the code that contains the loop
void main_loop(){
    char* input;
    char** args;
    char** args2 = NULL;
    int proceed = TRUE;
    char* check_pipe;

    do {
        printf("~");
        pwd();
        printf("$ ");

        input = read_input();

        // copying input to check_pipe so we don't mess up the original string
        check_pipe = (char*)malloc(sizeof(char)*(strlen(input)+1));

        // checking for allocation problems
        if (check_pipe != NULL) {
            strcpy(check_pipe, input);
            check_pipe = strchr(check_pipe, '|');
        }

        args = split_input(input);

        // if a pipe has been found
        if (check_pipe != NULL) {
            check_pipe = check_pipe+1; // gets rid of the first character, '|'
            args2 = split_input(check_pipe);

        //if a pipe hasn't been found, free check_pipe    
        } else { 
            free(check_pipe);
        }

        // checking the commands entered
        // no commands
        if (args[0] == NULL) { 
            printf("\n");
            continue;

        // exit    
        } else if (strcmp(args[0], "exit") == 0) { 
            proceed = FALSE;

        // commands with pipe
        } else if (args2 != NULL) { 
            pipe_handler(args, args2);

        // commands with no pipe
        } else { 
            process_handler(args);
        }

        free(input);
        free(args);
        if (args2 != NULL) {
            free(args2);
        }
    } while (proceed);
}

// function executes commands with no pipes
void process_handler(char** args){
    pid_t pid;
      
    // pwd command was entered    
    if (strcmp(args[0], "pwd") == 0) {
        pwd();
        printf("\n");

    // cd command was entered
    } else if (strcmp(args[0], "cd") == 0) {
        cd(args[1]);

    //if a different command than pwd or cd was entered
    } else { 
        pid = fork();

        // checking fork error
        if (pid == -1) {
            perror("fork error in process_handler");
            exit(1);

        //child's turn
        } else if (pid == 0) { 
            if (execvp(args[0], args) < 0) {
                perror("child error in process_handler");
            }


        //parents' turn, waits for child to terminate
        } else { 
            waitpid(pid,NULL, WUNTRACED);
            return;
        }
    }  
}

// function executes commands when there's a pipe (only works for one pipe)
void pipe_handler(char** args, char** args2){
    pid_t pid1, pid2;
    int fd[2];

    //checking for piping error
    if (pipe(fd) < 0) {
        perror("pipe error in pipe_handler()");
        exit(1);
    }

    //parent's first child
    pid1 = fork();
    if (pid1 < 0) {
        perror("pid1 fork error in pipe_handler()");
        exit(1);

    //first child's turn    
    } else if (pid1 == 0) { 
        close(fd[0]); //closes read
        dup2(fd[0], STDIN_FILENO); //writing

        //executes command
        if (execvp(args[0], args) < 0) {
            perror("first child execution error in pipe_handler()");
            exit(1);
        }

    //parent's turn    
    } else {

        //parent's second child
        pid2 = fork();
        if (pid2 < 0) {
            perror("pid2 fork error in pipe_handler()");
            exit(1);
        
        //second child's turn    
        }else if (pid2 == 0) { 
            close(fd[1]); //closes write
            dup2(fd[1], STDOUT_FILENO); //reads

            //executes command
            if (execvp(args2[0], args2) < 0) {
                perror("second child execution error in pipe_handler()");
                exit(1);
            }

        //parent's turn again
        } else {
            //parent waits for second child to terminate
            waitpid(pid2, NULL, WUNTRACED);
        }

        //parent waits for first child to terminate
        waitpid(pid1, NULL, WUNTRACED);
        
    }
}

// print working directory
void pwd() {
    char cwd[CHAR_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("%s", cwd);
    } else {
        perror("pwd error");
    }
}

// change working directory
void cd(char* path) {
    //chdir returns 0 if successful.
    if (chdir(path) != 0) {
        perror("cd error");
    }
}