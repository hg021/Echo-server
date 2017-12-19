/*
 * tftp_util.h
 *-----------------------------------------------------
 * Structures needed for TFTP server
 * AUTHOR: Himanshu Gupta
 * himgupt2@tamu.edu
 * 
 * ECEN 602 
 * November 2017
 * ----------------------------------------------------
 */

#ifndef __TFTP_UTIL_H_
#define __TFTP_UTIL_H_

#include<stdint.h>

#define BLOCK_SIZE 512



/* Opcodes for tftp operations*/
typedef enum tftp_opcode_types_ {
	TFTP_RRQ = 1,		/* Read request operation (RRQ) */
	TFTP_WRQ,		/* Write request operation(WRQ) */
	TFTP_DATA,		/* Data block */
	TFTP_ACK,		/* Acknowledgement for tftp request */
	TFTP_ERROR,		/* Error response */
} tftp_opcode_types;


typedef struct tftp_req_ {
	uint16_t 	code;	/* Includes the message type RRQ/WRQ*/
	char		*fname; /* File name requested */
	uint8_t		eof1;	/* EOF mark */
	char		*mode;	/* file mode */
	uint8_t		eof2;	/* EOF mark */
} tftp_req;

typedef struct tftp_data_ {
	uint16_t	code;
	uint16_t	bnum;
	char		data[BLOCK_SIZE + 1];
} tftp_data;


typedef struct tftp_ack_ {
	uint16_t	code;
	uint16_t	bnum;
} tftp_ack;

typedef struct tftp_err_ {
	uint16_t	code;
	uint16_t	err_code;
	char 		err_msg[BLOCK_SIZE];
	uint8_t		eof;
} tftp_err;


#endif
