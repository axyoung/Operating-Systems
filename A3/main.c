/*
* -------------------------------------------------------------
*  Author - Alex Young
*  Filename - main.c
*  Created - 2/7/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 3: smallsh
* -------------------------------------------------------------
*  Description: This program implements a smallsh (shell) in C.
*  This smallsh will provide a prompt for commands,
*  handle blank lines/comments, provide expansion for $$,
*  excecute built in exit/cd/status commands, execute other
*  commands through exec, support I/O redirection, support
*  running processes in background and foreground, and finally
*  handle SIGINT and SIGTSTP signals.
* -------------------------------------------------------------
*/

/* include header files with libraries that are used */
#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

/*
 * Global variable is used to control if shell is in foreground mode
 * - this makes the handler for sigint simpler to work with
 */  
int fg_mode = 0;

/* struct for commandline input information */
struct input {
    char *command;
    char *arg[512];
    int arg_num;
    int input_f;
    char *input_file;
    int output_f;
    char *output_file;
    int background;
};

/*
 * This structure is used to represent the child processes that are stored
 * in the linked list.
 */
struct childProcess {
    int pID;
    int status;
    struct childProcess *next;
};

/*
 * This structure is used to represent the linked list.
 */
struct list {
    struct childProcess *head;
};

/*
 * This function will allocate and initialize a new, empty linked list and
 * return a pointer to it.
 * It is transcribed from my CS261 coursework along with other framework for my linked list.
 */
struct list *list_create() {
	struct list* list = NULL;
	list = malloc(sizeof(struct list));
	list->head = NULL;
    return list;
}

/* 
*  Parse the current line which is space delimited and create a
*  input struct with the data in this line
*/
struct input *createInput(char *currLine) {
    struct input *currCommand = malloc(sizeof(struct input));
    currCommand->input_f = 0;
    currCommand->output_f = 0;
    currCommand->background = 0;

    // For use with strtok_r
    char *saveptr;
    int line_len = strlen(currLine);
    // Due to the ambiguity of args and I/O files, I keep track of the current
    // total token length and compare it to the input length with curr_len
    int curr_len = 0;
    int count = 1;
    int cont = 0;

    // The first token is the command
    char *token = strtok_r(currLine, " ", &saveptr);
    currCommand->command = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currCommand->command, token);
    currCommand->arg[0] = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currCommand->arg[0], token);
    // curr_len is incrememnted with the token length and the space that strtok parses with
    curr_len = curr_len + strlen(token) + 1;

    if (curr_len > line_len) {
        cont = 3;
    }

    // The next tokens are arguments, loop through 512 arguments or until an input/output or end of input
    while (count < 512 && cont == 0) {
        token = strtok_r(NULL, " ", &saveptr);        
        curr_len = curr_len + strlen(token) + 1;
        if (strncmp(token, "<", 1) == 0) {
            currCommand->input_f = 1;
            cont = 1;
        }
        else if (strncmp(token, ">", 1) == 0) {
            currCommand->output_f = 1;
            cont = 2;
        }
        else {
            currCommand->arg[count] = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->arg[count], token);
            count++;
        }
        if (curr_len > line_len) {
            cont = 3;
        }
    }
    // Keep track of how many arguments there are in an input to make freeing each arg easier
    currCommand->arg_num = count;
    currCommand->arg[count] = NULL;

    // Looping here allows for multiple I/O files with the last one being saved
    while (cont < 3) {
        if (cont == 1) {
            // The next token is the input file
            token = strtok_r(NULL, " ", &saveptr);
            currCommand->input_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->input_file, token);
            curr_len = curr_len + strlen(token) + 1;
            if (curr_len >= line_len) {
                cont = 3;
            }
            else {
                token = strtok_r(NULL, " ", &saveptr);
                curr_len = curr_len + strlen(token) + 1;
                if (strncmp(token, ">", 1) == 0) {
                    currCommand->output_f = 1;
                    cont = 2;
                }
            }
        }

        if (cont == 2) {
            // The next token is the output file
            token = strtok_r(NULL, " ", &saveptr);
            currCommand->output_file = calloc(strlen(token) + 1, sizeof(char));
            strcpy(currCommand->output_file, token);
            curr_len = curr_len + strlen(token) + 1;
            if (curr_len >= line_len) {
                cont = 3;
            }
            else {
                token = strtok_r(NULL, " ", &saveptr);
                curr_len = curr_len + strlen(token) + 1;
                if (strncmp(token, "<", 1) == 0) {
                    currCommand->input_f = 1;
                    cont = 1;
                }
            }
        }
    }
    return currCommand;
}

