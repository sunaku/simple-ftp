/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP server implementation.
**/

#include "model.h"
#include "server.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>

// globals
	char g_pwd[PATH_MAX+1];

// funcs
int main(int a_argc, char **ap_argv)
{
	// variables
		int serverSocket, clientSocket, clientSocketSize;
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
		if(!service_create(&serverSocket, ap_argv[2]))
		{
			fprintf(stderr, "%s: unable to create service on port: %s\n", ap_argv[0], ap_argv[2]);
			return 2;
		}
	
	// dispatcher loop
		while(1)
		{
			// wait for a client
				clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientSocketSize);
			
				#ifndef NODEBUG
					printf("\nmain(): got client connection [addr=%x,port=%d]\n", ntohl(clientAddr.sin_addr.s_addr), ntohs(clientAddr.sin_port));
				#endif
				
			// dispatch job
	//			if(fork() == 0) // child code
	//			{
					// service the client
						if(session_create(clientSocket))
							service_loop(clientSocket);
						
						session_destroy(clientSocket);
					
					// finished with client
						close(clientSocket);
						return 0;
	//			}
		}
		
	// destroy service
		close(serverSocket);

	return 0;
}

Boolean service_create(int *ap_socket, const String a_port)
{
	// variables
		struct sockaddr_in serverAddr;
		
	// create address
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		serverAddr.sin_port = htons(strtol(a_port, (char**)NULL, 10));
		
	// create socket
		if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
		{
			perror("service_create()");
			return false;
		}
	
	// bind socket
		if(bind(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr)) < 0)
		{
			perror("service_create()");
			close(*ap_socket);
			return false;
		}
		
	// listen to socket
		if(listen(*ap_socket, SERVER_SOCKET_BACKLOG) < 0)
		{
			perror("service_create()");
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
			
			if(!siftp_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_USERNAME))
			{
				fprintf(stderr, "service_create(): username not specified by client.\n");
				return false;
			}
		
		// server: accept|deny
		// client: password
			Message_setType(&msgOut, SIFTP_VERBS_ACCEPTED); //XXX check username... not required for this project
			
			if(!siftp_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_PASSWORD))
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
		
	// init vars
		Message_clear(&msg);
	
	while(siftp_recv(a_socket, &msg)) // await request
	{
		if(Message_hasType(&msg, SIFTP_VERBS_SESSION_END) || Message_hasType(&msg, ""))
			break;
		
		else
		{
			// parse request
				#ifndef NODEBUG
					printf("service_loop(): got command {str='%s',arg='%s'}\n", Message_getValue(&msg), strchr(Message_getValue(&msg), ' '));
				#endif
				
			// handle request
				service_command(a_socket, Message_getValue(&msg));
				/*
				if(!service_command(a_socket, Message_getValue(&msg))) // send failure notification
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


Boolean service_command(const int a_socket, const String a_cmdStr)
{
	// variables
		Message msgOut, msgIn;
		Boolean cmdStatus;
		
		char cmdName[MODEL_COMMAND_SIZE+1];
		String cmdArg, dataBuf;
		int dataBufLen;
		
		DIR *p_dirFd;
		struct dirent *p_dirInfo;
		
	// init variables
		Message_clear(&msgOut);
		Message_clear(&msgIn);
		
		// command name
			strncpy(cmdName, a_cmdStr, MODEL_COMMAND_SIZE);
			cmdName[MODEL_COMMAND_SIZE] = '\0';
		
		// argument string
			if((cmdArg = strchr(a_cmdStr, ' ')) != NULL)
				cmdArg += sizeof(char);
		
		cmdStatus = true;
	
	if(strstr(cmdName, "ls"))
	{
		int dataBufIndex;
		
		// validate command
			if((p_dirFd = opendir(g_pwd)) == NULL)
			{
				perror(a_cmdStr);
				cmdStatus = false;
			}
			
		// send status notification
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND_STATUS);
			Message_getValue(&msgOut)[0] = cmdStatus;
			
			if(!siftp_send(a_socket, &msgOut) || !cmdStatus)
				return false;
		
		// determine buffer size
			dataBufLen=0;
			
			while((p_dirInfo = readdir(p_dirFd)))
				dataBufLen += strlen(p_dirInfo->d_name) + 1;
			
			#ifndef NODEBUG
				printf("cmd_ls(): buffer size = %d\n", dataBufLen);
			#endif
			
			rewinddir(p_dirFd);
			
		// allocate buffer
			if((dataBuf = calloc(dataBufLen, sizeof(char))) == NULL)
			{
				closedir(p_dirFd);
				fprintf(stderr, "cmd_ls(): calloc() failed.\n");
				return false;
			}
		
		// read contents into buffer
			dataBufIndex=0;
			
			while((p_dirInfo = readdir(p_dirFd)))
			{
				strcpy(&dataBuf[dataBufIndex], p_dirInfo->d_name);
				
				#ifndef NODEBUG
					printf("cmd_ls(): buffer[%d]='%s'\n", dataBufIndex, &dataBuf[dataBufIndex]);
				#endif
				
				dataBufIndex += strlen(p_dirInfo->d_name);
				dataBuf[dataBufIndex++] = '\n'; // overwrite null term
				
				#ifndef NODEBUG
					printf("cmd_ls(): buffer {index=%d,val='%s'}\n", dataBufIndex, dataBuf);
				#endif
			}
			closedir(p_dirFd);
			
		// transmit data
			cmdStatus = siftp_sendData(a_socket, dataBuf, dataBufLen);
			
		// clean up
			free(dataBuf);
			dataBuf = NULL;
	}
	
	else if(strstr(cmdName, "pwd"))
	{
		// send status notification
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND_STATUS);
			Message_getValue(&msgOut)[0] = true;
			
			if(!siftp_send(a_socket, &msgOut))
				return false;
		
		// transmit data
			cmdStatus = siftp_sendData(a_socket, g_pwd, strlen(g_pwd));
	}
	
	else if(strstr(cmdName, "cd"))
	{
		cmdStatus = service_command_chdir(a_cmdStr, cmdArg, g_pwd);
		
		// send status notification
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND_STATUS);
			Message_getValue(&msgOut)[0] = cmdStatus;
			
			if(!siftp_send(a_socket, &msgOut))
				return false;
	}
	
	else
		cmdStatus = false;
	
	return cmdStatus;
}
