/*
*  Author - Alex Young
*  Filename - main.c
*  Created - 1/23/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 2: Files & Directories
*/

#include <fcntl.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define PREFIX "movies_"

/* struct for movie information */
struct movie {
    char *title;
    int year;
    struct movie *next;
};

/* 
*  Parse the current line which is space delimited and create a
*  movie struct with the data in this line
*/
struct movie *createMovie(char *currLine) {
    struct movie *currMovie = malloc(sizeof(struct movie));

    // For use with strtok_r
    char *saveptr;

    // The first token is the title
    char *token = strtok_r(currLine, ",", &saveptr);
    currMovie->title = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->title, token);

    // The next token is the year
    token = strtok_r(NULL, ",", &saveptr);
    currMovie->year = atoi(token);

    // Set the next node to NULL in the newly created movie entry
    currMovie->next = NULL;

    return currMovie;
}

/*
* Return a linked list of movies by parsing data from
* each line of the specified file.
*/
struct movie *processFile(char *filePath) {
    // Open the specified file for reading only
    FILE *movieFile = fopen(filePath, "r");

    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;
    char *token;
    int count = -1;

    // The head of the linked list
    struct movie *head = NULL;
    // The tail of the linked list
    struct movie *tail = NULL;

    // Read the file line by line
    while ((nread = getline(&currLine, &len, movieFile)) != -1) {
        if (count == -1) {
            count++;
        }
        else {
            // Get a new movie node corresponding to the current line
            struct movie *newNode = createMovie(currLine);
            count++;

            // Is this the first node in the linked list?
            if (head == NULL) {
                // This is the first node in the linked link
                // Set the head and the tail to this node
                head = newNode;
                tail = newNode;
            }
            else {
                // This is not the first node.
                // Add this node to the list and advance the tail
                tail->next = newNode;
                tail = newNode;
            }
        }
    }
    free(currLine);
    fclose(movieFile);
    printf("Now processing the chosen file named %s\n", filePath);
    return head;
}

/*
* Read in the files from the directory.
* Return the file name of movies file that is found depending on user input.
* For options 1 and 2 this function will check for csv. files with suffix "movies_"
*/
char *readDir(int option, char path[]) {
    // Open the current directory
    DIR* currDir = opendir(".");
    struct dirent *aDir;
    int fileSize = 0;
    int temp = 0;
    int length = 0;
    struct stat dirStat;
    char entryName[256] = "z";

    // Go through all the entries
    while ((aDir = readdir(currDir)) != NULL) {
        // With prefix movies_
        if (strncmp(PREFIX, aDir->d_name, strlen(PREFIX)) == 0) {
            // Get meta-data for the current entry
            stat(aDir->d_name, &dirStat);

            char tempEntry[256];
            strcpy(tempEntry, aDir->d_name);
            length = strlen(tempEntry);
            const char *check_csv = &tempEntry[length - 4];

            // If file extention is a csv 
            if (strcmp(check_csv, ".csv") == 0) {
                fileSize = dirStat.st_size;
                // Depending on the option, check for smaller or larger files
                if (option == 1 && fileSize > temp) {
                    memset(entryName, '\0', sizeof(entryName));
                    strcpy(entryName, aDir->d_name);
                    temp = fileSize;
                }
                else if (option == 2 && (fileSize < temp || temp == 0)) {
                    memset(entryName, '\0', sizeof(entryName));
                    strcpy(entryName, aDir->d_name);
                    temp = fileSize;
                }
            }
            
        }

        // if the inputted path is the same as the directory, set it to the filename
        if (option == 3 && strcmp(path, aDir->d_name) == 0) {
            memset(entryName, '\0', sizeof(entryName));
            strcpy(entryName, aDir->d_name);
        }
    }
    
    // Close the directory
    closedir(currDir);
    char *fileName = strdup(entryName);
    return fileName;
}

/*
* Free the movie structs in the linked list
*/
void freeMovie(struct movie *list) {
    while (list != NULL) {
        struct movie *temp = list;
        free(list->title);
        list = temp->next;
        free(temp);
    }
}

