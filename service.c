/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP cilent/server common services implementation.
**/

#include "service.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <dirent.h>

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
}

Boolean service_query(const int a_socket, const Message *ap_query, Message *ap_response)
{
	return siftp_send(a_socket, ap_query) && siftp_recv(a_socket, ap_response);
}

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
}

Boolean service_sendStatus(const int a_socket, const Boolean a_wasSuccess)
{
	Message msg;
	
	// init variables
		Message_clear(&msg);
		Message_setType(&msg, SIFTP_VERBS_COMMAND_STATUS);
		Message_setValue(&msg, a_wasSuccess ? "true" : "false");
		
	return siftp_send(a_socket, &msg);
}

String* service_parseArgs(const String a_cmdStr, int *ap_argc)
{
	String buf, *p_args, arg;
	int i;
	
	// init vars
		if((buf = calloc(strlen(a_cmdStr)+1, sizeof(char))) == NULL)
			return NULL;
		
		strcpy(buf, a_cmdStr);
	
	// parse words
		for(p_args = NULL, i=0, arg = strtok(buf, " \t"); arg != NULL; i++, arg = strtok(NULL, " \t"))
		{
			// expand array
				if((p_args = realloc(p_args, (i+1) * sizeof(String))) == NULL)
				{
					free(buf);
					return NULL;
				}
			
			// expand item
				if((p_args[i] = calloc(strlen(arg)+1, sizeof(char))) == NULL)
				{
					service_freeArgs(p_args, i);
					free(buf);
					
					return NULL;
				}
				
			// store item
				strcpy(p_args[i], arg);
		}
		
		*ap_argc = i;
		
		#ifndef NODEBUG
			for(i = 0; i<*ap_argc; i++)
				printf("parseArgs(): argv[%d]='%s', addr=%p\n", i, p_args[i], p_args[i]);
		#endif
		
	// clean up
		free(buf);
		
	return p_args;
}

void service_freeArgs(String *ap_argv, const int a_argc)
{
	// check args
	if(ap_argv == NULL || a_argc <= 0)
		return;
	
	int i;
	
	// free array items
	for(i = 0; i<a_argc; i++)
	{
		if(ap_argv[i] != NULL)
			free(ap_argv[i]);
	}
	
	free(ap_argv);
}

Boolean service_recvStatus(const int a_socket)
{
	Message msg;
	
	// init vars
		Message_clear(&msg);
	
	return (siftp_recv(a_socket, &msg) &&  Message_hasType(&msg, SIFTP_VERBS_COMMAND_STATUS) && Message_hasValue(&msg, "true"));
}

Boolean remote_exec(const int a_socket, Message *ap_query)
{
	return (siftp_send(a_socket, ap_query) &&  service_recvStatus(a_socket));
}

Boolean service_handleCmd_chdir(String a_currPath, const String a_newPath)
{
	// check args
		if(a_newPath == NULL)
			return false;
	
	// variables
		char path[PATH_MAX+1];
		
	// init variables
		memset(&path, 0, sizeof(path));
	
	// validate new path
		if(service_getAbsolutePath(a_currPath, a_newPath, path))
		{
			// check inode type & perms
			if(service_permTest(path, SERVICE_PERMS_READ_TEST) && service_statTest(path, S_IFMT, S_IFDIR))
			{
				strcpy(a_currPath, path);
			}
			else
			{
				perror("cd()");
				return false;
			}
		}

	return true;
}

String service_readFile(const String a_path, int *ap_length)
{
	String buf = NULL;
	FILE *p_fileFd;

	// check perms
	if(service_statTest(a_path, S_IFMT, S_IFREG))
	{
		if((p_fileFd = fopen(a_path, "r")) != NULL)
		{
			// determine file size
				fseek(p_fileFd, 0, SEEK_END);
				*ap_length = ftell(p_fileFd);
				rewind(p_fileFd);
				
			// allocate buffer
				if((buf = calloc(*ap_length, sizeof(char))) == NULL)
				{
					fclose(p_fileFd);
					
					fprintf(stderr, "readFile(): buffer alloc failed.\n");
					return false;
				}
				
			// read contents into buffer
				if(fread(buf, sizeof(char), *ap_length, p_fileFd) != *ap_length)
				{
					perror("readFile()");
					fclose(p_fileFd);
					
					
					free(buf);
					buf = NULL;
				}
				
			fclose(p_fileFd);
		}
		else
			perror("readFile()");
	}
	
	return buf;
}

String service_readDir(const String a_path, int *ap_length)
{
	String buf = NULL;
	int i, j;
	
	DIR *p_dirFd;
	struct dirent *p_dirInfo;

	if((p_dirFd = opendir(a_path)) == NULL)
	{
		perror("service_readDir()");
	}
	else
	{
		// read contents into buffer
		for(i = j = 0; (p_dirInfo = readdir(p_dirFd)); i = j)
		{
			j += strlen(p_dirInfo->d_name);
			
			#ifndef NODEBUG
				printf("service_readDir(): file='%s', i=%d, j=%d\n", p_dirInfo->d_name, i, j);
			#endif
			
			// expand buffer
				if((buf = realloc(buf, (j+1) * sizeof(char))) == NULL) // +1 for newline
				{
					closedir(p_dirFd);
					
					fprintf(stderr, "service_readDir(): buffer expansion failed.\n");
					return false;
				}
			
			strcpy(&buf[i], p_dirInfo->d_name);
			buf[j++] = '\n'; // set newline
		}
		
		closedir(p_dirFd);
		
		buf[j-1] = '\0'; // replace last newline w/ null term
		*ap_length = j; // store length
	}
	
	return buf;
}

Boolean service_writeFile(const String a_path, const String a_data, const int a_length)
{
	FILE* p_fileFd;
	Boolean result = false;
	
	if((p_fileFd = fopen(a_path, "w")))
	{
		if(fwrite(a_data, sizeof(char), a_length, p_fileFd) != -1)
		{
			result = true;
			
			#ifndef NODEBUG
				printf("writeFile(): wrote file='%s', length=%d, &data=%p, data='%s'.\n", a_path, a_length, a_data, a_data);
			#endif
		}
		else
			perror("service_writeFile()");
		
		fclose(p_fileFd);
	}
	else
		perror("service_writeFile()");
	
	return result;
}

Boolean service_permTest(const String a_path, const String a_mode)
{
	FILE *p_fileFd;
	Boolean result = false;
	
	if((p_fileFd = fopen(a_path, a_mode)))
	{
		fclose(p_fileFd);
		result = true;
	}
	
	return result;
}


Boolean service_statTest(const String a_path, const int a_testMode, const int a_resultMode)
{
	struct stat s;
	
	return (stat(a_path, &s) != -1) && ((s.st_mode & a_testMode) == a_resultMode);
}
