/*
 * Creates a server so that backend processes can connect.
 */
#include "server.h"
#include "transaction.h"
#include <mutex>
static unsigned long gtrans_id = 1;
static mutex t_mx;


/* Returns a new transaction */
trans_t *new_transaction()
{
	trans_t *t = new trans_t;
	unique_lock<mutex> lck(t_mx);
	t->trans_id = gtrans_id++;
	lck.unlock();
	return t;
}

static void *incoming(void *pctx)
{
	/* Recieve */
	bool cont = true;
	int rcv;
	ssize_t ret;
	rep_msg_t *rep = NULL;

	session *ctx = (session *)pctx;
	c_sock *cs = ctx->cs;
	cout << "Spawning the connection thread." << endl;
	while(cont) {
		rq_msg_t rmsg;
		ret = cs->c_sock_read((void *)&rmsg, sizeof(rq_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		//rep = msg_handler(rmsg);
		if(rep == NULL)
			break;
		ret = cs->c_sock_write(rep, sizeof(rep_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}
	}
	cs->c_sock_close();
	return NULL;
}

void cmdt_open_server(coomdt_t *cmdt)
{
	int rc;

	c_sock *bs = new c_sock;
	if(bs == NULL) {
		cr_log<<"Unable to open a socket:" << endl;
		return ;	
	}

	rc = bs->c_sock_addr(cmdt->cmdt_ip_addr, cmdt->cmdt_port);
	if(rc != 0) {
		cr_log << "Invalid Addresses" << endl;
		delete bs;
		return ;
	}

	cout << "CMDT server PORT:" << cmdt->cmdt_port << endl;
	rc = bs->c_sock_bind();
	if(rc < 0) {
		delete bs;
		cr_log << "Unable to bind" << endl;
		return ;
	}
	bs->c_sock_listen();

	while(1) {
		c_sock *cs = bs->c_sock_accept();
		if(cs == NULL) {
			cr_log << "Socket not connected:" << errno << endl;
			return ;
		}
		cout << "Client connected...." << endl;
		conn_mdt_t *ct = new conn_mdt_t;
		ct->state = CONNECTED;
		ct->sock = cs;
		(cmdt->conn_dt).push_back(ct); 
	}
}