/*
 * Custom handler for SIGTSTP changes the shell mode to or from foreground only
 * The handler will also print out the respective mode that has been entered
 */
void handle_SIGTSTP(int signo){
	fg_mode = !fg_mode;
    if (fg_mode) {
        write(STDOUT_FILENO, "\nEntering foreground-only mode (& is now ignored)\n: ", 53);
    }
    else {
        write(STDOUT_FILENO, "\nExiting foreground-only mode\n: ", 33);
    }
    return;
}

/*
 * The freeInput function is called when freeing an input struct
 * It dealocates the memory allocated in the struct and then the memory of the struct
 * This is called after every main loop (before the next prompt) because all foreground processes
 * will be complete at this point and all background processes are stored in the linked list.
 */
void freeInput(struct input *temp) {
    free(temp->command);
    if (temp->input_f == 1) {
        free(temp->input_file);
    }
    if (temp->output_f == 1) {
        free(temp->output_file);
    }
    for (int i = 0; i < temp->arg_num; i++) {
        free(temp->arg[i]);
    }
    free(temp);
}

/*
 * The freeList function frees the linked list of background processes
 * It is only called when the exit command is processed
 * It also terminates all background processes
 */
void freeList(struct list* list) {
	if (list != NULL) {	
		struct childProcess* current;
		while (list->head != NULL) {
			current = list->head;
			list->head = list->head->next;
            kill(current->pID, SIGTERM);
			free(current);
		}
		free(list);
	}
    return;
}

/*
 * The freeProcess function removes a link from the linked list after the 
 * process terminates normally. Again the basis for this function comes
 * from CS261 coursework on linked lists. 
 */
void freeProcess(struct childProcess *current, struct list *list) {
    if (list != NULL) {
        struct childProcess *temp = list->head;
        if (temp != NULL && temp->pID == current->pID) {
            list->head = temp->next;
            free(temp);
            return;
        }
        while (temp->next != NULL) {
			if (temp->next->pID == current->pID) {
				struct childProcess *temp_c = temp->next;
				temp->next = temp->next->next;
				free(temp_c);
				return;
			}
		temp = temp->next;
        }   
    }
    return;
}

/*
 * The checkTerminate function checks if a background function has terminated.
 * This function is necessary to avoid defunct processes and is called right
 * before returning access to the command line to the user
 */
void checkTerminate(struct list *list) {
    struct childProcess *curr = list->head;
    int wait_result;

    // Starting at the head, this will iterate through every background process stored in the list
    while (curr != NULL) {
        struct childProcess *temp = curr->next;
        int status = curr->status;

        // The restult of waitpid will be the same as the pid with WNOHANG if the process has terminated
        wait_result = waitpid(curr->pID, &status, WNOHANG);
        if (wait_result == curr->pID) {
            if (WIFEXITED(status)) {
                printf("background pid %d is done: exit value %d\n", curr->pID, WEXITSTATUS(status));
                fflush(stdout);
                freeProcess(curr, list);
            }
            else {
                printf("background pid %d is done: terminated by signal %d\n", curr->pID, WTERMSIG(status));
                fflush(stdout);
                freeProcess(curr, list);
            }
        }
        else if (wait_result == -1) {
            perror("Error");
            exit(0);
        }
        curr = temp;
    }
}

/*
 * The createChildProcess is a robust function that
 * forks a child, handles redirection, and execs the command.
 * This function returns the status outputted from waitpid
 * to be used by the in-built status command later.
 * A foreground status is passed into the function to be returned
 * if the input is a background process (and we dont call waitpid)
 */
