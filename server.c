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
		Message_init(&msgOut);
		Message_init(&msgIn);
		
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
		
	return true;
}

inline Boolean session_destroy(const int a_socket)
{
	// variables
		Message msgOut;
		
	// init vars
		Message_init(&msgOut);
		
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
		Message_init(&msg);
	
	
	while(1)
	{
		// await request
			siftp_recv(a_socket, &msg);
			
			if(strcmp(msg.m_verb, SIFTP_VERBS_SESSION_END))
			{
				// parse request
					strncpy(cmd, msg.m_param, SERVER_COMMAND_SIZE);
					
				// handle request
		/*			if(strstr(cmd, "ls"))
						status = cmd_ls(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "pwd"))
						status = cmd_pwd(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "cd"))
						status = cmd_cd(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "get"))
						status = cmd_get(a_socket, &msg.m_param);
					
					else if(strstr(cmd, "put"))
						status = cmd_put(a_socket, &msg.m_param);
					
					else // unknown request
					{
		*/				strcpy(msg.m_verb, SIFTP_VERBS_COMMAND_STATUS);
						msg.m_param[0] = false;
						
						siftp_send(a_socket, &msg);
			//		}
			}
			
			else
				break;
	}
}

/*
Boolean cmd_ls(const int a_socket, String a_commandLine)
{
	// variables
		Message msgOut;
		
		DIR *p_dirFd;
		struct dirent *p_dirInfo;
		
		String buf = NULL;
		unsigned int bufLen=0;
		
	// init vars
		Message_init(&msgOut);
		
	// open
		if((p_dirFd = opendir(g_pwd)) == NULL)
		{
			perror("cmd_ls()");
			return false;
		}

	// read
		while((p_dirInfo = readdir(p_dirFd)))
		{
			// create space in buffer
			if((buf = realloc(buf, d_reclen * sizeof(char)) == NULL)
			{
				fprintf(stderr, "cmd_ls(): realloc() failed.\n");
				return false;
			}
			
			strcpy(&buf[bufLen], p_dirInfo->d_name);
			bufLen += d_reclen;
		}
		
	// close
		closedir(p_dirFd);
		
	// clean up
		free(buf);

	return true;
}
*/
