//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//
//  Copyright (c) 2003 Waldemar Kornewald, Waldemar.Kornewald@web.de
//---------------------------------------------------------------------

#ifndef _K_PPP_REPORT_DEFS__H
#define _K_PPP_REPORT_DEFS__H


#define PPP_REPORT_DATA_LIMIT		128
	// how much optional data can be added to the report
#define PPP_REPORT_CODE				'_3PR'
	// the code field of read_port


// report flags
enum PPP_REPORT_FLAGS {
	PPP_NO_REPORT_FLAGS = 0,
	PPP_WAIT_FOR_REPLY = 0x1,
	PPP_REMOVE_AFTER_REPORT = 0x2,
	PPP_NO_REPLY_TIMEOUT = 0x4
};

// report types
enum PPP_REPORT_TYPE {
	PPP_DESTRUCTION_REPORT = 0,
		// the interface is being destroyed (no code is needed)
	PPP_CONNECTION_REPORT = 1,
	PPP_AUTHENTICATION_REPORT = 2
};

// report codes (type-specific)
enum PPP_CONNECTION_REPORT_CODES {
	PPP_REPORT_GOING_UP = 0,
	PPP_REPORT_UP_SUCCESSFUL = 1,
	PPP_REPORT_DOWN_SUCCESSFUL = 2,
	PPP_REPORT_UP_ABORTED = 3,
	PPP_REPORT_DEVICE_UP_FAILED = 4,
	PPP_REPORT_AUTHENTICATION_SUCCESSFUL = 5,
	PPP_REPORT_PEER_AUTHENTICATION_SUCCESSFUL = 6,
	PPP_REPORT_AUTHENTICATION_FAILED = 7,
	PPP_REPORT_CONNECTION_LOST = 8
};


typedef struct ppp_report_packet {
	int32 type;
	int32 code;
	uint8 length;
	char data[PPP_REPORT_DATA_LIMIT];
} ppp_report_packet;



//***********
// private
//***********
#define PPP_REPORT_TIMEOUT				10

typedef struct ppp_report_request {
	thread_id thread;
	int32 type;
	int32 flags;
} ppp_report_request;


#endif
