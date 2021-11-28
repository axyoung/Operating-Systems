/*
* -------------------------------------------------------------
*  Author - Alex Young
*  Filename - enc_server.c
*  Created - 3/6/2021
*  OSU Course - CS 344
*  Instructor - Justin Goins
*  Assignment 5
* -------------------------------------------------------------
*  Description: Server program that interacts with enc_client
*  through socket connection to encode a message with a key
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
*   This is the main function that implements the enc_server
*   Compile the program as follows:
*       gcc --std=gnu99 -o enc_server enc_server.c
*   Execute the program using:
*       enc_server <port> &
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
            // Get the message from the client saying that it is the enc_client
            memset(buffer, '\0', 256);
            // Read the client's message from the socket
            charsRead = recv(connectionSocket, buffer, 255, 0); 
            if (charsRead < 0) {
                error("ERROR reading from socket");
            }
            if (strcmp(buffer, "e") == 0) {
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
            char *plaintext_buffer = (char *)malloc(size);
            char *key_buffer = (char *)malloc(size);
            char *cipher_buffer = (char *)malloc(size);
            int total = 0;
            // loop to receive the message from client
            // do this because send might not send the full message at once
            do {
                bytesReceived = recv(connectionSocket, plaintext_buffer + total, size - total, 0); 
                total = total + bytesReceived;
                //printf("enc_server bytes received from client: %i and file size: %i\n", bytesReceived, size);
                if (bytesReceived < 0){
                    error("CLIENT: ERROR reading plaintext from socket");
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

            // encode the message and key into the cipher
            // the loop will individually encode every byte in message by using
            // ascii arithmetic and the mod 27 (an extra for the space char)
            // to perform a one time pad
            char message;
            char key;
            for (int i = 0; i < size; i++) {
                if (plaintext_buffer[i] == ' ') {
                    message = 26;
                }
                else {
                    message = plaintext_buffer[i] - 65;
                }
                if (key_buffer[i] == ' ') {
                    key = 26;
                }
                else {
                    key = key_buffer[i] - 65;
                }
                cipher_buffer[i] = (message + key) % 27;
                if (cipher_buffer[i] == 26) {
                    cipher_buffer[i] = ' ';
                }
                else {
                    cipher_buffer[i] = cipher_buffer[i] + 65;
                }
            }

            // loop to send the encoded ciphertext to the client
            // do this because send might not send the full message at once
            int bytesSent = 0;
            do {
                charsRead = send(connectionSocket, cipher_buffer + bytesSent, size - bytesSent, 0); 
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