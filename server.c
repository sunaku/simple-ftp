/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP server implementation.
**/

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
		Message_reset(&msgOut);
		Message_reset(&msgIn);
		
	// session challenge dialogue
	
		// client: greeting
			if(!siftp_recv(a_socket, &msgIn) || strcmp(msgIn.m_verb, SIFTP_VERBS_SESSION_BEGIN))
			{
				fprintf(stderr, "service_create(): session not requested by client.\n");
				return false;
			}
			
		// server: identify
		// CLIENT: username
			strcpy(msgOut.m_verb, SIFTP_VERBS_IDENTIFY);
			
			if(!siftp_query(a_socket, &msgOut, &msgIn) || strcmp(msgIn.m_verb, SIFTP_VERBS_USERNAME))
			{
				fprintf(stderr, "service_create(): username not specified by client.\n");
				return false;
			}
			
			//XXX check username... not required for this lab
		
		// SERVER: accept|deny
		// CLIENT: password
			strcpy(msgOut.m_verb, SIFTP_VERBS_ACCEPTED);
			
			if(!siftp_query(a_socket, &msgOut, &msgIn) || strcmp(msgIn.m_verb, SIFTP_VERBS_PASSWORD))
			{
				fprintf(stderr, "service_create(): password not specified by client.\n");
				return false;
			}
		
		// SERVER: accept|deny
			if(strcmp(msgIn.m_param, SERVER_PASSWORD) == 0)
			{
				strcpy(msgOut.m_verb, SIFTP_VERBS_ACCEPTED);
				siftp_send(a_socket, &msgOut);
			}
			else
			{
				strcpy(msgOut.m_verb, SIFTP_VERBS_DENIED);
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

inline Boolean session_destroy(const int a_socket)
{
	#ifndef NODEBUG
		printf("session_destroy(): closing session.");
	#endif
		
	// variables
		Message msgOut;

	// init vars
		Message_reset(&msgOut);
		
	// send notice
		strcpy(msgOut.m_verb, SIFTP_VERBS_SESSION_END);
		return siftp_send(a_socket, &msgOut);
}

void service_loop(const int a_socket)
{
	// variables
		Message msg;
		Boolean status;
		char cmd[SERVER_COMMAND_SIZE];
		
	// init vars
		Message_reset(&msg);
	
	
	while(1)
	{
		// await request
			if(siftp_recv(a_socket, &msg) && strcmp(msg.m_verb, SIFTP_VERBS_SESSION_END) != 0)
			{
				// parse request
					strncpy(cmd, msg.m_param, SERVER_COMMAND_SIZE);
					
					#ifndef NODEBUG
						printf("service_loop(): got command '%s'\n", cmd);
					#endif
					
				// handle request
					if(strstr(cmd, "ls"))
						status = cmd_ls(a_socket, msg.m_param);
					
		/*			else if(strstr(cmd, "pwd"))
						status = cmd_pwd(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "cd"))
						status = cmd_cd(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "get"))
						status = cmd_get(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "put"))
						status = cmd_put(a_socket, &msg.m_param);
		*/			
					else // unknown request
					{
						strcpy(msg.m_verb, SIFTP_VERBS_COMMAND_STATUS);
						msg.m_param[0] = false;
						
						siftp_send(a_socket, &msg);
					}
			}
			
			else
				break;
	}
}

Boolean cmd_ls(const int a_socket, const String a_cmdStr)
{
	#ifndef NODEBUG
		printf("cmd_ls(): entering\n");
	#endif
	
	// variables
		Message msgOut;
		
		DIR *p_dirFd;
		struct dirent *p_dirInfo;
		
		String buf = NULL, cmdArg;
		int bufLen, bufIndex;
		Boolean status = true;
		
	// init vars
		Message_reset(&msgOut);
		
	// validate command
		if((cmdArg = strchr(a_cmdStr, ' ')) == NULL)
			cmdArg = g_pwd;
		
		if((p_dirFd = opendir(cmdArg)) == NULL)
		{
			perror("cmd_ls()");
			status = false;
		}
		
		strcpy(msgOut.m_verb, SIFTP_VERBS_COMMAND_STATUS);
		msgOut.m_param[0] = status;
		if(!siftp_send(a_socket, &msgOut) || !status)
			return false;
	
	
	// determine buffer size
		bufLen=0;
		
		while((p_dirInfo = readdir(p_dirFd)))
			bufLen += strlen(p_dirInfo->d_name) + 1;
		
		#ifndef NODEBUG
			printf("cmd_ls(): buffer size = %d\n", bufLen);
		#endif
		
		rewinddir(p_dirFd);
		
	// allocate buffer
	
		if((buf = malloc(bufLen * sizeof(char))) == NULL)
		{
			closedir(p_dirFd);
			fprintf(stderr, "cmd_ls(): malloc() failed.\n");
			return false;
		}
	
	// read contents into buffer
	
		bufIndex=0;
		
		while((p_dirInfo = readdir(p_dirFd)))
		{
			strcpy(&buf[bufIndex], p_dirInfo->d_name);
			
			#ifndef NODEBUG
				printf("cmd_ls(): buffer[%d]='%s'\n", bufIndex, &buf[bufIndex]);
			#endif
			
			bufIndex += strlen(p_dirInfo->d_name);
			buf[bufIndex++] = '\n'; // overwrite null term
			
			#ifndef NODEBUG
				printf("cmd_ls(): buffer {index=%d,val='%s'}\n", bufIndex, buf);
			#endif
		}
		closedir(p_dirFd);
		
	// send data to client
		status = siftp_sendData(a_socket, buf, bufLen);
		
	// clean up
		free(buf);

	return status;
}
