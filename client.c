/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP client.
**/

#include "client.h"
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
		if(!link_create(&socket, ap_argv[1], ap_argv[2]))
		{
			fprintf(stderr, "%s: unable to connect to %s:%s\n", ap_argv[0], ap_argv[1], ap_argv[2]);
			return 2;
		}
		
	// establish session
		if(!session_create(socket))
		{
			close(socket);
			
			fprintf(stderr, "%s: unable to establish session\n", ap_argv[0]);
			return 3;
		}
	
	// handle user commands
		input_loop(socket);
	
	// destroy session
		session_destroy(socket);
		
	// destroy link
		close(socket);
		
	return 0;
}

Boolean link_create(int *ap_socket, const String a_serverName, const String a_serverPort)
{
	// variables
		struct sockaddr_in serverAddr;
		struct hostent *p_serverInfo;
		
	// assemble server address struct
		
		// determine dotted quad str from DNS
			if((p_serverInfo = gethostbyname(a_serverName)) == NULL)
			{
				herror("link_create()");
				return false;
			}
			
			#ifndef NODEBUG
				printf("link_create(): serverName='%s', serverAddr='%s'\n", a_serverName, inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
			#endif
			
			serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(*((struct in_addr *)p_serverInfo->h_addr)));
			
		serverAddr.sin_family = AF_INET;
		serverAddr.sin_port = htons(strtol(a_serverPort, (char**)NULL, 10));
		
	// create socket
		if((*ap_socket = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
		{
			perror("link_create()");
			return false;
		}
	
	// establish connection
		if((connect(*ap_socket, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0)
		{
			perror("link_create()");
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
	
		// c: greeting
		// s: identify
			strcpy(msgOut.m_verb, SIFTP_VERBS_SESSION_BEGIN);
			
			if(!siftp_query(a_socket, &msgOut, &msgIn) || strncmp(msgIn.m_verb, SIFTP_VERBS_IDENTIFY, SIFTP_VERB_SIZE))
			{
				fprintf(stderr, "session_create(): connection request rejected.\n");
				return false;
			}
			
		// C: username
		// S: accept|deny
			strcpy(msgOut.m_verb, SIFTP_VERBS_USERNAME);
			
			if(!siftp_query(a_socket, &msgOut, &msgIn) || strncmp(msgIn.m_verb, SIFTP_VERBS_ACCEPTED, SIFTP_VERB_SIZE))
			{
				fprintf(stderr, "session_create(): username rejected.\n");
				return false;
			}
		
		// C: password
		// S: accept|deny
		
			strcpy(msgOut.m_verb, SIFTP_VERBS_PASSWORD);
			
			// get user input
				printf("\npassword: ");
				scanf("%s", msgOut.m_param);
		
			if(!siftp_query(a_socket, &msgOut, &msgIn) || strncmp(msgIn.m_verb, SIFTP_VERBS_ACCEPTED, SIFTP_VERB_SIZE))
			{
				fprintf(stderr, "session_create(): password rejected.\n");
				return false;
			}
		
		// connection now established
		
	return true;
}

inline Boolean session_destroy(const int a_socket)
{
	// variables
		Message msgOut;
		
	// init vars
		Message_reset(&msgOut);
		
	// send notice
		strcpy(msgOut.m_verb, SIFTP_VERBS_SESSION_END);
		return siftp_send(a_socket, &msgOut);
}

void input_loop(const int a_socket)
{
	char buf[CLIENT_INPUT_SIZE], cmd[CLIENT_COMMAND_SIZE];
	Boolean isLocal, status;
	
	while(1)
	{
		// read input
			printf("\nSimpleFTP> ");
			scanf("%s", buf);
			
		// parse input
			isLocal = (buf[0] == 'l') && (buf[1] != 's');
			strncpy(cmd, &buf[(isLocal ? 1 : 0)], CLIENT_COMMAND_SIZE);
			
		// handle commands
			if(strstr(cmd, "ls"))
				status = cmd_ls(a_socket, isLocal, buf);

			else if(strstr(cmd, "pwd"))
				status = cmd_pwd(a_socket, isLocal, buf);

/*			else if(strstr(cmd, "cd"))
				status = cmd_cd(a_socket, isLocal, buf);

			else if(strstr(cmd, "get"))
				status = cmd_get(a_socket, isLocal, buf);

			else if(strstr(cmd, "put"))
				status = cmd_put(a_socket, isLocal, buf);
*/
			else if(strstr(cmd, "help"))
			{
				status = true;
				
				printf("ls\t-displays contents of remote current working directory.\n");
				printf("lls\t-displays contents of local current working directory.\n");
				printf("pwd\t-displays path of remote current working directory.\n");
				printf("lpwd\t-displays path of local current working directory.\n");
				printf("cd <path>\t-changes the remote current working directory to the specified <path>.\n");
				printf("lcd <path>\t-changes the local current working directory to the specified <path>.\n");
				printf("get <file>\t-downloads the remote <file> to local current working directory.\n");
				printf("put <file>\t-uploads the local <file> to remote current working directory.\n");
				printf("help\t-displays this message.\n");
				printf("exit\t-leaves this program.\n");
			}

			else if(strstr(cmd, "exit"))
				break;

			else
			{
				status = false;
				printf("Invalid command. Try 'help'.");
			}
			
		// display command status
			printf("\n(status) %s.", (status ? "Success" : "Failure"));
	}
}

Boolean cmd_ls(const int a_socket, const Boolean a_isLocal, const String a_cmdStr)
{
	// variables
		Message msgOut, msgIn;
		DIR *p_dirFd;
		struct dirent *p_dirInfo;
		String buf = NULL;
		int bufLen;
		
	// init vars
		Message_reset(&msgOut);
		Message_reset(&msgIn);
		
	// check domain
		if(a_isLocal)
		{
			// open
				if((p_dirFd = opendir(g_pwd)) == NULL)
				{
					perror("cmd_ls()");
					return false;
				}

			// read
				while((p_dirInfo = readdir(p_dirFd)))
					printf("%s\n", p_dirInfo->d_name);
				
			// close
				closedir(p_dirFd);
		}
		else
		{
			// build query
				strcpy(msgOut.m_verb, SIFTP_VERBS_COMMAND);
				strcpy(msgOut.m_param, "ls");
			
			// query server
				if(!siftp_query(a_socket, &msgOut, &msgIn) || strncmp(msgIn.m_verb, SIFTP_VERBS_COMMAND_STATUS, SIFTP_VERB_SIZE) || msgIn.m_param[0] == false)
					return false;
			
			// get result
				buf = siftp_recvData(a_socket, &bufLen);
				
			// print result
				printf("%s", buf);
				
			// clean up
				free(buf);
		}
		
	return true;
}

Boolean cmd_pwd(const int a_socket, const Boolean a_isLocal, const String a_cmdStr)
{
	// variables
		Message msgOut, msgIn;
		
	// init vars
		Message_reset(&msgOut);
		Message_reset(&msgIn);
		
	// check domain
		if(a_isLocal)
		{
			printf("%s", g_pwd);
		}
		else
		{
			// build query
				strcpy(msgOut.m_verb, SIFTP_VERBS_COMMAND);
				strcpy(msgOut.m_param, "pwd");
			
			// query server
				if(!siftp_query(a_socket, &msgOut, &msgIn) || strncmp(msgIn.m_verb, SIFTP_VERBS_COMMAND_STATUS, SIFTP_VERB_SIZE) || msgIn.m_param[0] == false)
					return false;
				
			// get result
				if(!siftp_recv(a_socket, &msgIn) || strncmp(msgIn.m_verb, SIFTP_VERBS_DATA_GRAM, SIFTP_VERB_SIZE))
					return false;
				
			// print result
				printf("%s", msgIn.m_param);
		}
		
	return true;
}


