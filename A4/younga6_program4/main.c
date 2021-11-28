/*
* -------------------------------------------------------------
*  Author - Alex Young
*  Filename - main.c
*  Created - 2/12/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 4: Multi-threaded Producer Consumer Pipeline
* -------------------------------------------------------------
*  Description: This program creates 4 threads to process
*  input from standard input. The 4 threads are used to 
*  read in lines of characters, replace line separators with
*  spaces, replace pairs of plus signs with a ^, and write
*  the processed data to standard out as lines of 80 characters.
* -------------------------------------------------------------
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>


// Size of the buffers, add extra space for manually inserting a STOP if necesary at end of a file
#define MAX_LINES 52
#define MAX_CHARS 1001

// Number of output line that will be produced. This number is less than the size of the buffer. Hence, we can model the buffer as being unbounded.
#define NUM_ITEMS 80

// Buffer 1, shared resource between input thread and Line Seperator thread
char buffer_1[MAX_LINES][MAX_CHARS];
// Number of items in the buffer
int count_1 = 0;
// Index where the input thread will put the next item
int prod_1_line_idx = 0;
int prod_1_char_idx = 0;
// Index where the Line Seperator thread will pick up the next item
int con_1_line_idx = 0;
int con_1_char_idx = 0;
// Initialize the mutex for buffer 1
pthread_mutex_t mutex_1 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 1
pthread_cond_t full_1 = PTHREAD_COND_INITIALIZER;

// Buffer 2, shared resource between Line Seperator thread and Plus Sign thread
char buffer_2[MAX_LINES][MAX_CHARS];
// Number of items in the buffer
int count_2 = 0;
// Index where the Line Seperator thread will put the next item
int prod_2_line_idx = 0;
int prod_2_char_idx = 0;
// Index where the Plus Sign thread will pick up the next item
int con_2_line_idx = 0;
int con_2_char_idx = 0;
// Initialize the mutex for buffer 2
pthread_mutex_t mutex_2 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 2
pthread_cond_t full_2 = PTHREAD_COND_INITIALIZER;

// Buffer 3, shared resource between Plus Sign thread and Output Thread
char buffer_3[MAX_LINES][MAX_CHARS];
// Number of items in the buffer
int count_3 = 0;
// Index where the Plus Sign thread will put the next item
int prod_3_line_idx = 0;
int prod_3_char_idx = 0;
// Index where the Output Thread will pick up the next item
int con_3_line_idx = 0;
int con_3_char_idx = 0;
// Initialize the mutex for buffer 3
pthread_mutex_t mutex_3 = PTHREAD_MUTEX_INITIALIZER;
// Initialize the condition variable for buffer 3
pthread_cond_t full_3 = PTHREAD_COND_INITIALIZER;

// Array that holds the length of buffer 1 for each line
int buffer_len[MAX_LINES] = {0};
// Array that holds the number of ++ pairs for each line
int plus_count[MAX_LINES] = {0};
// Variables that keep track of when each thread should stop
int STOP = 0;
int STOP1 = 0;
int STOP2 = 0;
int STOP3 = 0;
int noSTOP = 1;

/*
Get input from the user.
This function doesn't perform any error checking and only reads in a single character.
*/
char get_user_input() {
    char input;
    // scanf returns the number of input items successfully matched
    noSTOP = scanf("%c", &input);
    return input;
}

/*
 Put an item in buff_1
 - expanded from example code
*/
void put_buff_1(char item) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_1);
    // Put the item in the buffer
    buffer_1[prod_1_line_idx][prod_1_char_idx] = item;
    // Increment the index where the next item will be put.
    prod_1_char_idx++;
    // At the end of a line, check if the line is a STOP-processing line, if so let the producer know
    if (item == '\n') {
        if (strcmp("STOP\n", buffer_1[prod_1_line_idx]) == 0) {
            STOP = 1;
        }
        // Record the line length to use later to check if we are at the end of a line
        buffer_len[prod_1_line_idx] = prod_1_char_idx;
        prod_1_line_idx++;
        prod_1_char_idx = 0;
    }
    count_1++;

    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_1);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
}

