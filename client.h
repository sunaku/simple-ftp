/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP client interface.
**/

#ifndef CLIENT_H
#define CLIENT_H

#include "siftp.h"

	/* debugging */
	
	/* constants */
	
		#define CLIENT_COMMAND_SIZE	4	/* max length of cmd */
		#define CLIENT_INPUT_SIZE ( SIFTP_MESSAGE_SIZE + CLIENT_COMMAND_SIZE )
	
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
		inline Boolean session_destroy(const int a_socket);
		
		/**
		 * Establishes a network connection with the specified server.
		 * @param	ap_socket		Storage for socket descriptor.
		 */
		Boolean link_create(int *ap_socket, const String a_serverName, const String a_serverPort);
		
		/**
		 * Performs user interaction.
		 */
		void input_loop(const int a_socket);
		
	/* command funcs */
	
		Boolean cmd_ls(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_pwd(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_cd(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_get(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		Boolean cmd_put(const int a_socket, const Boolean a_isLocal, const String a_commandLine);
		
#endif
