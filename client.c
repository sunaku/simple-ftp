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


int main(int a_argc, char **ap_argv)
{
	// variables
		int sockfd;
		
	// check args
		if(a_argc < 3)
		{
			printf("Usage: %s server_name port_number\n", ap_argv[0]);
			return 1;
		}
		
	// establish link
		if(!link_create(&sockfd, ap_argv[1], ap_argv[2]))
		{
			fprintf(stderr, "%s: unable to connect to %s:%s\n", ap_argv[0], ap_argv[1], ap_argv[2]);
			return 2;
		}
		
	// establish session
		if(!session_create(sockfd))
		{
			link_destroy(&sockfd);
			
			fprintf(stderr, "%s: unable to establish session\n", ap_argv[0]);
			return 3;
		}
	
	// handle user commands
		input_loop(sockfd);
	
	// destroy session
		session_destroy(sockfd);
		
	// destroy link
		link_destroy(&sockfd);
		
	return 0;
}

Boolean link_create(int *ap_sockfd, const String a_serverName, const String a_serverPort)
{
	// variables
		struct sockaddr_in serverAddr;
		struct hostent *p_serverInfo;
		
	// assemble server address struct
		
		// determine dotted quad str from DNS
			if((p_serverInfo = gethostbyname(a_serverName)) == NULL)
			{
				herror("gethostbyname");
				fprintf(stderr, "link_create(): Cannot resolve IP address from hostname: \"%s\".\n", a_serverName);
				return false;
			}
			
		// convert dotted quad str to numerical addr
			if(inet_aton(p_serverInfo->h_addr, &serverAddr.sin_addr) == 0)
			{
				fprintf(stderr, "link_create(): inet_aton() failed.\n");
				return false;
			}
		
		serverAddr.sin_family = htonl(AF_INET);
		serverAddr.sin_port = htons(atoi(a_serverPort));
		memset(&(serverAddr.sin_zero), '\0', 8);  // zero the rest of the struct
		
	// create socket
		if((*ap_sockfd = socket(serverAddr.sin_family, SOCK_STREAM, 0)) < 0)
		{
			fprintf(stderr, "link_create(): Socket creation failed.\n");
			return false;
		}
	
	// establish connection
		if((connect(*ap_sockfd, (struct sockaddr *)&serverAddr, sizeof(struct sockaddr))) < 0)
		{
			fprintf(stderr, "link_create(): Connection refused.\n");
			close(*ap_sockfd);
			return false;
		}
	
	return true;
}

Boolean session_create(const int a_sockfd)
{
	// variables
		Message msgOut, msgIn;
		
	// session challenge dialogue
	
		// c: greeting
		// s: identify
			strcpy(msgOut.m_verb, SIFTP_VERBS_SESSION_BEGIN);
			
			if(!siftp_query(a_sockfd, &msgOut, &msgIn) || msgIn.m_verb != SIFTP_VERBS_IDENTIFY)
			{
				fprintf(stderr, "session_create(): connection request rejected.\n");
				return false;
			}
			
		// C: username
		// S: accept|deny
			strcpy(msgOut.m_verb, SIFTP_VERBS_USERNAME);
			
			if(!siftp_query(a_sockfd, &msgOut, &msgIn) || msgIn.m_verb != SIFTP_VERBS_ACCEPTED)
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
		
			if(!siftp_query(a_sockfd, &msgOut, &msgIn) || msgIn.m_verb != SIFTP_VERBS_ACCEPTED)
			{
				fprintf(stderr, "session_create(): password rejected.\n");
				return false;
			}
		
		// connection now established
		
	return true;
}

void input_loop(const int a_sockfd)
{
	char buf[CLIENT_INPUT_SIZE], cmd[CLIENT_COMMAND_SIZE];
	Boolean isLocal, status;
	
	while(1)
	{
		// read input
			printf("\nSimpleFTP> ");
			scanf("%s", buf);
			
		// parse input
			isLocal = (buf[0] == 'l');
			strncpy(cmd, &buf[(isLocal ? 1 : 0)], CLIENT_COMMAND_SIZE);
			
		// handle commands
			if(strstr(cmd, "ls"))
				status = cmd_ls(a_sockfd, isLocal, buf);
			else if(strstr(cmd, "pwd"))
				status = cmd_pwd(a_sockfd, isLocal, buf);
			else if(strstr(cmd, "cd"))
				status = cmd_cd(a_sockfd, isLocal, buf);
			else if(strstr(cmd, "get"))
				status = cmd_get(a_sockfd, isLocal, buf);
			else if(strstr(cmd, "put"))
				status = cmd_put(a_sockfd, isLocal, buf);
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