/*
 Function that the input thread will run.
 Get input from the user.
 Put the item in the buffer shared with the line separator thread.
  - expanded from example code
*/
void *get_input(void *args) {
    for (int i = 0; i < MAX_LINES; i++) {
        for (int j = 0; j < MAX_CHARS; j++) {
            // Get the user input
            char item = get_user_input();
            // This is for handling input files without a STOP-processing line
            // This will also work for ctrl-d in the terminal
            if (noSTOP <= 0) {
                put_buff_1('\n');
                put_buff_1('S');
                put_buff_1('T');
                put_buff_1('O');
                put_buff_1('P');
                put_buff_1('\n');
                return NULL;
            }
            put_buff_1(item);
            // If the stop-processing line has been found, we are done after putting the STOP in the buffer
            if (STOP) {
                return NULL;
            }
        }
    }
    return NULL;
}

/*
Get the next item from buffer 1
- expanded from example code
*/
char get_buff_1() {
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_1);
    while (count_1 == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_1, &mutex_1);
    char item = buffer_1[con_1_line_idx][con_1_char_idx];
    // Increment the index from which the item will be picked up
    con_1_char_idx++;
    count_1--;
    // At the end of a line, check if the line is a STOP-processing line, if so let the consumer know
    if (con_1_char_idx == buffer_len[con_1_line_idx] && buffer_len[con_1_line_idx] != 0) {
        if (strcmp("STOP\n", buffer_1[con_1_line_idx]) == 0) {
            STOP1 = 1;
        }
        con_1_line_idx++;
        con_1_char_idx = 0;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_1);
    // Return the item
    return item;
}

/*
 Put an item in buff_2
 - expanded from example code
*/
void put_buff_2(char item) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_2);
    // Put the item in the buffer
    buffer_2[prod_2_line_idx][prod_2_char_idx] = item;
    // Increment the index where the next item will be put.
    prod_2_char_idx++;
    if (prod_2_char_idx == buffer_len[prod_2_line_idx] && buffer_len[prod_2_line_idx] != 0) {
        prod_2_line_idx++;
        prod_2_char_idx = 0;
    }
    count_2++;

    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_2);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
}

/*
 Function that the line separator thread will run.
 Turns newline characters into spaces
 Put the new item in the buffer shared with the plus sign thread.
*/
void *separate_lines(void *args) {
    // While the stop-processing line hasn't been seen get the next item and put it into buffer 2
    while (!STOP1) {
        char item = get_buff_1();
        if (item == '\n') {
            item = ' ';
        }
        put_buff_2(item);
    }
    return NULL;
}

/*
Get the next item from buffer 2
- expanded from example code
*/
char get_buff_2() {
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_2);
    while (count_2 == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_2, &mutex_2);
    char item = buffer_2[con_2_line_idx][con_2_char_idx];
    // Increment the index from which the item will be picked up
    con_2_char_idx++;
    count_2--;
    // At the end of a line, check if the line is a STOP-processing line, if so let the consumer know
    if (con_2_char_idx == buffer_len[con_2_line_idx] && buffer_len[con_2_line_idx] != 0) {
        if (strcmp("STOP ", buffer_2[con_2_line_idx]) == 0) {
            STOP2 = 1;
        }
        con_2_line_idx++;
        con_2_char_idx = 0;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_2);
    // Return the item
    return item;
}

/*
 Put an item in buff_3
 - expanded from example code
*/
void put_buff_3(char item, int inc_count) {
    // Lock the mutex before putting the item in the buffer
    pthread_mutex_lock(&mutex_3);
    // Put the item in the buffer
    buffer_3[prod_3_line_idx][prod_3_char_idx] = item;
    // Increment the index where the next item will be put.
    prod_3_char_idx++;
    // Note that the length of a line in buffer 2 is the buffer 1 length minus the number of ++ pairs
    if (prod_3_char_idx + plus_count[prod_3_line_idx] == buffer_len[prod_3_line_idx] && buffer_len[prod_3_line_idx] != 0) {
        prod_3_line_idx++;
        prod_3_char_idx = 0;
    }
    count_3++;
    // If there was a ++ pair, we record it, this is useful for checking the new line length in the consumer thread
    if (inc_count) {
        plus_count[prod_3_line_idx]++;
    }
    if (prod_3_char_idx == 0) {
        plus_count[prod_3_line_idx] = 0;
    }

    // Signal to the consumer that the buffer is no longer empty
    pthread_cond_signal(&full_3);
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
}

