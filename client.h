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
	
	/* typedefs */
	
	/* structs */
		
	/* constructors */
		
	/* utility functions */
	
		/**
		 * Establishes a network connection with the specified server.
		 * @param	ap_socket		Storage for socket descriptor.
		 */
		Boolean service_create(int *ap_socket, const String a_serverName, const String a_serverPort);
		
#endif
