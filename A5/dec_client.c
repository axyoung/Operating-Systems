/*
* -------------------------------------------------------------
*  Author - Alex Young
*  Filename - dec_client.c
*  Created - 3/6/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 5
* -------------------------------------------------------------
*  Description: Client program that interacts with dec_server
*  to decode a ciphertext and print the message to stdout
* -------------------------------------------------------------
*/

/* include header files with libraries that are used */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>  // ssize_t
#include <sys/socket.h> // send(),recv()
#include <netdb.h>      // gethostbyname()

// Error function used for reporting issues
void error(const char *msg) { 
    perror(msg); 
    exit(1); 
} 

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber, 
                        char* hostname){
 
    // Clear out the address struct
    memset((char*) address, '\0', sizeof(*address)); 

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);

    // Get the DNS entry for this host name
    struct hostent* hostInfo = gethostbyname(hostname); 
    if (hostInfo == NULL) { 
        fprintf(stderr, "CLIENT: ERROR - no such host\n"); 
        exit(0); 
    }
    // Copy the first IP address from the DNS entry to sin_addr.s_addr
    memcpy((char*) &address->sin_addr.s_addr, 
            hostInfo->h_addr_list[0],
            hostInfo->h_length);
}

/*
*   This is the main function that implements the dec_client
*   Compile the program as follows:
*       gcc --std=gnu99 -o dec_client dec_client.c
*   Execute the program using:
*       dec_client <cipher> <key> <port>
*/
int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    // Check usage & args
    if (argc < 3) { 
        fprintf(stderr,"USAGE: %s hostname port\n", argv[0]); 
        exit(0);
    } 

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFD < 0){
        error("CLIENT: ERROR opening socket");
    }

   // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        error("CLIENT: ERROR connecting");
    }

    // Send message to server
    // Write to the server that it is the dec_client
    charsWritten = send(socketFD, "d", 1, 0); 
    if (charsWritten < 0){
        error("CLIENT: ERROR writing to socket");
    }
    if (charsWritten < 1) {
        error("CLIENT: WARNING: Not all data written to socket!");
    }

    char buffer[256];
    // Get return message from server that will confirm that we are connected to the right server
    // Clear out the buffer again for reuse
    memset(buffer, '\0', sizeof(buffer));
    // Read data from the socket, leaving \0 at end
    charsRead = recv(socketFD, buffer, 1, 0); 
    if (charsRead < 0){
        error("CLIENT: ERROR reading from socket");
    }
    
    // check if we are connected to the dec_server, if not, exit with value of 2 and print to stderr
    if (!atoi(buffer)) {
        fprintf(stderr, "CLIENT: ERROR - dec_client cannot use enc_server\n");
        exit(2);
    }
    
    int fd;
    // open ciphertext file so that we can read in the cipher
    fd = open(argv[1], O_RDONLY);
	if (fd == -1){
		perror("Error");
		exit(1);
	}
    // use fstat to get the filesize of the cipher which will be used to dynamically size the buffers
    struct stat ciphertext_stat;
    fstat(fd, &ciphertext_stat);
    int ciphertext_size = ciphertext_stat.st_size - 1;
    // set the size to the filesize - 1 to get rid of the \n
    char *ciphertext_buffer = (char *)malloc(ciphertext_size);
    // the ciphertext_size buffer will just hold the cipher size so that it can be sent to the server so it can correctly create its own buffers
    char ciphertext_size_buffer[256];
    // Clear out the buffer again for reuse
    memset(ciphertext_size_buffer, '\0', sizeof(ciphertext_size_buffer));
    sprintf(ciphertext_size_buffer, "%i", ciphertext_size);
    // Clear out the buffer array
    memset(ciphertext_buffer, '\0', ciphertext_size);
    // Get input from the cipher
    int bytes_read = read(fd, ciphertext_buffer, ciphertext_size);
    if (bytes_read <= 0) {
        fprintf(stderr, "CLIENT: ERROR - %s bytes read less than or equal to zero\n", argv[1]);
        exit(1);
    }
    // loop through the message to ensure that the input is correctly formatted
    for (int i = 0; i < ciphertext_size; i++) {
        if(!isupper(ciphertext_buffer[i]) && ciphertext_buffer[i] != ' ') {
            close(fd);
            fprintf(stderr, "CLIENT: ERROR - %s contains bad characters\n", argv[1]);
            exit(1);
        }
    }
    close(fd);

    // open key file to read in the key
    fd = open(argv[2], O_RDONLY);
	if (fd == -1){
		perror("Error");
		exit(1);
	}
    // use fstat to get the filesize of the key which will be used to dynamically size the buffers
    struct stat key_stat;
    fstat(fd, &key_stat);
    // set the size to the filesize - 1 so we can correctly compare it to message size to ensure the key isnt too small
    int key_size = key_stat.st_size - 1;
    char *key_buffer = (char *)malloc(ciphertext_size);
    // Clear out the buffer array
    memset(key_buffer, '\0', ciphertext_size);
    // Get input from the key file
    bytes_read = read(fd, key_buffer, ciphertext_size);
    if (bytes_read <= 0) {
        fprintf(stderr, "CLIENT: ERROR - key bytes read less than or equal to zero\n");
        exit(1);
    }
    // make sure that the key is long enough
    if (key_size < ciphertext_size) {
        fprintf(stderr, "CLIENT: ERROR - key '%s' is too short\n", argv[2]);
        exit(1);
    }
    // loop through the message to ensure that the input is correctly formatted
    for (int i = 0; i < ciphertext_size; i++) {
        if(!isupper(key_buffer[i]) && key_buffer[i] != ' ') {
            close(fd);
            fprintf(stderr, "CLIENT: ERROR - key contains bad characters\n");
            exit(1);
        }
    }
    close(fd);

    // Send message to server with the ciphertext length
    // Write to the server
    charsWritten = send(socketFD, ciphertext_size_buffer, sizeof(ciphertext_size_buffer) - 1, 0); 
    if (charsWritten < 0){
        error("CLIENT: ERROR writing to socket");
    }
    if (charsWritten < strlen(buffer)){
        error("CLIENT: WARNING: Not all data written to socket!");
    }

    // Send message to server with the ciphertext buffer
    // Write to the server

    // loop to send the cipher to be decoded to the server
    // do this because send might not send the full message at once
    int bytesSent = 0;
    do {
        charsWritten = send(socketFD, ciphertext_buffer + bytesSent, ciphertext_size - bytesSent, 0); 
        bytesSent = bytesSent + charsWritten;
        if (charsWritten < 0){
            error("CLIENT: ERROR writing to socket");
            exit(2);
        }
    } while (bytesSent < ciphertext_size);
    
    // loop to send the key to the server
    // do this because send might not send the full message at once
    bytesSent = 0;
    do {
        charsWritten = send(socketFD, key_buffer + bytesSent, ciphertext_size - bytesSent, 0); 
        bytesSent = bytesSent + charsWritten;
        if (charsWritten < 0){
            error("CLIENT: ERROR writing to socket");
            exit(2);
        }
    } while (bytesSent < ciphertext_size);
    
    // Get return message from server
    char *message_buffer = (char *)malloc(ciphertext_size + 1);
    // Clear out the buffer again for reuse
    memset(message_buffer, '\0', ciphertext_size + 1);
    // loop to receive the message from the server
    // do this because send might not send the full message at once
    int total = 0;
    do {
        charsRead = recv(socketFD, message_buffer + total, ciphertext_size - total, 0); 
        total = total + charsRead;
        if (charsRead < 0){
            error("CLIENT: ERROR reading message from socket");
        }
    } while (total < ciphertext_size);
    printf("%s\n", message_buffer);

    // Close the socket
    close(socketFD); 
    return 0;
}