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
	
		#define SIFTP_PASSWORD	"CE150"
		#define SIFTP_PASSWORD_SIZE	8
		
		#define	SIFTP_VERBS_SESSION_BEGIN	"HELO"
		#define	SIFTP_VERBS_SESSION_END	"GBYE"
		
		#define	SIFTP_VERBS_IDENTIFY	"IDNT"
		#define	SIFTP_VERBS_USERNAME	"USER"
		#define	SIFTP_VERBS_PASSWORD	"PASS"
		
		#define	SIFTP_VERBS_ACCEPTED	"ACPT"
		#define	SIFTP_VERBS_DENIED	"DENY"
		
		#define	SIFTP_VERBS_PROCEED	"PRCD"
		#define	SIFTP_VERBS_ABORT	"ABRT"
		
		#define	SIFTP_VERBS_COMMAND	"CMND"
		#define SIFTP_VERBS_COMMAND_STATUS	"CMST"
		
		#define SIFTP_VERBS_DATA_BEGIN	"DBEG"
		#define SIFTP_VERBS_DATA_STREAM	"DSTR"
		#define SIFTP_VERBS_DATA_END	"DEND"
		
		
		#define SIFTP_SERIALIZED_FLAG	0x10
		#define SIFTP_SERIALIZED_MESSAGE_SIZE	256
		#define SIFTP_SERIALIZED_VERB_SIZE	4
		#define SIFTP_SERIALIZED_PARAMETER_SIZE	( SIFTP_SERIALIZED_MESSAGE_SIZE - SIFTP_SERIALIZED_VERB_SIZE - 2 ) /* -2 for the bounding flags */

	/* typedefs */
	
		typedef struct TAG_Message Message;
		typedef enum { false, true } Boolean;
		typedef char* String;
		
	/* structs */
	
		struct TAG_Message
		{
			String m_verb, m_param; /* the payload */
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
		void Message_destroy(Message *ap_msg);
		
	/* utility functions */
	
		/**
		 * Escapes the SimpleFTP flags in the given string.
		 * Note: returns a malloc()ed object.
		 */
		String siftp_escape(const String a_str);
		
		/**
		 * Unescapes SimpleFTP flags in the payload.
		 * Note: returns a malloc()ed object.
		 */
		String siftp_unescape(const String a_str);
		
		/**
		 * Serializes a Message object for network transport.
		 */
		String siftp_serialize(const Message *ap_msg);
		
		/**
		 * Deserializes a Message object from network transport.
		 */
		Message* siftp_deserialize(const String a_str);
		
		/**
		 * Performs a simple query/response dialogue.
		 */
		Boolean siftp_query(const int a_sockfd, const Message *ap_query, Message *ap_response);
		
		
#endif