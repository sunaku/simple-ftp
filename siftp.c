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
				if(a_verb != NULL)
					strcpy(p_msg->m_verb, a_verb);
				
				if(a_param != NULL)
					strcpy(p_msg->m_param, a_param);
			}
		
		return p_msg;
	}
	
	inline void Message_destroy(Message *ap_msg)
	{
		if(ap_msg != NULL)
			free(ap_msg);
	}
	
	inline void Message_reset(Message *ap_msg)
	{
		memset(ap_msg, 0, sizeof(Message));
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
					fprintf(stderr, "siftp_escape(): malloc() failed.\n");
					return NULL;
				}
				
			// copy payload into buffer whilst escaping it
				for(i=0, j=0; i < len; i++, j++)
				{
					if((buf[j] = a_str[i]) == SIFTP_FLAG) // copy char
						buf[++j] = SIFTP_FLAG; // escape the flag
				}
				
				buf[j] = '\0';	// set null term
				
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
					fprintf(stderr, "siftp_unescape(): malloc() failed.\n");
					return NULL;
				}
				
			// copy payload into buffer whilst unescaping it
				for(i=0, j=0; i < len; i++, j++)
				{
					if((buf[j] = a_str[i]) == SIFTP_FLAG) // copy char
						i++; // skip rest of the escaped flag
				}
				
				buf[j] = '\0';	// set null term
				
			return buf;
		}
		
		Boolean siftp_serialize(const Message *ap_msg, String a_result)
		{
			// variables
				
			// check args
				if(ap_msg == NULL)		// no work to be done
					return true;
			
			// assemble string
				strncpy(a_result, ap_msg->m_verb, SIFTP_VERB_SIZE);
				strncpy(&a_result[SIFTP_VERB_SIZE], ap_msg->m_param, SIFTP_PARAMETER_SIZE);
				
				#ifndef NODEBUG
					printf("serialize(): result='%s'\n", a_result);
				#endif
				
			return true;
		}
		
		Boolean siftp_deserialize(const String a_str, Message *ap_result)
		{
			// variables
			
			// check args
				if(a_str == NULL)	// no work to be done
					return true;
				
			// parse serialized string
				strncpy(ap_result->m_verb, a_str, SIFTP_VERB_SIZE);
				strncpy(ap_result->m_param, &a_str[SIFTP_VERB_SIZE], SIFTP_PARAMETER_SIZE);
				
				#ifndef NODEBUG
					printf("deserialize(): message [verb='%s',param='%s']\n", ap_result->m_verb, ap_result->m_param);
				#endif
				
			return true;
		}

		Boolean siftp_query(const int a_socket, const Message *ap_query, Message *ap_response)
		{
			// variables
				char buf[SIFTP_MESSAGE_SIZE];
				
			// serialize message
				if(!siftp_serialize(ap_query, buf))
				{
					fprintf(stderr, "siftp_query(): Message serialization failed.\n");
					return false;
				}
				
			// perform dialouge
				send(a_socket, buf, SIFTP_MESSAGE_SIZE, 0);
				recv(a_socket, buf, SIFTP_MESSAGE_SIZE, 0);
				
			// deserialize message
				return siftp_deserialize(buf, ap_response);
		}
		
		Boolean siftp_send(const int a_socket, const Message *ap_query)
		{
			// variables
				char buf[SIFTP_MESSAGE_SIZE];
				
			// serialize message
				if(!siftp_serialize(ap_query, buf))
				{
					fprintf(stderr, "siftp_query(): Message serialization failed.\n");
					return false;
				}
				
			// perform dialouge
				send(a_socket, buf, SIFTP_MESSAGE_SIZE, 0);
				
			return true;
		}
		
		Boolean siftp_recv(const int a_socket, Message *ap_response)
		{
			// variables
				char buf[SIFTP_MESSAGE_SIZE];
				
			// perform dialouge
				recv(a_socket, buf, SIFTP_MESSAGE_SIZE, 0);
				
			// deserialize message
				return siftp_deserialize(buf, ap_response);
		}
		
		Boolean siftp_sendData(const int a_socket, const String a_data, const int a_length)
		{
			#ifndef NODEBUG
				printf("siftp_sendData(): data length = %d\n", a_length);
			#endif
			
			// variables
				Message msgOut;
				int tempLen;
				
			// init vars
				memset(&msgOut, 0, sizeof(msgOut));
				
			if(a_length <= SIFTP_PARAMETER_SIZE) // send as "datagram"
			{
				strcpy(msgOut.m_verb, SIFTP_VERBS_DATA_GRAM);
				strcpy(msgOut.m_param, a_data);
				return siftp_send(a_socket, &msgOut);
			
			}
			else // send as data stream
			{
				// header
					strcpy(msgOut.m_verb, SIFTP_VERBS_DATA_STREAM_HEADER);
					sprintf(msgOut.m_param, SIFTP_VERBS_DATA_STREAM_HEADER_LENFMT, a_length);
					
					if(!siftp_send(a_socket, &msgOut))
						return false;
				
				// send data as discrete messages
					strcpy(msgOut.m_verb, SIFTP_VERBS_DATA_STREAM_PAYLOAD);
					
					for(tempLen=0; tempLen < a_length; tempLen += SIFTP_PARAMETER_SIZE)
					{
						memset(&msgOut.m_param, 0, sizeof(msgOut.m_param));
						strncpy(msgOut.m_param, &a_data[tempLen], SIFTP_PARAMETER_SIZE);
						if(!siftp_send(a_socket, &msgOut))
						return false;
					}
				
				// tailer
					strcpy(msgOut.m_verb, SIFTP_VERBS_DATA_STREAM_TAILER);
					return siftp_send(a_socket, &msgOut);
			}
		}
		
		String siftp_recvData(const int a_socket, int *ap_length)
		{
			// variables
				Message msgIn;
				String buf = NULL;
				int tempLen;
				
			// init
				memset(&msgIn, 0, sizeof(msgIn));
				
			// determine transfer type
				if(!siftp_recv(a_socket, &msgIn))
					return false;
				
				if(strcmp(msgIn.m_verb, SIFTP_VERBS_DATA_GRAM) == 0) // gram
				{
					*ap_length = strlen(msgIn.m_param);
					
					// allocate space
					if((buf = calloc(++*ap_length,  sizeof(char))) == NULL) // +1 for null term
					{
						fprintf(stderr, "cmd_ls(): calloc() failed.\n");
						return false;
					}
					
					strcpy(buf, msgIn.m_param);
				}
				else if(strcmp(msgIn.m_verb, SIFTP_VERBS_DATA_STREAM_HEADER) == 0) // stream
				{
					// allocate space
						sscanf(msgIn.m_param, SIFTP_VERBS_DATA_STREAM_HEADER_LENFMT, ap_length);
						
						#ifndef NODEBUG
							printf("siftp_recvData(): data length = %d\n", *ap_length);
						#endif
						
						if((buf = calloc(++*ap_length, sizeof(char))) == NULL) // +1 for null term
						{
							// cancel transmission
							
							fprintf(stderr, "cmd_ls(): calloc() failed.\n");
							return false;
						}
						
					// read stream into buffer
						tempLen=0;
						
						do
						{
							// read stream
								if(!siftp_recv(a_socket, &msgIn))
								{
									free(buf);
									return NULL;
								}
							
							// store data
								if(strcmp(msgIn.m_verb, SIFTP_VERBS_DATA_STREAM_PAYLOAD) == 0)
								{
									if(tempLen < *ap_length)
										strncpy(&buf[tempLen], msgIn.m_param, SIFTP_PARAMETER_SIZE);
									else
									{
										fprintf(stderr, "siftp_recvData(): receiving more data than expected; ignoring excess data.\n");
										break;
									}
									
									tempLen += SIFTP_PARAMETER_SIZE;
								}
						}
						while(strcmp(msgIn.m_verb, SIFTP_VERBS_DATA_STREAM_TAILER));
				}
				
			return buf;
		}