/*
* Print the top level user instruction and read in choices
*/
int instructions() {
    int i = 0;
    while (i < 1 || i > 2) {
        printf("\n1. Select file to process\n"
                "2. Exit from the program\n"
                "\nEnter a choice 1 or 2:  ");
        scanf("%i", &i);

        // only continue with integer inputs between 1 and 2
        if (i < 1 || i > 2) {
            printf("You entered an incorrect choice. Try again.\n");
        }
    }
    return i;
}

/*
* Ask the user what file they want to process and read in choices
*/
int chooseFile() {
    int i = 0;
    while (i < 1 || i > 3) {
        printf("\nWhat file you want to process?\n"
                "1. Enter 1 to pick the largest file\n"
                "2. Enter 2 to pick the smallest file\n"
                "3. Enter 3 to specify the name of a file\n"
                "\nEnter a choice from 1 to 3:  ");
        scanf("%i", &i);

        // only continue with integer inputs between 1 and 3
        if (i < 1 || i > 3) {
            printf("You entered an incorrect choice. Try again.\n");
        }
    }
    return i;
}

/*
* Make directory with files for each year with movies
*/
void createDir(struct movie *list) {

    // Head points to start of linked list, allowing to loop back
    struct movie *head = list;
    
    // Intialize rng - from 0 to 99999
    time_t t;
    srand((unsigned) time(&t));
    int random = rand() % 100000;
    char dirName[256];
    char fileName[256];
    int fd;

    // The new directory will be called younga6.movies.random with rwxr-x--- permissions
    sprintf(dirName, "./younga6.movies.%i", random);
    mkdir(dirName, 0750);

    // We will loop through every possible year
    for (int i = 1900; i < 2022; i++) {
        list = head;
        
        // Each new file will be called YYYY.txt for the incrementing year i
        sprintf(fileName, "%s/%i.txt", dirName, i);
        while (list != NULL) {
            // All movies with equivalent years will be added to the file
            if (list->year == i) {
                // Initialize movie titles to be added to files
                char *fileContent;
                fileContent = calloc(strlen(list->title) + 2, sizeof(char));
                // We will append new movies on the file, which is created with rw-r----- permissions
                fd = open(fileName, O_RDWR | O_CREAT | O_APPEND, 0640);
                if (fd == -1) {
                    printf("open() failed on \"%s\"\n", fileName);
                    perror("Error");
                    exit(1);
                }
                sprintf(fileContent, "%s\n", list->title);
                write(fd, fileContent, strlen(fileContent));
                close(fd);
                free(fileContent);
            }
            list = list->next;   
        }
    }
    printf("Created directory with name younga6.movies.%i", random);
}

/*
*   Follow user instructions to read in files and parce them into 
*   a linked list of movie structs which are written onto new 
*   directories and files sorted by year.
*   Compile the program as follows:
*       gcc --std=gnu99 -o movies_by_year main.c
*   Execute the program using:
*       ./movies_by_year
*/
int main(int argc, char *argv[]) {

    int fileChoice;
    int cont = 0;
    
    // While the program runs, print out instructions and run user choices
    while (cont == 0) {
        int i = instructions();
        char *path = 0;
        
        // User input choice 1
        if (i == 1) {
            fileChoice = chooseFile();

            // When user wants to look for smallest or biggest size file in the directory
            if (fileChoice == 1 || fileChoice == 2) {
                path = readDir(fileChoice, "");
                struct movie *list = processFile(path);
                // Create new directories and files using the movie struct, then free memory
                createDir(list);
                freeMovie(list);
            }
            
            // When user wants to input their own filename
            else if (fileChoice == 3) {
                int file_exist = 0;

                while (file_exist == 0) {
                    char path_input[256] = "x";
                    printf("Enter the complete file name:  ");
                    scanf("%s", path_input);
                    // here we make sure the file exists, setting it to path
                    path = readDir(fileChoice, path_input);

                    // if the file exists, create the new directory and files, otherwise try again
                    if (strcmp(path, path_input) == 0) {
                        struct movie *list = processFile(path);
                        createDir(list);
                        file_exist = 1;
                        freeMovie(list);
                    }

                    else {
                        printf("The file %s was not found. Try again\n", path_input);
                        free(path);
                    }

                }
                
            }
            free(path);
        }

        // User input choice 2: Exit program
        if (i == 2) {
            cont = 1;
        }
    }
    return EXIT_SUCCESS;
}
