/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP cilent/server application model.
**/

#ifndef MODEL_H
#define MODEL_H

#include "siftp.h"

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
		Boolean session_destroy(const int a_socket);
		
		/**
		 * Performs interaction/dialouge.
		 */
		void service_loop(const int a_socket);
	
		/**
		 * Executes a given command.
		 */
		Boolean service_command(const int a_socket, const String a_cmdStr, const String a_argStr);
		
#endif

