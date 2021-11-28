/*
*  Author - Alex Young
*  Filename - main.c
*  Created - 1/17/2021
*  CS 344 - Justin Goins
*  Assignment 1: Movies
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* struct for movie information */
struct movie
{
    char *title;
    int year;
    char *lang;
    char languages[5][21];
    int num_lang;
    double rating;
    struct movie *next;
};

/* 
*  Parse the current line which is space delimited and create a
*  movie struct with the data in this line
*/
struct movie *createMovie(char *currLine)
{
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

    // The next token is the languages
    token = strtok_r(NULL, ",", &saveptr);
    currMovie->lang = calloc(strlen(token) + 1, sizeof(char));
    strcpy(currMovie->lang, token);

    // token2 is used to hold language substrings of token
    char *token2;
    int i = 0;
    // remove the first and last index '[ ]' of language token
    token++;
    token[strlen(token) - 1] = 0;

    while (token2 = strtok_r(token, ";", &token))
    {
        strcpy(currMovie->languages[i], token2);
        i++;
    }
    currMovie->num_lang = i;

    // The last token is the rating value
    token = strtok_r(NULL, "\n", &saveptr);
    currMovie->rating = atof(token);

    // Set the next node to NULL in the newly created movie entry
    currMovie->next = NULL;

    return currMovie;
}

/*
* Return a linked list of movies by parsing data from
* each line of the specified file.
*/
struct movie *processFile(char *filePath)
{
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
    while ((nread = getline(&currLine, &len, movieFile)) != -1)
    {
        if (count == -1)
        {
            count++;
        }
        else
        {
            // Get a new movie node corresponding to the current line
            struct movie *newNode = createMovie(currLine);
            count++;

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
    }
    free(currLine);
    fclose(movieFile);
    printf("Processed file %s and parsed data for %i movies\n", filePath, count);
    return head;
}

/*
* Print data for the given movie
*/
void printMovie(struct movie *aMovie){
    printf("%s, %i, %s, %i, %0.1f\n",
            aMovie->title,
            aMovie->year,
            aMovie->lang,
            aMovie->num_lang,
            aMovie->rating);
}

/*
* Print the linked list of movies
*/
void printMovieList(struct movie *list)
{
    while (list != NULL)
    {
        printMovie(list);
        list = list->next;
    }
}

/*
* Free the movie structs in the linked list
*/
void freeMovie(struct movie *list)
{
    while (list != NULL)
    {
        struct movie *temp = list;
        free(list->title);
        free(list->lang);
        list = temp->next;
        free(temp);
    }
}

/*
* Print the user instruction and read in choices
*/
int instructions()
{
    int i = 0;
    while (i < 1 || i > 4)
    {
        printf("\n1. Show movies released in the specified year\n"
                "2. Show highest rated movie for each year\n"
                "3. Show the title and year of release of all movies in a specific language\n"
                "4. Exit from the program\n"
                "\nEnter a choice from 1 to 4: ");
        scanf("%i", &i);

        // only continue with integer inputs between 1 and 4
        if (i < 1 || i > 4)
        {
            printf("You entered an incorrect choice. Try again.\n");
        }
    }
    return i;
}

/*
* Show movies released in a certain year
*/
void optionOne(struct movie *list)
{
    int i;
    int temp = 0;

    // User will enter a year value
    printf("Enter the year for which you want to see movies: ");
    scanf("%i", &i);
    while (list != NULL)
    {
        // all movies with equivalent years will be printed
        if (list->year == i) {
            printf("%s\n", list->title);
            temp = 1;
        }
        list = list->next;
    }

    // if no movie has a matching year, print message
    if (temp == 0)
    {
        printf("No data about movies released in the year %i\n", i);
    }
}

/*
* Show highest rated movie for each year
*/
void optionTwo(struct movie *list)
{
    // Create an array from years 1900 to 2021 that holds pointers to a struct
    struct movie *highest[122];
    for (int i = 0; i < 122; i++)
    {
        highest[i] = NULL;
    }

    // for every movie in the list compare rating to the same year movies
    while (list != NULL)
    {
        if (highest[((list->year) - 1900)] == NULL)
        {
            highest[((list->year) - 1900)] = list;
        }
        else if (highest[((list->year) - 1900)]->rating < list->rating) {
            highest[((list->year) - 1900)] = list;
        }

        list = list->next;
    }

    // print out every highest rating per year
    for (int i = 0; i < 122; i++)
    {
        if (highest[i] != NULL)
        {
            printf("%i %0.1f %s\n", highest[i]->year, highest[i]->rating, highest[i]->title);
        }
    }
}

/*
* Show movies and their year of release for a specific language
*/
void optionThree(struct movie *list)
{
    char temp_lang[21];
    int temp = 0;

    // Ask user for desired language
    printf("Enter the language for which you want to see movies: ");
    scanf("%s", temp_lang);

    // For every movie, check if it has the desired langauge
    // If language is found, print year and title of movie
    while (list != NULL)
    {
        for (int i = 0; i < list->num_lang; i++)
        {
            if (strcmp(list->languages[i], temp_lang) == 0) {
                printf("%i %s\n", list->year, list->title);
                temp = 1;
            }
        }
        
        list = list->next;
    }

    // If data does not include any movie in the language, print message
    if (temp == 0)
    {
        printf("No data about movies released in %s\n", temp_lang);
    }
}

/*
*   Process the file provided as an argument to the program to
*   create a linked list of movie structs and follow user instructions.
*   Compile the program as follows:
*       gcc --std=gnu99 -o movies main.c
*/

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("You must provide the name of the file to process\n");
        printf("Example usage: ./movie.exe movies_sample_1.csv\n");
        return EXIT_FAILURE;
    }
    struct movie *list = processFile(argv[1]);
    //printMovieList(list);
    int cont = 0;

    // while the program runs, print out instructions and run user choices
    while (cont == 0)
    {
        int i = instructions();
        
        if (i == 1)
        {
            optionOne(list);
        }

        if (i == 2)
        {
            optionTwo(list);
        }

        if (i == 3)
        {
            optionThree(list);
        }

        if (i == 4)
        {
            cont = 1;
        }
    }

    freeMovie(list);
    return EXIT_SUCCESS;
}