/*
 Function that the plus sign thread will run.
 Turns ++ character pairs into a ^
 Put the new item in the buffer shared with the write output thread.
*/
void *replace_plus(void *args) {
    // The state will be a 1 when the previous character was a +
    int state = 0;
    // While the stop-processing line has not been seen, get the next item, process it, and put it into buffer 3
    while (!STOP2) {
        char item = get_buff_2();
        if (item == '+') {
            // ++ case, puts ^ into the next buffer
            if (state == 1) {
                put_buff_3('^', 1);
                state = 0;
            }
            // _+ case, waits for the next input before printing a + or ^
            else {
                state = 1;
            }
        }
        else {
            // +_ case, need to put a + and the item into the next buffer
            if (state == 1) {
                put_buff_3('+', 0);
                put_buff_3(item, 0);
            }
            // __ case, put in the item to the next buffer
            else {
                put_buff_3(item, 0);
            }
            state = 0;
        }
    }
    return NULL;
}

/*
Get the next item from buffer 3
- expanded from example code
*/
char get_buff_3() {
    // Lock the mutex before checking if the buffer has data
    pthread_mutex_lock(&mutex_3);
    while (count_3 == 0)
        // Buffer is empty. Wait for the producer to signal that the buffer has data
        pthread_cond_wait(&full_3, &mutex_3);
    char item = buffer_3[con_3_line_idx][con_3_char_idx];
    // Increment the index from which the item will be picked up
    con_3_char_idx++;
    count_3--;
    // At the end of a line, check if the line is a STOP-processing line, if so let the consumer know
    // Note that the length of a line in buffer 2 is the buffer 1 length minus the number of ++ pairs
    if (con_3_char_idx + plus_count[con_3_line_idx] == buffer_len[con_3_line_idx] && buffer_len[con_3_line_idx] != 0) {
        if (strcmp("STOP ", buffer_3[con_3_line_idx]) == 0) {
            STOP3 = 1;
        }
        con_3_line_idx++;
        con_3_char_idx = 0;
    }
    // Unlock the mutex
    pthread_mutex_unlock(&mutex_3);
    // Return the item
    return item;
}


/*
 Function that the output thread will run. 
 Consume an item from the buffer shared with the replace plus thread.
 Print the items 80 characters at a time.
*/
void *write_output(void *args) {
    while (1) {
        // Set a temporary buffer to read in 80 characters at a time (with a null terminating character)
        char temp_buff[NUM_ITEMS + 1] = {0};
        // For all 80 characters, insert the item in buffer 3 into the corresponding character
        for (int i = 0; i < NUM_ITEMS; i++) {
            temp_buff[i] = get_buff_3();
            // If the stop-processing line was read, return before printing out the STOP
            if (STOP3) {
                return NULL;
            }
            // If we reach the end of the 80 character output line, print out of the temporary buffer
            if (i == NUM_ITEMS - 1) {
                printf("%s\n", temp_buff);                                                               
                fflush(stdout);
            }
        }
    }
    return NULL;
}

/*
*   This is the main function that implements the smallsh shell
*   Compile the program as follows:
*       gcc --std=gnu99 -lpthread -o line_processor main.c
*   Execute the program using:
*       ./line_processor
*/
int main(int argc, char *argv[]) {
    srand(time(0));
    pthread_t input_t, line_separator_t, plus_sign_t, output_t;
    // Create the threads
    pthread_create(&input_t, NULL, get_input, NULL);
    pthread_create(&line_separator_t, NULL, separate_lines, NULL);
    pthread_create(&plus_sign_t, NULL, replace_plus, NULL);
    pthread_create(&output_t, NULL, write_output, NULL);
    // Wait for the threads to terminate
    pthread_join(input_t, NULL);
    pthread_join(line_separator_t, NULL);
    pthread_join(plus_sign_t, NULL);
    pthread_join(output_t, NULL);
    return EXIT_SUCCESS;
}