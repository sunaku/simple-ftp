/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP cilent/server common services interface.
 *
 * Provides common application level services to and
 * serves as a application model for the client & server.
**/

#ifndef SERVICE_H
#define SERVICE_H

#include "siftp.h"

	/* constants */
	
		/** max length of a service command's name */
		#define SERVICE_COMMAND_SIZE	4
		
		/** argument separators in a command string */
		#define SERVICE_COMMAND_ARGUMENT_DELIMITERS	" \t"
		
		#define SERVICE_PERMS_READ_TEST	"r"
		#define SERVICE_PERMS_WRITE_TEST	"a"
		
	/* services */
	
		/**
		 * Establishes a SimpleFTP session.
		 * @param	a_socket	Socket descriptor.
		 */
		Boolean session_create(const int a_socket);
		
		/**
		 * Destroys an established SimpleFTP session.
		 * @param	a_socket	Socket descriptor.
		 */
		Boolean session_destroy(const int a_socket);
		
		/**
		 * Performs interaction/dialouge.
		 * @param	a_socket	Socket descriptor.
		 */
		void service_loop(const int a_socket);
	
		/**
		 * Performs a simple two-way query & response dialouge.
		 * @param	a_socket	Socket descriptor.
		 * @param	ap_query	Query message.
		 * @param	ap_response	Storage for response message.
		 */
		Boolean service_query(const int a_socket, const Message *ap_query, Message *ap_response);
		
		/**
		 * Returns a string containing the absolute path of the extension relative to the base path.
		 * @param	a_basePath	The origin to which the extension is relatve, unless the extension is itself an absolute path.
		 * @param	a_extension	The path we want to make absolute.
		 * @param a_result	Storage, which must have a minimum capacity of <tt>PATH_MAX</tt>, for resulting path.
		 */
		Boolean service_getAbsolutePath(const String a_basePath, const String a_extension, String a_result);
		
		/**
		 * Sends a command status acknowlegdement for a command.
		 * @param	a_socket	Socket descriptor.
		 * @param	a_wasSuccess	Status value.
		 */
		Boolean service_sendStatus(const int a_socket, const Boolean a_wasSuccess);

		/**
		 * Returns an array of words parsed from the command string. Assumes words are separated by <tt>SERVICE_COMMAND_ARGUMENT_DELIMITERS</tt> characters.
		 * @param	a_cmdStr	String to parse.
		 * @param	ap_argc	Storage for length of array.
		 * @note	returns a dynamically allocated object.
		 */
		String* service_parseArgs(const String a_cmdStr, int *ap_argc);
		
		/**
		 * Frees memory associated with the array created exclusively by <tt>parseArgs()</tt>.
		 * @param	ap_argv	Array of arguments gotten from <tt>parseArgs()</tt>.
		 * @param	a_argc	Number of items in array.
		 */
		void service_freeArgs(String *ap_argv, const int a_argc);
		
		/**
		 * Returns the value of the command staus acknowlegdement.
		 * @param	a_socket	Socket descriptor.
		 */
		Boolean service_recvStatus(const int a_socket);

		/**
		 * Performs a remote command and returns its status.
		 * @param	a_socket	Socket descriptor.
		 * @param	ap_query	Message containing command.
		 */
		Boolean remote_exec(const int a_socket, Message *ap_query);
		
		/**
		 * Handles a command occuring in interaction/dialouge.
		 * @param	a_socket	Socket descriptor.
		 * @param	ap_argv	Array of arguments.
		 * @param	a_argc	Number of arguments in array.
		 */
		Boolean service_handleCmd(const int a_socket, const String *ap_argv, const int a_argc);
		
		/**
		 * Changes the path of the current working dir.
		 * @param	a_currPath	Storage for resolved value and value of current working directory.
		 * @param	a_newPath	The path to which we want to change the current working directory.
		 */
		Boolean service_handleCmd_chdir(String a_currPath, const String a_newPath);
		
		/**
		 * Returns true if the path is accessible under the given permissions.
		 * @param	a_path	Path to test.
		 * @param	a_mode	Access mode (same as fopen()).
		 * @see <tt>fopen</tt> for access modes.
		 */
		Boolean service_permTest(const String a_path, const String a_mode);
		
		/**
		 * Returns true if the path has all of the given attributes. <tt>errno</tt> is also set upon failure.
		 * @param	a_path	Path to test.
		 * @param	a_testMode	Attributes to test.
		 * @param	a_resultMode	Expected results of test.
		 * @see	stat.st_mode
		 */
		Boolean service_statTest(const String a_path, const int a_testMode, const int a_resultMode);
		
		/**
		 * Returns contents of a file or NULL upon error.
		 * @param	a_path	Path of the file to read.
		 * @param	ap_length	Storage for length of data.
		 * @note	returns a dynamically allocated object.
		 */
		String service_readFile(const String a_path, int *ap_length);
		
		/**
		 * Returns names of all files in the given directory. Each directory entry name is separated by a newline. The trailing newline is replaced by a null terminator.
		 * @param	a_path	Path of the directory to read.
		 * @param	ap_length	Storage for length of data.
		 * @note	returns a dynamically allocated object.
		 */
		String service_readDir(const String a_path, int *ap_length);
		
		/**
		 * Writes data whilst overwriting any existing file at the given path.
		 * @param	a_path	Path to which data will be written.
		 * @param	a_data	Data to be written.
		 * @param	a_length	Number of bytes of data to write.
		 */
		Boolean service_writeFile(const String a_path, const String a_data, const int a_length);
		
#endif

