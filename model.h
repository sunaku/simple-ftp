/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP cilent/server application model.
**/

#ifndef MODEL_H
#define MODEL_H

#include "siftp.h"

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

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
		 * Performs a simple two-way query & response dialouge.
		 */
		inline Boolean service_query(const int a_socket, const Message *ap_query, Message *ap_response)
		{
			return siftp_send(a_socket, ap_query) && siftp_recv(a_socket, ap_response);
		};
		
		/**
		 * Returns a string containing the absolute path which joins the basePath to the extension.
		 * @param a_result	Storage for resulting path; must have minimum size PATH_MAX
		 */
		Boolean service_getAbsolutePath(const String a_basePath, const String a_extension, String a_result)
		{
			char tempPath[PATH_MAX+1];
			int tempLen;
			
			// init
				memset(&tempPath, 0, sizeof(tempPath));
				
			// assemble absolute path from arg
				if(a_extension[0] == '/')
					strcpy(tempPath, a_extension);
				
				else
				{
					strcpy(tempPath, a_basePath);
					
					// append extension
						if((tempLen = strlen(a_basePath)) + strlen(a_extension) < sizeof(tempPath))
						{
							tempPath[tempLen++] = '/'; // relative path
							strcpy(&tempPath[tempLen], a_extension);
						}
				}
				
			// conv to absolute
				
				realpath(tempPath, a_result);
				
				#ifndef NODEBUG
					printf("service_getAbsolutePath(): before='%s', after='%s'\n", tempPath, a_result);
				#endif
			
			return true;
		};
		
		/**
		 * Handles commands given in interaction/dialouge.
		 */
		Boolean service_handleCmd(const int a_socket, const String a_cmdStr);
		
		/**
		 * Sends a command status acknowlegdement for a command.
		 */
		Boolean service_handleCmd_sendStatus(const int a_socket, const Boolean a_wasSuccess)
		{
			Message msg;
			
			// init variables
				Message_clear(&msg);
				Message_setType(&msg, SIFTP_VERBS_COMMAND_STATUS);
				Message_setValue(&msg, a_wasSuccess ? "true" : "false");
				
			return siftp_send(a_socket, &msg);
		};

		/**
		 * Returns a pointer to the next word in the string.
		 * Assumes words are separated by spaces.
		 */
		String service_handleCmd_getNextArg(const String a_str)
		{
			String arg;
			
			if((arg = strchr(a_str, ' ')) != NULL)
			{
				while(*arg == ' ') // skip whitespace
					*arg++;
			}
			
			return arg;
		};

		
		/**
		 * Returns the value of the command staus acknowlegdement.
		 */
		inline Boolean service_handleCmd_recvStatus(const int a_socket)
		{
			Message msg;
			
			// init vars
				Message_clear(&msg);
			
			return (siftp_recv(a_socket, &msg) &&  Message_hasType(&msg, SIFTP_VERBS_COMMAND_STATUS) && Message_hasValue(&msg, "true"));
		};

		/**
		 * Performs a remote command and returns its status.
		 */
		inline Boolean remote_exec(const int a_socket, Message *ap_query)
		{
			return (siftp_send(a_socket, ap_query) &&  service_handleCmd_recvStatus(a_socket));
		};
		
		/**
		 * Changes the path of the current working dir.
		 * @param	a_pwd		Storage for new current working dir value.
		 */
		Boolean service_handleCmd_chdir(const String a_cmdStr, const String a_cmdArg, String a_pwd)
		{
			// check args
				if(a_cmdArg == NULL)
					return false;
			
			// variables
				struct stat fileStats;
				char path[PATH_MAX+1];
				
			// init variables
				memset(&path, 0, sizeof(path));
			
				if(service_getAbsolutePath(a_pwd, a_cmdArg, path))
				{
					if(stat(path, &fileStats) >= 0 && S_ISDIR(fileStats.st_mode) && (fileStats.st_mode & S_IRUSR))
					{
						strcpy(a_pwd, path);
					}
					else
					{
						perror(a_cmdStr);
						return false;
					}
				}

			return true;
		};
		
		
		/**
		 * Returns contents of a file.
		 * @param	ap_length	Storage for length (including null terminator) of data.
		 * Note: returns a malloc()ed object.
		 */
		String service_handleCmd_readFile(const String a_path, int *ap_length)
		{
			// variables
				String buf = NULL;
				FILE *p_fileFd;
			
			if((p_fileFd = fopen(a_path, "rb")) == NULL)
			{
				perror("service_handleCmd_readFile()");
			}
			else
			{
				// determine file size
					fseek(p_fileFd, 0, SEEK_END);
					*ap_length = ftell(p_fileFd) + 1; // +1 for null term
					rewind(p_fileFd);
					
				// allocate buffer
					if((buf = calloc(*ap_length, sizeof(char))) == NULL) // +1 for null term
					{
						fclose(p_fileFd);
						
						fprintf(stderr, "service_handleCmd_readFile(): calloc() for buffer failed.\n");
						return false;
					}
					
				// read contents into buffer
				fread(buf, sizeof(char), *ap_length-1, p_fileFd); // -1 to preserve null term
			}
			
			return buf;
		};
		
		/**
		 * Returns contents of a dir, one entry per line.
		 * @param	ap_length	Storage for length (including null terminator) of data.
		 * Note: returns a malloc()ed object.
		 */
		String service_handleCmd_readDir(const String a_path, int *ap_length)
		{
			// variables
				String buf = NULL;
				int i;
				
				DIR *p_dirFd;
				struct dirent *p_dirInfo;
		
			if((p_dirFd = opendir(a_path)) == NULL)
			{
				perror("service_handleCmd_readDir()");
			}
			else
			{
				// determine buffer size
					*ap_length=0;
					
					while((p_dirInfo = readdir(p_dirFd)))
						*ap_length += strlen(p_dirInfo->d_name) + 1;
					
					#ifndef NODEBUG
						printf("service_handleCmd_readDir(): buffer size = %d\n", *ap_length);
					#endif
					
					rewinddir(p_dirFd);
					
				// allocate buffer
					if((buf = calloc(*ap_length, sizeof(char))) == NULL)
					{
						closedir(p_dirFd);
						
						fprintf(stderr, "service_handleCmd_readDir(): calloc() for buffer failed.\n");
						return false;
					}
				
				// read contents into buffer
					i=0;
					
					while((p_dirInfo = readdir(p_dirFd)))
					{
						strcpy(&buf[i], p_dirInfo->d_name);
						
						#ifndef NODEBUG
							printf("service_handleCmd_readDir()(): buffer[%d]='%s'\n", i, &buf[i]);
						#endif
						
						i += strlen(p_dirInfo->d_name);
						buf[i++] = '\n'; // overwrite null term
						
						#ifndef NODEBUG
							printf("service_handleCmd_readDir()(): buffer {index=%d,val='%s'}\n", i, buf);
						#endif
					}
					
					closedir(p_dirFd);
			}
			
			return buf;
		};
		
		/**
		 * Writes data to the given path.
		 * @param	a_path	Path to which data will be written.
		 * @param	a_data	Data to be written.
		 * @param	a_length	Number of bytes of data to write.
		 */
		Boolean service_handleCmd_writeFile(const String a_path, const String a_data, const int a_length)
		{
			int fileFd;
			Boolean result = false;
			
			if((fileFd = open(a_path, O_WRONLY)) >= 0)
			{
				if(write(fileFd, a_data, a_length) != -1)
					result = true;
				else
					perror("service_handleCmd_writeFile()");
				
				close(fileFd);
			}
			else
				perror("service_handleCmd_writeFile()");
			
			#ifndef NODEBUG
				printf("writeFile(): wrote file='%s', length=%d, &data=%p, data='%s'.\n", a_path, a_length, a_data, a_data);
			#endif
			
			return result;
		};
		
		
		/**
		 * Changes the path of the current working dir.
		 * @param	a_pwd		Storage for new current working dir value.
		 */
		Boolean service_handleCmd_sendFile(const int a_socket, const String a_filePath)
		{
			// variables
				struct stat fileStats;
				String buf;
				int bufLen;
				Boolean tempStatus;
				
			// init variables
			
				if(stat(a_filePath, &fileStats) >= 0 && !S_ISDIR(fileStats.st_mode) && (fileStats.st_mode & S_IRUSR))
				{
					if((buf = service_handleCmd_readFile(a_filePath, &bufLen)) != NULL)
					{
						if(service_handleCmd_sendStatus(a_socket, true))
						{
							tempStatus = siftp_sendData(a_socket, buf, bufLen);
							
							#ifndef NODEBUG 
								printf("get(): sent file '%s'.\n", a_filePath);
							#endif
						}
						
						free(buf);
						return tempStatus;
					}
				}
				else
				{
					perror("service_handleCmd_sendFile()");
				}

			return false;
		};
		
#endif

