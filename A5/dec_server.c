/*
* -------------------------------------------------------------
*  Author - Alex Young
*  Filename - dec_server.c
*  Created - 3/6/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 5
* -------------------------------------------------------------
*  Description: Server program that interacts with dec_client
*  through socket connection to decode a ciphertext with a key
* -------------------------------------------------------------
*/

/* include header files with libraries that are used */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>

// Error function used for reporting issues
void error(const char *msg) {
  perror(msg);
  exit(1);
} 

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, 
                        int portNumber){
 
    // Clear out the address struct
    memset((char*) address, '\0', sizeof(*address)); 

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

/*
*   This is the main function that implements the dec_server
*   Compile the program as follows:
*       gcc --std=gnu99 -o dec_server dec_server.c
*   Execute the program using:
*       dec_server <port> &
*/
int main(int argc, char *argv[]){
    int connectionSocket, charsRead;
    char buffer[256];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);

    // Check usage & args
    if (argc < 2) { 
        fprintf(stderr,"USAGE: %s port\n", argv[0]); 
        exit(1);
    } 
  
    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, 
            (struct sockaddr *)&serverAddress, 
            sizeof(serverAddress)) < 0){
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5); 
  
    // Accept a connection, blocking if one is not available until one connects
    while(1) {
        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket, 
                    (struct sockaddr *)&clientAddress, 
                    &sizeOfClientInfo); 
        if (connectionSocket < 0){
            error("ERROR on accept");
        }

        // after accepting connection, fork to a child
        pid_t child_server = fork();

        // if we aren't the child or the parent, error
        if (child_server < 0) {
            perror("Error");
            exit(0);
        }
        // Case where we are the child process that has been forked to
        else if (child_server == 0) {
            // Get the message from the client saying that it is the dec_client
            memset(buffer, '\0', 256);
            // Read the client's message from the socket
            charsRead = recv(connectionSocket, buffer, 255, 0); 
            if (charsRead < 0) {
                error("ERROR reading from socket");
            }
            if (strcmp(buffer, "d") == 0) {
                // Send a Success message back to the client
                charsRead = send(connectionSocket, "1", 1, 0); 
                if (charsRead < 0) {
                    error("ERROR writing to socket");
                }
            }
            else {
                // Send a Failure message back to the client
                charsRead = send(connectionSocket, "0", 1, 0); 
                if (charsRead < 0) {
                    error("ERROR writing to socket");
                }
            }

            // read in the size of the files
            memset(buffer, '\0', 256);
            charsRead = recv(connectionSocket, buffer, 255, 0);
            if (charsRead < 0) {
                error("ERROR reading from socket");
            }

            // save the size of the file and create buffers to hold the message, key, and cipher
            int size = atoi(buffer);
            int bytesReceived;
            char *ciphertext_buffer = (char *)malloc(size);
            char *key_buffer = (char *)malloc(size);
            char *message_buffer = (char *)malloc(size);
            int total = 0;
            // loop to receive the ciphertext from client
            // do this because send might not send the full message at once
            do {
                bytesReceived = recv(connectionSocket, ciphertext_buffer + total, size - total, 0); 
                total = total + bytesReceived;
                if (bytesReceived < 0){
                    error("CLIENT: ERROR reading ciphertext from socket");
                }
            } while (total < size);

            // loop to receive the key from client
            // do this because send might not send the full message at once
            total = 0;
            do {
                bytesReceived = recv(connectionSocket, key_buffer + total, size - total, 0); 
                total = total + bytesReceived;
                if (bytesReceived < 0){
                    error("CLIENT: ERROR reading key from socket");
                }
            } while (total < size);

            // decode the cipher and key into the messgae
            // the loop will individually encode every byte in message by using
            // ascii arithmetic and the mod 27 (an extra for the space char)
            // to decode the one time pad
            char cipher;
            char key;
            for (int i = 0; i < size; i++) {
                if (ciphertext_buffer[i] == ' ') {
                    cipher = 26;
                }
                else {
                    cipher = ciphertext_buffer[i] - 65;
                }
                if (key_buffer[i] == ' ') {
                    key = 26;
                }
                else {
                    key = key_buffer[i] - 65;
                }
                message_buffer[i] = (cipher - key + 27) % 27;
                if (message_buffer[i] == 26) {
                    message_buffer[i] = ' ';
                }
                else {
                    message_buffer[i] = message_buffer[i] + 65;
                }
            }

            // loop to send the decoded message to the client
            // do this because send might not send the full message at once
            int bytesSent = 0;
            do {
                charsRead = send(connectionSocket, message_buffer + bytesSent, size - bytesSent, 0); 
                bytesSent = bytesSent + charsRead;
                if (charsRead < 0) {
                    error("CLIENT: ERROR writing to socket");
                }
            } while (bytesSent < size);


            // Close the connection socket for this client
            close(connectionSocket);
            exit(0);
        }
        // Close the connection socket for this client
        close(connectionSocket);
        // Kill zombie processes
        waitpid(-1, NULL, WNOHANG);
    }
    // Close the listening socket
    close(listenSocket); 
    return 0;
}