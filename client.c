/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP client interface.
**/

#include "service.h"
#include "client.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// globals
	char g_pwd[PATH_MAX+1]; // holds the path of current working directory

// funcs
int main(int a_argc, char **ap_argv)
{
	// variables
		int socket;
		
	// check args
		if(a_argc < 3)
		{
			printf("Usage: %s server_name port_number\n", ap_argv[0]);
			return 1;
		}
		
	// init vars
		realpath(".", g_pwd);
		
	// establish link
		if(!service_create(&socket, ap_argv[1], strtol(ap_argv[2], (char**)NULL, 10)))
		{
			fprintf(stderr, "%s: Connection to %s:%s failed.\n", ap_argv[0], ap_argv[1], ap_argv[2]);
			return 2;
		}
		
	// establish session
		if(!session_create(socket))
		{
			close(socket);
			
			fprintf(stderr, "%s: Session failed.\n", ap_argv[0]);
			return 3;
		}
		
		printf("\nSession established successfully.");
	
	// handle user commands
		service_loop(socket);
	
	// destroy session
		if(session_destroy(socket))
			printf("Session terminated successfully.\n");
		
	// destroy link
		close(socket);
		
	return 0;
}

Boolean service_create(int *ap_socket, const String a_serverName, const int a_serverPort)
{
	// variables
		struct sockaddr_in serverAddr;
		struct hostent *p_serverInfo;
		
	// create server address
		
		memset(&serverAddr, 0, sizeof(serverAddr));
		
		// determine dotted quad str from DNS query
			if((p_serverInfo = gethostbyname(a_serverName)) == NULL)
			{
				herror("service_create()");
				return false;
			}
			
			#ifndef NODEBUG
				printf("service_create(): serverName='%s', serverAddr='%s'\n", a_serverName, inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
			#endif
			
			serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
			
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(a_serverPort);
		
	// create socket
		if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
		{
			perror("service_create(): create socket");
			return false;
		}
	
	// connect on socket
		if((connect(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0)
		{
			perror("service_create(): connect socket");
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
	
		// cilent: greeting
		// server: identify
			Message_setType(&msgOut, SIFTP_VERBS_SESSION_BEGIN);
			
			if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_IDENTIFY))
			{
				fprintf(stderr, "session_create(): connection request rejected.\n");
				return false;
			}
			
		// cilent: username
		// server: accept|deny
			Message_setType(&msgOut, SIFTP_VERBS_USERNAME);
			
			// get user input
				printf("\nusername: ");
				
				// XXX prohibited by this project
				//scanf("%s", msgOut.m_param);
			
			if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED))
			{
				fprintf(stderr, "session_create(): username rejected.\n");
				return false;
			}
		
		// cilent: password
		// server: accept|deny
		
			Message_setType(&msgOut, SIFTP_VERBS_PASSWORD);
			
			// get user input
				printf("\npassword: ");
				scanf("%s", msgOut.m_param);
		
			if(!service_query(a_socket, &msgOut, &msgIn) || !Message_hasType(&msgIn, SIFTP_VERBS_ACCEPTED))
			{
				fprintf(stderr, "session_create(): password rejected.\n");
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
		char buf[SIFTP_MESSAGE_SIZE+1];
		Boolean status, isLooping = true;
		String bufCR;
		
		String *p_argv;
		int argc;
		
	while(isLooping)
	{
		// read input
			printf("\nSimpleFTP> ");
			memset(&buf, 0, sizeof(buf)); // clear buffer
			fgets(buf, sizeof(buf), stdin);
			
			// remove newline
			if((bufCR = strrchr(buf, '\n')) != NULL)
				*bufCR = '\0';
			
			#ifndef NODEBUG
				printf("service_loop(): got command '%s'\n", buf);
			#endif
			
		// handle commands
			status = true;
		
			if(strcmp(buf, "exit") == 0)
			{
				isLooping = false;
			}
			else if(strcmp(buf, "help") == 0)
			{
				printf("\nls\n  displays contents of remote current working directory.\n");
				printf("\nlls\n  displays contents of local current working directory.\n");
				printf("\npwd\n  displays path of remote current working directory.\n");
				printf("\nlpwd\n  displays path of local current working directory.\n");
				printf("\ncd <path>\n  changes the remote current working directory to the specified <path>.\n");
				printf("\nlcd <path>\n  changes the local current working directory to the specified <path>.\n");
				printf("\nget <src> [dest]\n  downloads the remote <src> to local current working directory by the name of [dest] if specified.\n");
				printf("\nput <src> [dest]\n  uploads the local <src> file to remote current working directory by the name of [dest] if specified.\n");
				printf("\nhelp\n  displays this message.\n");
				printf("\nexit\n  terminates this program.\n");
			}
			else if(strlen(buf) > 0)
			{
				// parse command arguments
				if((p_argv = service_parseArgs(buf, &argc)) == NULL || argc <= 0)
					status = false;
				else
					status = service_handleCmd(a_socket, p_argv, argc);
				
				// clean up
					service_freeArgs(p_argv, argc);
					p_argv = NULL;
			}
			else
				continue;
			
		// display command status
			printf("\n(status) %s.", (status ? "Success" : "Failure"));
	}
}

Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc)
{
	// variables
		Message msgOut;
		
		String dataBuf;
		int dataBufLen;
		
		Boolean tempStatus = false;
		
	// init variables
		Message_clear(&msgOut);
	
	// handle commands
	if(strcmp(ap_argv[0], "lls") == 0)
	{
		if((dataBuf = service_readDir(g_pwd, &dataBufLen)) != NULL)
		{
			printf("%s", dataBuf);
			free(dataBuf);
			
			return true;
		}
	}
	
	else if(strcmp(ap_argv[0], "lpwd") == 0)
	{
		printf("%s", g_pwd);
		return true;
	}
	
	else if(strcmp(ap_argv[0], "lcd") == 0 && a_argc > 1)
	{
		return service_handleCmd_chdir(g_pwd, ap_argv[1]);
	}
	
	else if(strcmp(ap_argv[0], "ls") == 0 || strcmp(ap_argv[0], "pwd") == 0)
	{
		// build command
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
			Message_setValue(&msgOut, ap_argv[0]);
			
		// perform command
			if(remote_exec(a_socket, &msgOut))
			{
				if((dataBuf = siftp_recvData(a_socket, &dataBufLen)) != NULL)
				{
					printf("%s", dataBuf);
					free(dataBuf);
					
					return true;
				}
			}
	}
	
	else if(strcmp(ap_argv[0], "cd") == 0 && a_argc > 1)
	{
		// build command
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
			Message_setValue(&msgOut, ap_argv[0]);
			strcat(Message_getValue(&msgOut), " ");
			strcat(Message_getValue(&msgOut), ap_argv[1]);
			
		// perform command
			return remote_exec(a_socket, &msgOut);
	}
	
	else if(strcmp(ap_argv[0], "get") == 0 && a_argc > 1)
	{
		char dstPath[PATH_MAX+1];
		String src, dst;
		
		// init vars
			src = ap_argv[1];
			dst = (a_argc > 2) ? ap_argv[2] : src;
		
		// build command with param='get remote-path'
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
			Message_setValue(&msgOut, ap_argv[0]);
			strcat(Message_getValue(&msgOut), " ");
			strcat(Message_getValue(&msgOut), src);
			
		// determine destination file path
			if(service_getAbsolutePath(g_pwd, dst, dstPath))
			{
				// check write perms & file type
				if(service_permTest(dstPath, SERVICE_PERMS_WRITE_TEST) && service_statTest(dstPath, S_IFMT, S_IFREG))
				{
					// perform command
					if(remote_exec(a_socket, &msgOut))
					{
						// receive destination file
						if((dataBuf = siftp_recvData(a_socket, &dataBufLen)) != NULL)
						{
							// write file
							if((tempStatus = service_writeFile(dstPath, dataBuf, dataBufLen)))
							{
								printf("%d bytes transferred.", dataBufLen);
							}
							
							free(dataBuf);
							
							#ifndef NODEBUG
								printf("get(): file writing %s.\n", tempStatus ? "OK" : "FAILED");
							#endif
						}
						#ifndef NODEBUG
						else
							printf("get(): getting of remote file failed.\n");
						#endif
					}
					#ifndef NODEBUG
					else
						printf("get(): server gave negative ACK.\n");
					#endif
				}
				#ifndef NODEBUG
				else
					printf("get(): don't have write permissions.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("get(): absolute path determining failed.\n");
			#endif
			
		return tempStatus;
	}
	
	else if(strcmp(ap_argv[0], "put") == 0 && a_argc > 1)
	{
		char srcPath[PATH_MAX+1];
		String src, dst;
		
		// init vars
			src = ap_argv[1];
			dst = (a_argc > 2) ? ap_argv[2] : src;
			
		// build command with param='put remote-path'
			Message_setType(&msgOut, SIFTP_VERBS_COMMAND);
			Message_setValue(&msgOut, ap_argv[0]);
			strcat(Message_getValue(&msgOut), " ");
			strcat(Message_getValue(&msgOut), dst);
			
		// determine source path
			if(service_getAbsolutePath(g_pwd, src, srcPath))
			{
				// check read perms & file type
				if(service_permTest(srcPath, SERVICE_PERMS_READ_TEST) && service_statTest(srcPath, S_IFMT, S_IFREG))
				{
					// try to read source file
					if((dataBuf = service_readFile(srcPath, &dataBufLen)) != NULL)
					{
						// client: i'm sending a file
						if(remote_exec(a_socket, &msgOut))
						{
							// server: OK to send file
							
							// client: here is the file
							if((tempStatus = (siftp_sendData(a_socket, dataBuf, dataBufLen) && service_recvStatus(a_socket))))
							{
								// server: success
							
								printf("%d bytes transferred.", dataBufLen);
							}
							
							#ifndef NODEBUG
								printf("put(): file sent %s.\n", tempStatus ? "OK" : "FAILED");
							#endif
						}
						#ifndef NODEBUG
						else
							printf("put(): server gave negative ACK.\n");
						#endif
						
						free(dataBuf);
					}
					#ifndef NODEBUG
					else
						printf("put(): file reading failed.\n");
					#endif
				}
				#ifndef NODEBUG
				else
					printf("put(): don't have read permissions.\n");
				#endif
			}
			#ifndef NODEBUG
			else
				printf("put(): absolute path determining failed.\n");
			#endif
			
		return tempStatus;
	}
	
	return false;
}
