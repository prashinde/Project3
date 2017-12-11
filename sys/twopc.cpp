#include "server.h"
#include "transaction.h"
#include <iterator>
#include <stdlib.h>
#include <string.h>

int timeout = 10; /* 10 secs */
/*
 * Sends prepare transaction message to all participating nodes.
 * Recieves a reply from them and decides what to do.
 * If reply is Ok from all, protocol furthers, else it will halt.
 */

void query(trans_t *trans, coomdt_t *cmt)
{
	int abrt = 0;
	int ret;

	conn_mdt_t *mdt;
	list<conn_mdt_t *>::iterator it;
	two_pc_msg_t pmesg;

	pmesg.cmt_type = QUERY_M;
	memcpy(&pmesg.trans, trans, sizeof(trans_t));
	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Prepare:Server not in a group view" << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_write(&pmesg, sizeof(two_pc_msg_t));
		if(ret <= 0) {
			mdt->state = UNRESPONSIVE;
			cout << "Error in writing to the socket:" << ret << " Errno:"<< errno << endl;
		}

		ret = cs->c_sock_read(&pmesg, sizeof(two_pc_msg_t));
		if(ret <= 0) {
			mdt->state = UNRESPONSIVE;
			cout << "Error in writing to the socket:" << ret << " Errno:"<< errno << endl;
		} {
			memcpy(trans, &pmesg.trans, sizeof(trans_t));
			break;
		}
	}
}

int prepare(trans_t *trans, coomdt_t *cmt)
{
	int abrt = 0;
	int ret;

	conn_mdt_t *mdt;
	list<conn_mdt_t *>::iterator it;
	two_pc_msg_t pmesg;

	pmesg.cmt_type = PREPARE;
	memcpy(&pmesg.trans, trans, sizeof(trans_t));
	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Prepare:Server not in a group view" << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_write(&pmesg, sizeof(two_pc_msg_t));
		if(ret <= 0) {
			mdt->state = UNRESPONSIVE;
			cout << "Error in writing to the socket:" << ret << " Errno:"<< errno << endl;
		}
	}

	two_pc_msg_t rep;
	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Prepare:REP, Server not in a group view" << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_read(&rep, sizeof(two_pc_msg_t), timeout);
		if(ret < 0) {
			/* Failures int he prepare state is fine. IT means, servers are just not part of transactions.
			 **/
			mdt->state = UNRESPONSIVE;
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
			continue;
		}
		if(rep.cmt_type != OK_M)
			abrt++;
	}

	return abrt;
}

int vote_commit(trans_t *trans, coomdt_t *cmt)
{
	int abrt = 0;
	int ret;

	conn_mdt_t *mdt;
	list<conn_mdt_t *>::iterator it;
	two_pc_msg_t pmesg;

	pmesg.cmt_type = VOTE_COMMIT;
	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Server not in a group view" << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_write(&pmesg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			abrt++;
			mdt->state = UNRESPONSIVE;
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}
	}

	two_pc_msg_t rep;
	cout << "Reading Votes...." << endl;
	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Server not in a group view" << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_read(&rep, sizeof(two_pc_msg_t), timeout);
		if(ret < 0) {
			/* This is important. Server has crashed in between the protocol
			 * Consider it abort.
			 */
			abrt++;
			mdt->state = UNRESPONSIVE;
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
			continue;
		}
		if(rep.cmt_type != VOTE_COMMIT_REP || rep.cmt_vote != COMMIT_V)
			abrt++;
	}

	return abrt;
}

void commit(trans_t *trans, coomdt_t *cmt)
{
	int abrt = 0;
	int ret;

	conn_mdt_t *mdt;
	list<conn_mdt_t *>::iterator it;
	two_pc_msg_t pmesg;

	pmesg.cmt_type = COMMIT_M;
	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Server is not in a group view" << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_write(&pmesg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			/* This is fine. no need to abort. Sever will commit when it is back. */
			mdt->state = UNRESPONSIVE;
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}
	}
}

void send_abort(trans_t *trans, coomdt_t *cmt)
{
	int ret;
	conn_mdt_t *mdt;
	list<conn_mdt_t *>::iterator it;

	two_pc_msg_t abrt;
	abrt.cmt_type = ABORT_M;

	for(it = (cmt->conn_dt).begin(); it != (cmt->conn_dt).end(); it++) {
		mdt = *it;
		if(mdt->state != CONNECTED) {
			cout << "Server not in a group view.." << endl;
			continue;
		}
		c_sock *cs = mdt->sock;
		ret = cs->c_sock_write(&abrt, sizeof(two_pc_msg_t));
		if(ret < 0) {
			mdt->state = UNRESPONSIVE;
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}
	}
}


void commit_transaction(trans_t *trans, coomdt_t *cmt)
{
	int rc;
	trans->t_state = INIT;

	cout << "Entering Two Phase Commit" << endl;

	cout <<"Sending PREPARE message to backend servers" << endl;
	rc = prepare(trans, cmt);
	if(rc != 0) {
		cout << "Recieved ABORT" << endl;
		send_abort(trans, cmt);
		trans->status = -ENOENT;
		return ;
	}

	cout << "Sending VOTE to commit to backend servers" << endl;
	rc = vote_commit(trans, cmt);
	if(rc != 0) {
		cout << "Recieved ABORT" << endl;
		send_abort(trans, cmt);
		trans->status = -ENOENT;
		return ;
	}

	cout << "Sending final commit message" << endl;
	commit(trans, cmt);
	trans->status = 0;
}