int createChildProcess(struct input *commandLine, struct list *list, int f_status) {
    int result;
    int status = 0;
    pid_t newChild = fork();

    if (newChild < 0) {
        perror("Error");
        exit(0);
    }
    // Case where we are the child process that has been forked to
    else if (newChild == 0) {
        // Here we setup all child processes to SIG_IGN (ignore) the SIGTSTP signal
        struct sigaction ignore_action = {0};
        ignore_action.sa_handler = SIG_IGN;
        ignore_action.sa_flags = SA_RESTART;
        sigaction(SIGTSTP, &ignore_action, NULL);

        // if there is an input file, redirect input accordingly with dup2
        if (commandLine->input_f == 1) {
            int sourceFD = open(commandLine->input_file, O_RDONLY);
            if (sourceFD == -1) {
                printf("cannot open %s for input\n", commandLine->input_file);
                fflush(stdout);
                exit(1);
            }
            result = dup2(sourceFD, 0);
            // In the case of an error, print it out
            if (result == -1) {
                perror(commandLine->input_file);
                exit(1);
            }
        }

        // if there is no input file and input is to be run in background
        // then redirect input to /dev/null with dup2
        else if (commandLine->input_f == 0 && commandLine->background == 1) {
            int sourceFD = open("/dev/null", O_RDONLY);
            if (sourceFD == -1) {
                perror("/dev/null");
                exit(1);
            }
            result = dup2(sourceFD, 0);
            if (result == -1) {
                perror("/dev/null");
                exit(1);
            }
        }

        // if there is an output file, redirect output accordingly with dup2
        if (commandLine->output_f == 1) {
            int targetFD = open(commandLine->output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (targetFD == -1) {
                printf("cannot open %s for output\n", commandLine->output_file);
                fflush(stdout);
                exit(1);
            }
            result = dup2(targetFD, 1);
            if (result == -1) {
                perror(commandLine->output_file);
                exit(1);
            }
        }

        // if there is no output file and output is to be run in background
        // then redirect output to /dev/null with dup2
        else if (commandLine->output_f == 0 && commandLine->background == 1) {
            int targetFD = open("/dev/null", O_WRONLY);
            if (targetFD == -1) {
                perror("/dev/null");
                exit(1);
            }
            result = dup2(targetFD, 1);
            if (result == -1) {
                perror("/dev/null");
                exit(1);
            }
        }

        // Here we setup all foreground child processes to SIG_DFL (default action) 
        // when catching the the SIGINT signal. We do not use a custom handler
        // for the active portion of SIGINT because it is the same as the default action
        if (commandLine->background == 0) {
            struct sigaction def_action = {0};
            def_action.sa_handler = SIG_DFL;
            def_action.sa_flags = SA_RESTART;
            sigaction(SIGINT, &def_action, NULL);
        }

        // After handling all redirection, execvp is called on the input command and its arguments
        result = execvp(commandLine->command, commandLine->arg);
        if (result == -1) {
            perror(commandLine->command);
            exit(1);
        }

        // Should never reach here, this is just for safety
        exit(0);
    }
    
    // If the current process is the parent, print the pid if it is a
    // background process. If it a foreground process, wait for it to terminate.
    else if (newChild > 0) {
        if (commandLine->background == 1) {
            // Sleep so that if command fails, prompt prints correctly
            // This handles some prompt out errors but does cause a slight delay in
            // outputting the prompt after a background process is inputted
            sleep(1);
            printf("background pid is %i\n", newChild);
            fflush(stdout);
        }
        else {
            waitpid(newChild, &status, 0);
        }
    }

    // Check how foreground process terminated, return the status of waitpid
    if (commandLine->background == 0) {
        if (WIFEXITED(status)) {
            return(status);
        }
        else if (WIFSIGNALED(status)) {
            printf("terminated by signal %d\n", WTERMSIG(status));
            fflush(stdout);
            return(status);
        }
    }
    // If we are a background process, create a link in the linked list to store
    // the process ID and status of the current process. This will be used to 
    // check if any background processes have terminated later on.
    else {
        struct childProcess *node = malloc(sizeof(struct childProcess));
        node->pID = newChild;
        node->status = status;
        node->next = list->head;
        list->head = node;
    }
    return f_status;
}

/*
 * The changeDir function is called when the user inputs the built-in command cd.
 * This function uses chdir to change directory to the path input noted by the user
 * in their argument
 */
void changeDir(char *path) {
    int fd = chdir(path);
    if (fd == -1) {
        printf("open() failed on \"%s\"\n", path);
        fflush(stdout);
    }
}

/*
*   This is the main function that implements the smallsh shell
*   Compile the program as follows:
*       gcc --std=gnu99 -o smallsh main.c
*   Execute the program using:
*       ./smallsh
*/
int main(int argc, char *argv[]) {
    // Here I start by setting my parent process to SIG_IGN (ignore) SIG_INT
    // This also will affect the child processes when we fork, which is why
    // I only update the SIG_INT of the foreground processes which are affected
    // by the SIG_INT signal
    struct sigaction ignore_action = {0}, SIGTSTP_action = {0};
	ignore_action.sa_handler = SIG_IGN;
    // All my sigaction calls use SA_RESTART as the flag, this especially helps
    // when trying to catch signals on the prompt during getline()
    ignore_action.sa_flags = SA_RESTART;
    sigaction(SIGINT, &ignore_action, NULL);
    // Here I setup the parent process to call my custom handle (handle_SIGTSTP) on signal SIGTSTP
    // Later, after forking, the children will be set to ignore SIGTSTP
    // The custom handler for the parent will update a golbal variable to control the shell "mode"
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // These variables are used in the main function to reformat the input, check for continues,
    // give the environment variable HOME, and store the status for the in-built status command
    int cont = 0;
    char *position;
    char *input;
    char *prefix;
    char *suffix;
    char *home_path = getenv("HOME");
    size_t size = 2048;
    long processID = getpid();
    char ID_string[32];
    int status = 0;
    // Create a list of processes that we can access through the head
    struct list *processes = list_create();
    
    // While the program continues to run, prompt the user and execute commands
    while (cont == 0) {
        // We start by checking if any of our background processes have terminated right
        // before prompting the user for their command input with a colon
        checkTerminate(processes);
        printf(": ");
        fflush(stdout);
        input = (char *)malloc(size * sizeof(char));
        // normalAnd will be used to check if the input is to be run as a background process
        int normalAnd = 1;

        getline(&input, &size, stdin);
        // First off, find the position of the newline in the input and replace it with a \0
        position = strchr(input, '\n');
        if (position != NULL) {
            *position = '\0';
        }
        // Next, check for the last char being & and replace the empty space before the & with \0
        if (strlen(input) > 1) {
            if (input[strlen(input) - 1] == '&' && input[strlen(input) - 2] == ' ') {
                normalAnd = 0;
                input[strlen(input) - 2] = 0;
            }
        }
        // Here ID_string is used to get the length of the string output of the process id
        // This is useful when formating the expansion of && into the process id
        // The position will be used to set the prefix and suffix to the $$ expansion
        snprintf(ID_string, 32, "%ld", processID);
        position = strstr(input, "$$");
        // This loop implements the expansion of variable $$
        // Each iteration of the loop will replace one instance of $$ until there are no more $$
        // variables left in the input string to handle. A $$$ will give <processID>$
        while (position != NULL) {
            // The new length is as set below because we are subtracting 2 $$ charcters
            // but we need to keep track of the extra null character at the end
            int new_len = (strlen(input) + strlen(ID_string) - 1);        
            *position = '\0';
            prefix = input;
            suffix = position + 2;
            char* temp_input = (char *)malloc(new_len * sizeof(char));
            snprintf(temp_input, new_len, "%s%s%s", prefix, ID_string, suffix);
            // Freeing the input and setting it to temp_input allows for the loop to function correctly
            // because in the next iteration the first instance of "$$" will be reformatted
            free(input);
            input = temp_input;
            position = strstr(input, "$$");
            size = new_len;
        }

        // Handle blank lines and comments (note that spaces dont count as blank lines, and will seg fault)
        if (strncmp(input, "#", 1) == 0 || strlen(input) == 0) {
            free(input);
        }
        // If the command is not blank/a comment, then we will parse it into our input struct
        // Keep in mind that this does not error check the syntax, and extra spaces may cause seg faults
        // strsep may be better to use to tokenize in the future
        else {
            struct input *newInput = createInput(input);
            // Now that the input is in the struct, we can update if it will run as a background process or not
            if (fg_mode || normalAnd) {
                newInput->background = 0;
            }
            else if (!normalAnd) {
                newInput->background = 1;
            }
            
            // If the user uses built-in command exit, free the current
            // input struct, input string, and background processes.
            if (strcmp(newInput->command, "exit") == 0) {
                freeInput(newInput);
                free(input);
                freeList(processes);
                exit(0);
            }

            // If the user uses built-in command cd, call the changeDir function with correct path
            else if (strcmp(newInput->command, "cd") == 0) {
                if (newInput->arg[1] == NULL) {
                    changeDir(home_path);
                } else {
                    changeDir(newInput->arg[1]);
                }
            }

            // If the user uses built-in command status, check how the process terminated with the status
            // If it terminated normally, print the exit value, if it was terminated by signal, print signal num
            else if (strcmp(newInput->command, "status") == 0) {
                if (WIFEXITED(status)) {
                    printf("exit value %i\n", WEXITSTATUS(status));
                    fflush(stdout);
                }
                else if (WIFSIGNALED(status)) {
                    printf("terminated by signal %i\n", WTERMSIG(status));
                    fflush(stdout);
                }
            }

            // If the command is not built-in create a child to call the exec libary of functions on
            else {
                status = createChildProcess(newInput, processes, status);
            }
            // At the end of processing each command input, free the input struct and input string
            freeInput(newInput);
            free(input);
        }
    }
}