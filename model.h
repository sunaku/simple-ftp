/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP cilent/server application model.
**/

#ifndef MODEL_H
#define MODEL_H

#include "siftp.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>
#include <sys/stat.h>

	/* debugging */
	
	/* constants */
	
		#define MODEL_COMMAND_SIZE	4	/* max length of cmd */
	
	/* typedefs */
	
	/* structs */
		
	/* constructors */
		
	/* utilities */
	
		/**
		 * Establishes a SimpleFTP session.
		 */
		Boolean session_create(const int a_socket);
		
		/**
		 * Destroys an established SimpleFTP session.
		 */
		Boolean session_destroy(const int a_socket)
		{
			#ifndef NODEBUG
				printf("session_destroy(): closing session.");
			#endif
			
			// variables
				Message msgOut;
				
			// init vars
				Message_clear(&msgOut);
				
			// send notice
				Message_setType(&msgOut, SIFTP_VERBS_SESSION_END);
				return siftp_send(a_socket, &msgOut);
		};
		
		/**
		 * Performs interaction/dialouge.
		 */
		void service_loop(const int a_socket);
	
		/**
		 * Handles commands given in interaction/dialouge.
		 */
		Boolean service_command(const int a_socket, const String a_cmdStr);
		
		/**
		 * Performs a remote command and returns its status.
		 */
		inline Boolean remote_command(const int a_socket, Message *ap_query, Message *ap_response)
		{
			return (siftp_query(a_socket, ap_query, ap_response) &&  Message_hasType(ap_response, SIFTP_VERBS_COMMAND_STATUS) && Message_getValue(ap_response)[0]);
		};
		
		
		/**
		 * Changes the path of the current working dir.
		 * @param	a_pwd		Storage for new current working dir value.
		 */
		Boolean service_command_chdir(const String a_cmdStr, const String a_cmdArg, String a_pwd)
		{
			// check args
				if(a_cmdArg == NULL)
					return false;
			
			// variables
				int tempLen;
				char tempPath[PATH_MAX+1];
				struct stat fileStats;
				
			// init variables
				memset(&tempPath, 0, sizeof(tempPath));
			
			
			// assemble absolute path from arg
				if(a_cmdArg[0] == '/')
					strcpy(tempPath, a_cmdArg);
				
				else
				{
					strcpy(tempPath, a_pwd);
					
					// append arg to current path
						if((tempLen = strlen(a_pwd)) + strlen(a_cmdArg) < sizeof(tempPath))
						{
							tempPath[tempLen++] = '/'; // relative path
							strcpy(&tempPath[tempLen], a_cmdArg);
						}
				}
				
			// change path
				if(stat(tempPath, &fileStats) >= 0 && S_ISDIR(fileStats.st_mode) && (fileStats.st_mode & S_IRUSR))
				{
					realpath(tempPath, a_pwd);
					
					#ifndef NODEBUG
						debug("cmd_ls(): tempPath='%s', a_pwd='%s'\n", tempPath, a_pwd);
					#endif
				}
				else
				{
					perror(a_cmdStr);
					return false;
				}

			return true;
		};
		
		
#endif

