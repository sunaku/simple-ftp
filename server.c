/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP server implementation.
**/

#include "service.h"
#include "server.h"

#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// globals
	char g_pwd[PATH_MAX+1];

// funcs
int main(int a_argc, char **ap_argv)
{
	// variables
		int serverSocket, clientSocket, clientAddrSize;
		struct sockaddr_in clientAddr;
		
	// check args
		if(a_argc < 3)
		{
			printf("Usage: %s dir_name port_number\n", ap_argv[0]);
			return 1;
		}
		
	// init vars
		realpath(ap_argv[1], g_pwd);
		
	// create service
		if(!service_create(&serverSocket, strtol(ap_argv[2], (char**)NULL, 10)))
		{
			fprintf(stderr, "%s: unable to create service on port: %s\n", ap_argv[0], ap_argv[2]);
			return 2;
		}
	
	// dispatcher loop
		while(true)
		{
			clientAddrSize = sizeof(clientAddr);
			
			// wait for a client
				if((clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize)) != -1)
				{
					#ifndef NODEBUG
						printf("\nmain(): got client connection [addr=%s,port=%d]\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
					#endif
					
				// dispatch job
					if(fork() == 0) // child code
					{
						// service the client
							if(session_create(clientSocket))
								service_loop(clientSocket);
							
							session_destroy(clientSocket);
						
						// finished with client
							close(clientSocket);
							return 0;
					}
				}
				else
					perror("main()");
		}
		
	// destroy service
		close(serverSocket);

	return 0;
}

Boolean service_create(int *ap_socket, const int a_port)
{
	// variables
		struct sockaddr_in serverAddr;
		int yes=1;
		
	// create address
		memset(&serverAddr, 0, sizeof(serverAddr));
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr.sin_port = htons(a_port);
		
	// create socket
		if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
		{
			perror("service_create(): create socket");
			return false;
		}
		
	// set options
		if(setsockopt(*ap_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
		{
			perror("service_create(): socket opts");
			return false;
		}
	
	// bind socket
		if(bind(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
		{
			perror("service_create(): bind socket");
			close(*ap_socket);
			return false;
		}
		
	// listen to socket
		if(listen(*ap_socket, SERVER_SOCKET_BACKLOG) < 0)
		{
			perror("service_create(): listen socket");
			close(*ap_socket);
			return false;
		}
	
	return true;
}

Boolean session_create(const int a_socket)
{
	// variables
		Message msgOut, msgIn;
		
	// init vars
		Message_clear(&msgOut);
		Message_clear(&msgIn);
		
	// session challenge dialogue
	
		// client: greeting
			if(!siftp_recv(a_socket, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_SESSION_BEGIN))
			{
				fprintf(stderr, "service_create(): session not requested by client.\n");
				return false;
			}
			
		// server: identify
		// client: username
			Message_setType(&msgOut, SIFTP_VERBS_IDENTIFY);
			
			if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_USERNAME))
			{
				fprintf(stderr, "service_create(): username not specified by client.\n");
				return false;
			}
		
		// server: accept|deny
		// client: password
			Message_setType(&msgOut, SIFTP_VERBS_ACCEPTED); //XXX check username... not required for this project
			
			if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_PASSWORD))
			{
				fprintf(stderr, "service_create(): password not specified by client.\n");
				return false;
			}
		
		// server: accept|deny
			if(Message_hasValue(&msgIn, SERVER_PASSWORD))
			{
				Message_setType(&msgOut, SIFTP_VERBS_ACCEPTED);
				siftp_send(a_socket, &msgOut);
			}
			else
			{
				Message_setType(&msgOut, SIFTP_VERBS_DENIED);
				siftp_send(a_socket, &msgOut);
				
				fprintf(stderr, "service_create(): client password rejected.\n");
				return false;
			}
		
		// session now established
			#ifndef NODEBUG
				printf("session_create(): success\n");
			#endif
			
	return true;
}

