/*
 * sbcp_util.h
 *-----------------------------------------------------
 * Structures needed for SBCP server
 * AUTHOR: Himanshu Gupta
 * himgupt2@tamu.edu
 * 
 * ECEN 602 
 * October 2017
 * ----------------------------------------------------
 */


#ifndef __SBCP_UTIL_H__
#define __SBCP_UTIL_H__

#define MAX_CLIENT_ID 20

/* Header types for SBCP message */
typedef enum sbcp_hdr_types_ {
	SB_JOIN = 2,	/* Client sends to server to join chat session */
	SB_FWD,		/* Server sends to client, contains chat text  */
	SB_SEND,	/* Client sends to  server, contains chat text */
	SB_NAK,		/* Server sends to client to reject a request  */
	SB_OFFLINE,	/* Server sends to client indicating departure of chat participant */
	SB_ACK,		/* Server sends to client to confirm the JOIN request		   */
	SB_ONLINE,	/* Server sends to client indicating arrival of chat participant   */
	SB_IDLE,	/* Client sends to server indicating it has been idle, server 
			   sends to client indicating the username which is idle */
} sbcp_hdr_types;

/* Header format for SBCP message */
typedef struct sbcp_hdr_ {
	unsigned int vrsn : 9;
	unsigned int type : 7;
	unsigned int length;
} sbcp_hdr;

/* Payload types for SBCP Attribute*/
typedef enum sbcp_attr_types_ {
	AT_REASON = 1,	/* Text indicating reason of failure */
	AT_USRNAME,	/* Nickname client is using for chat */
	AT_COUNT,	/* Number of client in chat session  */
	AT_MESSAGE,	/* Chat message			     */
	AT_MAX,		/* Max attribute values		     */
} sbcp_attr_types;

/* SBCP Attribute structure */
typedef struct sbcp_attr_ {
	int type;
	int length;
	char payload[512];
} sbcp_attr;

/* SBCP message structure*/
typedef struct sbcp_msg_ {
	sbcp_hdr hdr;
	sbcp_attr attr[AT_MAX];
} sbcp_msg;


/* SBCP client information format */
typedef struct sbcp_client_ {
	char usrname[MAX_CLIENT_ID];
	int connfd;
	int count;
} sbcp_client;


#endif
