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
	
		#define SERVER_SOCKET_BACKLOG	5
		#define SERVER_PASSWORD	"ce150"
	
	/* typedefs */
	
	/* structs */
		
	/* constructors */
		
	/* utility functions */
	
		/**
		 * Establishes a network service on the specified port.
		 * @param	ap_socket		Storage for socket descriptor.
		 */
		Boolean service_create(int *ap_socket, const String a_port);
		
#endif
