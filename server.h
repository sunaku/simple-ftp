/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP server interface.
**/

#ifndef SERVER_H
#define SERVER_H

#include "siftp.h"

	/* debugging */
	
	/* constants */
	
		#define SERVER_COMMAND_SIZE	4	/* max length of cmd */
		#define SERVER_SOCKET_BACKLOG	5
		
		#define SERVER_PASSWORD	"ce150"
	
	/* typedefs */
	
	/* structs */
		
	/* constructors */
		
	/* utility functions */
	
		/**
		 * Establishes a SimpleFTP session.
		 */
		Boolean session_create(const int a_socket);
		
		/**
		 * Destroys an established SimpleFTP session.
		 */
		Boolean session_destroy(const int a_socket);
		
		/**
		 * Establishes a network service on the specified port.
		 * @param	ap_socket		Storage for socket descriptor.
		 */
		Boolean service_create(int *ap_socket, const String a_port);
		
		/**
		 * Performs client interaction.
		 */
		void service_loop(const int a_socket);
		
	/* command funcs */
	
		Boolean cmd_ls(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_pwd(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_cd(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_get(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_put(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		
#endif
