/**
 * Suraj Kurapati <skurapat@ucsc.edu>
 * CMPS-150, Spring04, final project
 *
 * SimpleFTP protocol structures interface.
 *
 * Serialized message format: flag|verb|parameter|flag
**/


#ifndef SIFTP_H
#define SIFTP_H

	/* debugging */
	
	/* constants */
	
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
			
			/** param fmt: [status_byte] */
			#define SIFTP_VERBS_COMMAND_STATUS	"CMST"
			
			#define SIFTP_VERBS_DATA_GRAM	"DGRM"
			
			/** param fmt: [data_size_in_bytes] */
			#define SIFTP_VERBS_DATA_STREAM_HEADER	"DSTH"
			#define SIFTP_VERBS_DATA_STREAM_HEADER_LENFMT	"%d"
			#define SIFTP_VERBS_DATA_STREAM_PAYLOAD	"DSTP"
			#define SIFTP_VERBS_DATA_STREAM_TAILER	"DSTT"
		
		// sizes
			#define SIFTP_FLAG	0x10
			#define SIFTP_MESSAGE_SIZE	128
			#define SIFTP_VERB_SIZE	4
			#define SIFTP_PARAMETER_SIZE	( SIFTP_MESSAGE_SIZE - SIFTP_VERB_SIZE )

	/* typedefs */
	
		typedef struct TAG_Message Message;
		typedef enum { false, true } Boolean;
		typedef char* String;
		
	/* structs */
	
		struct TAG_Message
		{
			char m_verb[SIFTP_VERB_SIZE+1], m_param[SIFTP_PARAMETER_SIZE+1];
		};
		
	/* constructors */
		
		/**
		 * Constructs a Message object.
		 * Note: returns a malloc()ed object.
		 */
		Message* Message_create(String a_verb, String a_param);
		
		/**
		 * Destroys a Message object.
		 */
		inline void Message_destroy(Message *ap_msg);
		
		/**
		 * Null terminates member strings (character arrays).
		 * @pre	ap_msg != NULL
		 */
		inline void Message_reset(Message *ap_msg);
		
	/* utility functions */
	
		/**
		 * Escapes the SimpleFTP flags in the given string.
		 * Note: returns a malloc()ed object.
		 * @deprecated
		 */
		String siftp_escape(const String a_str);
		
		/**
		 * Unescapes SimpleFTP flags in the payload.
		 * Note: returns a malloc()ed object.
		 * @deprecated
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
		 * Performs a simple query/response dialogue.
		 */
		Boolean siftp_query(const int a_socket, const Message *ap_query, Message *ap_response);
		
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
		 * Performs a data transfer dialouge.
		 * @param	a_socket	Socket descriptor on which to receive.
		 * @param	ap_length	Storage for length (including null terminator) of received data.
		 * Note: returns a malloc()ed object.
		 */
		String siftp_recvData(const int a_socket, int *ap_length);
#endif
