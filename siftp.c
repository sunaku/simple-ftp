/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP protocol structures implementation.
**/

#include "siftp.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// constants

// prototypes

// constructors

	Message* Message_create(String a_verb, String a_param)
	{
		// variables
			Message *p_msg;
			
		// allocate space
			p_msg = malloc(sizeof(Message));
		
		// initialize members
			if(p_msg != NULL)
			{
				p_msg->m_verb = a_verb;
				p_msg->m_param = a_param;
			}
		
		return p_msg;
	}
	
	void Message_destroy(Message *ap_msg)
	{
		// check args
			if(ap_msg == NULL)
				return;
		
		// destroy obj
			free(ap_msg->m_verb);
			free(ap_msg->m_param);
			free(ap_msg);
	}

// utility functions

		String siftp_escape(const String a_str)
		{
			String buf = NULL;
			int i, j, len;
			
			// check args
				if(a_str == NULL)		// no work to be done
					return NULL;
			
			// allocate space for buffer
				// worst case: payload contains all flags; 
				// thus, buffer_size = 2*payload_size
				len = strlen(a_str) << 1;
				
				if((buf = malloc(++len * sizeof(char))) == NULL) // +1 for null term
				{
					fprintf(stderr, "siftp_escape: malloc() failed.\n");
					return false;
				}
				
			// copy payload into buffer whilst escaping it
				for(i=0, j=0; i < len; i++, j++)
				{
					if((buf[j] = a_str[i]) == SIFTP_SERIALIZED_FLAG) // copy char
						buf[++j] = SIFTP_SERIALIZED_FLAG; // escape the flag
				}
				
				buf[j] = NULL;	// set null term
				
			return buf;
		}
		
		String siftp_unescape(const String a_str)
		{
			// variables
				String buf = NULL;
				int i, j, len;
			
			// check args
				if(a_str == NULL)		// no work to be done
					return NULL;
			
			// allocate space for buffer
				len = strlen(a_str);
				
				if((buf = malloc(++len * sizeof(char))) == NULL) // +1 for null term
				{
					fprintf(stderr, "siftp_unescape: malloc() failed.\n");
					return false;
				}
				
			// copy payload into buffer whilst unescaping it
				for(i=0, j=0; i < len; i++, j++)
				{
					if((buf[j] = a_str[i]) == SIFTP_SERIALIZED_FLAG) // copy char
						i++; // skip rest of the escaped flag
				}
				
				buf[j] = NULL;	// set null term
				
			return buf;
		}
		
		String siftp_serialize(const Message *ap_msg)
		{
			// variables
				String buf = NULL, paramEscaped;
				
			// check args
				if(ap_msg == NULL)		// no work to be done
					return NULL;
			
			// allocate space for buffer
				if((buf = malloc(SIFTP_SERIALIZED_MESSAGE_SIZE * sizeof(char))) == NULL)
				{
					fprintf(stderr, "siftp_serialize: malloc() failed.\n");
					return false;
				}
				
			// escape payload
				if((paramEscaped = siftp_escape(ap_msg->m_param)) == NULL)
				{
					free(buf);
					
					fprintf(stderr, "siftp_serialize: payload escaping failed.\n");
					return false;
				}
				
			// assemble string
				buf[0] = SIFTP_SERIALIZED_FLAG;
				strcpy(&buf[1], ap_msg->m_verb);
				strcpy(&buf[SIFTP_SERIALIZED_VERB_SIZE], paramEscaped);
				buf[strlen(paramEscaped)+1] = SIFTP_SERIALIZED_FLAG;
				
			return buf;
		}
		
		Message* siftp_deserialize(const String a_str)
		{
			// variables
				Message *p_msg = NULL;
				String verb, param, paramEnd; // these are unescaped
			
			// check args
				if(a_str == NULL || a_str[0] != SIFTP_SERIALIZED_FLAG || (paramEnd = strrchr(a_str, SIFTP_SERIALIZED_FLAG)) == NULL)	// no work to be done
					return NULL;
				
			// allocate space
				if((verb = malloc((SIFTP_SERIALIZED_VERB_SIZE+1) * sizeof(char))) == NULL)
				{
					fprintf(stderr, "siftp_deserialize: malloc() failed.\n");
					return false;
				}
				
				if((param = malloc((SIFTP_SERIALIZED_PARAMETER_SIZE+1) * sizeof(char))) == NULL)
				{
					free(verb);
					
					fprintf(stderr, "siftp_deserialize: malloc() failed.\n");
					return false;
				}
			
			// parse serialized string
				strncpy(verb, a_str[1], SIFTP_SERIALIZED_VERB_SIZE);
				verb[SIFTP_SERIALIZED_VERB_SIZE] = NULL;
				
				strncpy(param, a_str[SIFTP_SERIALIZED_VERB_SIZE], paramEnd - &a_str[1]); // includes null term
				
			// build Message object
				p_msg = Message_create(siftp_unescape(verb), siftp_unescape(param));
			
			// clean up
				free(param);
				free(verb);
				
			return p_msg;
		}

		Boolean siftp_query(const int a_sockfd, const Message *ap_query, Message *ap_response)
		{
			// variables
				String msgStr = NULL;	// holds serialized message
			
			// serialize message
				if((msgStr = siftp_msgToStr(ap_query)) == NULL)
				{
					fprintf(stderr, "siftp_query: Message serialization failed.\n");
					return false;
				}
				
			// perform dialouge
				send(a_sockfd, msgStr, strlen(msgStr), 0);
				recv(a_sockfd, (void *)msgStr, sizeof(msgStr), 0);
				
			// clean up
				free(msgStr);
				
			return true;
		}
		
