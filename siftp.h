/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP protocol interface/definition.
 *
 * Provides transport layer services such as sending and
 * receiving Message streams and Message grams. This also
 * defines the vocabulary and syntax for communication.
 *
**/

#ifndef SIFTP_H
#define SIFTP_H

#include <string.h>

	/* constants */
	
		/* Serialized message format: verb|parameter */
		
		// dialouge
			#define SIFTP_VERBS_SESSION_BEGIN	"HELO"
			#define SIFTP_VERBS_SESSION_END	"GBYE"
			
			#define SIFTP_VERBS_IDENTIFY	"IDNT"
			#define SIFTP_VERBS_USERNAME	"USER"
			#define SIFTP_VERBS_PASSWORD	"PASS"
			
			#define SIFTP_VERBS_ACCEPTED	"ACPT"
			#define SIFTP_VERBS_DENIED	"DENY"
			
			#define SIFTP_VERBS_PROCEED	"PRCD"
			#define SIFTP_VERBS_ABORT	"ABRT"
			
			#define SIFTP_VERBS_COMMAND	"CMND"
			#define SIFTP_VERBS_COMMAND_STATUS	"CMST"
			
			#define SIFTP_VERBS_DATA_STREAM_HEADER	"DSTH" /** param = [data_size_in_bytes] */
			#define SIFTP_VERBS_DATA_STREAM_HEADER_LENFMT	"%d"
			#define SIFTP_VERBS_DATA_STREAM_PAYLOAD	"DSTP"
			#define SIFTP_VERBS_DATA_STREAM_TAILER	"DSTT"
		
			#define SIFTP_FLAG	0x10
			
		// sizes in bytes
			#define SIFTP_MESSAGE_SIZE	( 1 << 10 )
			#define SIFTP_VERB_SIZE	4
			#define SIFTP_PARAMETER_SIZE	( SIFTP_MESSAGE_SIZE - SIFTP_VERB_SIZE )

	/* typedefs */
	
		typedef struct TAG_Message Message;
		typedef enum { false, true } Boolean;
		typedef char* String;
		
	/* data structures */
	
		/**
		 * The basic communication primitive.
		 */
		struct TAG_Message
		{
			/** the message type/qualifier/preamble */
			char m_verb[SIFTP_VERB_SIZE+1];
			
			/** the message content */
			char m_param[SIFTP_PARAMETER_SIZE+1];
		};
		
		/* constructors */
			
			/**
			 * Constructs a Message object.
			 * @note	returns a dynamically allocated object.
			 */
			Message* Message_create(String a_verb, String a_param);
			
			/**
			 * Destroys a Message object.
			 */
			void Message_destroy(Message *ap_msg);
			
		/* accessors */
		
			/**
			 * Returns the type of the Message.
			 * @pre	ap_msg != NULL
			 */
			#define Message_getType(ap_msg) ( (ap_msg)->m_verb )
			
			/**
			 * Changes the type of the Message.
			 * @pre	ap_msg != NULL
			 * @pre	arg is null terminated
			 */
			#define Message_setType(ap_msg, arg) ( strcpy((ap_msg)->m_verb, arg) )
			
			/**
			 * Returns the value of the Message.
			 * @pre	ap_msg != NULL
			 */
			#define Message_getValue(ap_msg) ( (ap_msg)->m_param )
			
			/**
			 * Changes the value of the Message.
			 * @pre	ap_msg != NULL
			 * @pre	arg is null terminated
			 */
			#define Message_setValue(ap_msg, arg) ( strcpy((ap_msg)->m_param, arg) )
			
			/**
			 * Checks if type of the Message is equal to the given type.
			 * @pre	ap_msg != NULL
			 * @pre	value is null terminated
			 */
			#define Message_hasType(ap_msg, type) ( strcmp((ap_msg)->m_verb, type) == 0 )
			
			/**
			 * Checks if value of the Message is equal to the given value.
			 * @pre	ap_msg != NULL
			 * @pre	value is null terminated
			 */
			#define Message_hasValue(ap_msg, value) ( strcmp((ap_msg)->m_param, value) == 0 )
			
			/**
			 * Fills the Message member values with zeros.
			 * @pre	ap_msg != NULL
			 */
			#define Message_clear(ap_msg) ( memset(ap_msg, 0, sizeof(Message)) )
		
		
	/* services */
	
		/**
		 * Escapes the SimpleFTP flags in the given string.
		 * @note	returns a dynamically allocated object.
		 * @deprecated	using fixed length messages instead.
		 */
		String siftp_escape(const String a_str);
		
		/**
		 * Unescapes SimpleFTP flags in the payload.
		 * @note	returns a dynamically allocated object.
		 * @deprecated	using fixed length messages instead.
		 */
		String siftp_unescape(const String a_str);
		
		/**
		 * Serializes a Message object for network transport.
		 * @param	ap_msg		Object to serialze
		 * @param	a_result	Storage (minimum size = SIFTP_MESSAGE_SIZE) where the resulting serialized string will be placed
		 */
		Boolean siftp_serialize(const Message *ap_msg, String a_result);
		
		/**
		 * Deserializes a Message object from network transport.
		 * @param	a_str		String to deserialize
		 * @param	ap_result	Storage where resulting deserialized object will be placed
		 */
		Boolean siftp_deserialize(const String a_str, Message *ap_result);
		
		/**
		 * Performs a one-way dialouge.
		 */
		Boolean siftp_send(const int a_socket, const Message *ap_query);
		
		/**
		 * Waits for a one-way dialouge.
		 */
		Boolean siftp_recv(const int a_socket, Message *ap_response);
		
		/**
		 * Performs a data transfer dialouge.
		 * @param	a_socket	Socket descriptor on which to send.
		 * @param	a_data	The data to send.
		 * @param	a_length	Number of bytes to send.
		 */
		Boolean siftp_sendData(const int a_socket, const String a_data, const int a_length);
		
		/**
		 * Returns the data from a transfer dialouge. The returned data is null terminated.
		 * @param	a_socket	Socket descriptor on which to receive.
		 * @param	ap_length	Storage for length of received data (excluding null terminator).
		 * @note	returns a dynamically allocated object.
		 */
		String siftp_recvData(const int a_socket, int *ap_length);
#endif

