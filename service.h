/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP cilent/server common services interface.
**/

#ifndef SERVICE_H
#define SERVICE_H

#include "siftp.h"

	/* constants */
	
		// max length of command name
		#define SERVICE_COMMAND_SIZE	4
		#define SERVICE_COMMAND_ARGUMENT_DELIMITERS	" \t"
		
		// default permissions when writing a file
		#define SERVICE_FILE_PERMS	( S_IREAD | S_IWRITE )
	
		
		
	/* services */
	
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
		 * Performs a simple two-way query & response dialouge.
		 */
		Boolean service_query(const int a_socket, const Message *ap_query, Message *ap_response);
		
		/**
		 * Returns a string containing the absolute path which joins the basePath to the extension.
		 * @param a_result	Storage for resulting path; must have minimum size PATH_MAX
		 */
		Boolean service_getAbsolutePath(const String a_basePath, const String a_extension, String a_result);
		
		/**
		 * Sends a command status acknowlegdement for a command.
		 */
		Boolean service_sendStatus(const int a_socket, const Boolean a_wasSuccess);

		/**
		 * Returns an array of words parsed from the command string. Assumes words are separated by spaces.
		 * @param	ap_argc	Storage for length of array.
		 * Note: returns a malloc()ed object.
		 */
		String* service_parseArgs(const String a_cmdStr, int *ap_argc);
		
		/**
		 * Frees memory associated with the array created exclusively by parseArgs().
		 * @param	a_argc	Number of items in array.
		 */
		void service_freeArgs(String *ap_argv, const int a_argc);
		
		/**
		 * Returns the value of the command staus acknowlegdement.
		 */
		inline Boolean service_recvStatus(const int a_socket);

		/**
		 * Performs a remote command and returns its status.
		 */
		inline Boolean remote_exec(const int a_socket, Message *ap_query);
		
		/**
		 * Handles a command occuring in interaction/dialouge.
		 */
		Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc);
		
		/**
		 * Changes the path of the current working dir.
		 * @param	a_currPath	Storage for resolved value and value of current path.
		 */
		Boolean service_handleCmd_chdir(String a_currPath, const String a_newPath);
		
		/**
		 * Returns contents of a file.
		 * @param	ap_length	Storage for length (including null terminator) of data.
		 * Note: returns a malloc()ed object.
		 */
		String service_readFile(const String a_path, int *ap_length);
		
		/**
		 * Returns contents of a dir, one entry per line.
		 * @param	ap_length	Storage for length (including null terminator) of data.
		 * Note: returns a malloc()ed object.
		 */
		String service_readDir(const String a_path, int *ap_length);
		
		/**
		 * Writes data whilst overwriting any existing file at the given path.
		 * @param	a_path	Path to which data will be written.
		 * @param	a_data	Data to be written.
		 * @param	a_length	Number of bytes of data to write.
		 * @param	a_mode	Permission bits.
		 */
		Boolean service_writeFile(const String a_path, const String a_data, const int a_length, const int a_mode);
		
#endif

