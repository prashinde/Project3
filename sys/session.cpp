#include "server.h"
#include "transaction.h"
#include <mutex>
static unsigned long gacc_n = 1;
static mutex t_mx;

unsigned long get_next_acc()
{
	unsigned long t;
	t = gacc_n++;
	return t;
}

rep_msg_t *handle_create(rq_cr_msg cr_msg, coomdt_t *cmt)
{
	/* Allocate a new transaction */ 
	rep_cr_msg rep_cr;	
	unique_lock<mutex> lck(t_mx);
	trans_t *trans = new_transaction();
	trans->acc_nr = get_next_acc();
	trans->amount = cr_msg.amount;
	trans->t_op_type = CREATE;

	commit_transaction(trans, cmt);
	rep_msg_t *msg = new rep_msg_t;
	rep_cr.acc_nr = trans->acc_nr;
	rep_cr.err_code = trans->status;
	msg->u.cr_msg = rep_cr;
	msg->type = CREATE_REP;
	lck.unlock();
	delete trans;
	return msg;
}

rep_msg_t *handle_update(rq_update_msg u_msg, coomdt_t *cmt)
{
	rep_update_msg rep_up;
	unique_lock<mutex> lck(t_mx);
	trans_t *trans = new_transaction();
	trans->acc_nr = u_msg.acc_nr;
	trans->amount = u_msg.amount;
	trans->t_op_type = UPDATE;
	commit_transaction(trans, cmt);

	rep_msg_t *msg = new rep_msg_t;
	rep_up.err_code = trans->status;
	rep_up.amount = trans->amount;
	msg->u.up_msg = rep_up;
	msg->type = UPDATE_REP;
	delete trans;
	lck.unlock();
	return msg;	
}

rep_msg_t *handle_query(rq_query_msg q_msg, coomdt_t *cmt)
{
	rep_query_msg rep_q;
	rep_msg_t *msg = new rep_msg_t;

	unique_lock<mutex> lck(t_mx);
	trans_t *trans = new_transaction();
	trans->acc_nr = q_msg.acc_nr;
	trans->t_op_type = QUERY;
	/* Dont neet 2 phase commit for query */
	query(trans, cmt);
	rep_q.amount = trans->amount;
	rep_q.err_code = trans->status;
	msg->u.q_msg = rep_q;
	msg->type = QUERY_REP;
	delete trans;
	lck.unlock();
	return msg;	
}

rep_msg_t * msg_handler(rq_msg_t req, coomdt_t *cmt)
{
	rep_msg_t *rep;
	switch(req.type) {
		case CREATE_REQ:
		rep = handle_create(req.u.cr_msg, cmt);
		cout << "Sending reply to a client" << endl;
		break;
		case UPDATE_REQ:
		rep = handle_update(req.u.up_msg, cmt);
		cout << "Sending reply to a client" << endl;
		break;
		case QUERY_REQ:
		rep = handle_query(req.u.q_msg, cmt);
		cout << "Sending reply to a client" << endl;
		break;
		case QUIT:
		rep = NULL;
		break;
	}
	return rep;
}

static void *incoming(void *pctx)
{
	/* Recieve */
	bool cont = true;
	int rcv;
	ssize_t ret;
	rep_msg_t *rep;

	session *ctx = (session *)pctx;
	c_sock *cs = ctx->cs;
	cout << "Spawning the connection thread." << endl;
	while(cont) {
		rq_msg_t rmsg;
		ret = cs->c_sock_read((void *)&rmsg, sizeof(rq_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		rep = msg_handler(rmsg, ctx->cmt);
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

void cmdt_open_client(coomdt_t *cmdt)
{
	int rc;

	c_sock *bs = new c_sock;
	if(bs == NULL) {
		cr_log<<"Unable to open a socket:" << endl;
		return ;	
	}

	rc = bs->c_sock_addr(cmdt->cmdt_cl_ip, cmdt->cmdt_cl_port);
	if(rc != 0) {
		cr_log << "Invalid Addresses" << endl;
		delete bs;
		return ;
	}

	cout << "Clients connect on PORT:" << cmdt->cmdt_cl_port << endl;
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

		session *ctx = new session;
		if(ctx == NULL) {
			/* Handle, retry? */
		}

		ctx->cs = cs;
		ctx->cmt = cmdt;
		/* Handle incoming traffic from this node */
		thread t1(incoming, ctx);
		t1.detach();
	}
}