void service_loop(const int a_socket)
{
	// variables
		Message msg;
		
		String *p_argv;
		int argc;
		
	// init vars
		Message_clear(&msg);
	
	while(siftp_recv(a_socket, &msg)) // await request
	{
		if(Message_hasType(&msg, SIFTP_VERBS_SESSION_END) || Message_hasType(&msg, ""))
			break;
		
		else
		{
			#ifndef NODEBUG
				printf("service_loop(): got command '%s'\n", Message_getValue(&msg));
			#endif
			
			// parse request
				if((p_argv = service_parseArgs(Message_getValue(&msg), &argc)) == NULL || argc <= 0)
				{
					service_freeArgs(p_argv, argc);
					
					if(!service_sendStatus(a_socket, false)) // send negative ack
						break;
					
					continue;
				}
				
			// handle request
				if(!service_handleCmd(a_socket, p_argv, argc))
					service_sendStatus(a_socket, false); // send negative ack upon fail
				
			// clean up
				service_freeArgs(p_argv, argc);
				p_argv = NULL;
				
				/*
				if(!service_handleCmd(a_socket, Message_getValue(&msg))) // send failure notification
				{
					Message_setType(&msg, SIFTP_VERBS_ABORT);
					
					if(!siftp_send(a_socket, &msg))
					{
						fprintf(stderr, "service_loop(): unable to send faliure notice.\n");
						break;
					}
				}
				*/
		}
	}
}


Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc)
{
	// variables
		Message msg;
		
		String dataBuf;
		int dataBufLen;
		
		Boolean tempStatus = false;
		
	// init variables
		Message_clear(&msg);
		
	// handle commands
	if(strcmp(ap_argv[0], "ls") == 0)
	{
		if((dataBuf = service_readDir(g_pwd, &dataBufLen)) != NULL)
		{
			// transmit data
				if(service_sendStatus(a_socket, true))
					tempStatus = siftp_sendData(a_socket, dataBuf, dataBufLen);
				
				#ifndef NODEBUG
					printf("ls(): status=%d\n", tempStatus);
				#endif
				
			// clean up
				free(dataBuf);
				
			return tempStatus;
		}
	}
	
	else if(strcmp(ap_argv[0], "pwd") == 0)
	{
		if(service_sendStatus(a_socket, true))
			return siftp_sendData(a_socket, g_pwd, strlen(g_pwd));
	}
	
	else if(strcmp(ap_argv[0], "cd") == 0 && a_argc > 1)
	{
		return service_sendStatus(a_socket, service_handleCmd_chdir(g_pwd, ap_argv[1]));
	}
	
	else if(strcmp(ap_argv[0], "get") == 0 && a_argc > 1)
	{
		char srcPath[PATH_MAX+1];
		
		// determine absolute path
		if(service_getAbsolutePath(g_pwd, ap_argv[1], srcPath))
		{
			// check read perms & file type
			if(service_permTest(srcPath, SERVICE_PERMS_READ_TEST) && service_statTest(srcPath, S_IFMT, S_IFREG))
			{
				// read file
				if((dataBuf = service_readFile(srcPath, &dataBufLen)) != NULL)
				{
					if(service_sendStatus(a_socket, true))
					{
						// send file
						tempStatus = siftp_sendData(a_socket, dataBuf, dataBufLen);
						
						#ifndef NODEBUG 
							printf("get(): sent file '%s'.\n", srcPath);
						#endif
					}
					
					free(dataBuf);
				}
			}
		}
			
		return tempStatus;
	}
	
	else if(strcmp(ap_argv[0], "put") == 0 && a_argc > 1)
	{
		char dstPath[PATH_MAX+1];
		
		// determine destination file path
		if(service_getAbsolutePath(g_pwd, ap_argv[1], dstPath))
		{
			// check write perms & file type
			if(service_permTest(dstPath, SERVICE_PERMS_WRITE_TEST) && service_statTest(dstPath, S_IFMT, S_IFREG))
			{
				// send primary ack: file perms OK
				if(service_sendStatus(a_socket, true))
				{
					// receive file
					if((dataBuf = siftp_recvData(a_socket, &dataBufLen)) != NULL)
					{
						tempStatus = service_writeFile(dstPath, dataBuf, dataBufLen);
						
						free(dataBuf);
						
						// send secondary ack: file was written OK
						if(tempStatus)
						{
							return service_sendStatus(a_socket, true);
						}
					}
				}
			}
		}
	}
	
	return false;
}
