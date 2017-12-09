#ifndef __SERVER_H_
#define __SERVER_H_
#include <iostream>
#include <stdlib.h>
#include <thread>
#include <list>

#include "sock.h"
using namespace std;
typedef struct coordinator_mdata {
	char          *cmdt_ip_addr;
	int            cmdt_port;

	char          *cmdt_cl_ip;
	int            cmdt_cl_port;
	list<c_sock *> cmdt_sock;
} coomdt_t;

typedef struct server_mdata {
	char   *smdt_ip;
	int     smdt_port;
	char   *coord_ip;
	int     coord_port;
	c_sock *smdt_sock;
} smdt_t;

typedef struct session {
	char  *sip;
	int    sport;
	c_sock *cs;
} session_t;

enum msg_type {
	CREATE_REQ,
	CREATE_REP,

	UPDATE_REQ,
	UPDATE_REP,

	QUERY_REQ,
	QUERY_REP,

	QUIT,
};

typedef struct rq_create_msg {
	unsigned long amount;
} rq_cr_msg;

typedef struct rq_update_msg {
	unsigned long acc_nr;
	unsigned long amount;
} rq_update_msg;

typedef struct rq_query_msg {
	unsigned long acc_nr;
} rq_query_msg;

typedef struct request_msg {
	enum msg_type type;
	union {
		rq_cr_msg cr_msg;
		rq_update_msg up_msg;
		rq_query_msg q_msg;
	} u;
} rq_msg_t;

typedef struct rep_create_msg {
	unsigned long acc_nr;
	int err_code;
} rep_cr_msg;

typedef struct rep_update_msg {
	unsigned long amount;
	int err_code;
} rep_update_msg;

typedef struct rep_query_msg {
	unsigned long amount;
} rep_query_msg;

typedef struct reply_msg {
	enum msg_type type;
	union {
		rep_cr_msg cr_msg;
		rep_update_msg up_msg;
		rep_query_msg q_msg;
	} u;
} rep_msg_t;

void cmdt_open_client(coomdt_t *cmdt);
int boot_coord(char *ip, int port);
int connect_coord(char *ip, int port);
#endif
