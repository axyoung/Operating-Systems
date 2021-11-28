/*
* -------------------------------------------------------------
*  Author - Alex Young
*  Filename - keygen.c
*  Created - 3/6/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 5
* -------------------------------------------------------------
*  Description: Creates a key to stdout with the size argv[1]
* -------------------------------------------------------------
*/

/* include header files with libraries that are used */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

/*
*   This is the main function that implements the keygen
*   Compile the program as follows:
*       gcc --std=gnu99 -o keygen keygen.c
*   Execute the program using:
*       keygen <keysize>
*/
int main(int argc, char *argv[]) {
    srand(time(NULL));
    // convert input to length of key
    int len = strtol(argv[1], NULL, 10);
    // check for valid input to keygen
    if (len < 1 || argc > 2) {
        fprintf(stderr, "KEYGEN: Input Error\n");
        return 1;
    }
    // dynamically allocate a key string
    char *key = (char *)malloc(len);
    // use rand() and ascii math to generate random key values
    for (int i = 0; i < len; i++) {
        int random = ((rand() % 27) + 65);
        if (random == 91) {
            key[i] = ' ';
        }
        else {
            key[i] = random;
        }
    }
    key[len] = '\0';
    // write the key to stdout
    printf("%s\n", key);
    return 0;
}