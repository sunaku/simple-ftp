/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP server interface.
**/

#ifndef SERVER_H
#define SERVER_H

#include "siftp.h"

	/* constants */
	
		#define SERVER_SOCKET_BACKLOG	5
		#define SERVER_PASSWORD	"ce150"
	
	/* services */
	
		/**
		 * Establishes a network service on the specified port.
		 * @param	ap_socket	Storage for socket descriptor.
		 * @param	a_port	Port number in decimal.
		 */
		Boolean service_create(int *ap_socket, const int a_port);
		
#endif
