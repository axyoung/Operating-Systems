// If you are not compiling with the gcc option --std=gnu99, then
// uncomment the following line or you might get a compiler warning
//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* struct for student information */
struct student
{
    char *onid;
    char *lastName;
    char *firstName;
    char *major;
    struct student *next;
};

/* Parse the current line which is space delimited and create a
*  student struct with the data in this line
*/
struct student *createStudent(char *currLine)
{
    struct student *currStudent = malloc(sizeof(struct student));

    // For use with strtok_r
    char *saveptr;

    // The first token is the ONID
    char *token = strtok_r(currLine, ",", &saveptr);
    currStudent->onid = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currStudent->onid, token);

    // The next token is the lastName
    token = strtok_r(NULL, ",", &saveptr);
    currStudent->lastName = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currStudent->lastName, token);

    // The next token is the firstName
    token = strtok_r(NULL, ",", &saveptr);
    currStudent->firstName = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currStudent->firstName, token);

    // The last token is the major
    token = strtok_r(NULL, "\n", &saveptr);
    currStudent->major = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currStudent->major, token);

    // Set the next node to NULL in the newly created student entry
    currStudent->next = NULL;

    return currStudent;
}

/*
* Return a linked list of students by parsing data from
* each line of the specified file.
*/
struct student *processFile(char *filePath)
{
    // Open the specified file for reading only
    FILE *studentFile = fopen(filePath, "r");

    char *currLine = NULL;
    size_t len = 0;
    ssize_t nread;
    char *token;

    // The head of the linked list
    struct student *head = NULL;
    // The tail of the linked list
    struct student *tail = NULL;

    // Read the file line by line
    while ((nread = getline(&currLine, &len, studentFile)) != -1)
    {
        // Get a new student node corresponding to the current line
        struct student *newNode = createStudent(currLine);

        // Is this the first node in the linked list?
        if (head == NULL)
        {
            // This is the first node in the linked link
            // Set the head and the tail to this node
            head = newNode;
            tail = newNode;
        }
        else
        {
            // This is not the first node.
            // Add this node to the list and advance the tail
            tail->next = newNode;
            tail = newNode;
        }
    }
    free(currLine);
    fclose(studentFile);
    return head;
}

/*
* Print data for the given student
*/
void printStudent(struct student* aStudent){
  printf("%s, %s %s, %s\n", aStudent->onid,
               aStudent->firstName,
               aStudent->lastName,
               aStudent->major);
}
/*
* Print the linked list of students
*/
void printStudentList(struct student *list)
{
    while (list != NULL)
    {
        printStudent(list);
        list = list->next;
    }
}

/*
*   Process the file provided as an argument to the program to
*   create a linked list of student structs and print out the list.
*   Compile the program as follows:
*       gcc --std=gnu99 -o students main.c
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./students student_info1.txt\n");
        return EXIT_FAILURE;
    }
    struct student *list = processFile(argv[1]);
    printStudentList(list);
  
    return EXIT_SUCCESS;
}
