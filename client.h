/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP client interface.
**/

#ifndef CLIENT_H
#define CLIENT_H

#include "siftp.h"

	/* services */
	
		/**
		 * Establishes a network connection with the specified server.
		 * @param	ap_socket	Storage for socket descriptor.
		 * @param	a_serverName	Canonical name or IP address of remote host.
		 * @param	a_serverPort	Port number in decimal of remote host.
		 */
		Boolean service_create(int *ap_socket, const String a_serverName, const int a_serverPort);

#endif
