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
		
#endif

