#include "server.h"
#include "db.h"
#include "transaction.h"

static void print_trans(trans_t t)
{
	cout <<"Transaction is:" << endl;
	cout << "Transaction id:" << t.trans_id << endl;
	cout << "Acc nr:" << t.acc_nr << endl;
	cout << "*******************************************" << endl;
}
enum failure_point {
	DONT_OK = 1,
	JUST_ABORT,
};

static void send_veto(c_sock *cs)
{
	int ret;
	two_pc_msg_t abrt;
	abrt.cmt_type = VOTE_COMMIT_REP;
	abrt.cmt_vote = ABORT_V; 
	cout << "Sending Veto to co-ordinator..." << endl;
	ret = cs->c_sock_write(&abrt, sizeof(two_pc_msg_t));
	if(ret < 0) {
		cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
	}
}

static void send_abort(c_sock *cs)
{
	int ret;
	two_pc_msg_t abrt;
	abrt.cmt_type = ABORT_M;
	cout << "Sending ABORT to the coordinator" << endl;
	ret = cs->c_sock_write(&abrt, sizeof(two_pc_msg_t));
	if(ret < 0) {
		cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
	}
}

void transaction_manager(smdt_t *smdt)
{
	/* Recieve */
	bool cont = true;
	int rcv;
	ssize_t ret;
	c_sock *cs = smdt->smdt_sock;
	cout << "Out connection is acccepted..." << endl;
	while(cont) {
		two_pc_msg_t pmsg;
		trans_t tr;

		/* First step: Always PREPARE MESSAGE, */
		ret = cs->c_sock_read((void *)&pmsg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		/* If not prepare, read again...*/
		if(pmsg.cmt_type != PREPARE) {
			if(pmsg.cmt_type == QUERY_M) {
				double amount = 0;
				tr = pmsg.trans;
				two_pc_msg_t ok;
				ret = search_record(tr.acc_nr, &amount);
				tr.status = ret;
				tr.amount = amount;
				ok.trans = tr;
				ret = cs->c_sock_write(&ok, sizeof(two_pc_msg_t));
				if(ret < 0) {
					cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
				}
			}
			continue;
		}

		cout << "Entering the prepared state of a two phase commit" << endl;
		tr = pmsg.trans;

		print_trans(tr);

		/* Introduce random failure here. FP NO 1 */
		if(smdt->smdt_fp == DONT_OK) {
			cout << "Failure point one activated. Abort." << endl;
			tr.t_state = ABORT;
			send_abort(cs);
			continue;
		}

		two_pc_msg_t ok;
		ok.cmt_type = OK_M;
		cout << "Sending OK to the coordinator" << endl;
		ret = cs->c_sock_write(&ok, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		/* Try to complete the transaction..*/
		/* If transaction cannot be completed, then veto against commit
		 * Set transaction state to abort.
		 */
		ret = transaction(tr);
		if(ret != 0) {
			tr.t_state = ABORT;
		} else {
			tr.t_state = READY;
		}
		ret = cs->c_sock_read((void *)&pmsg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		/* Someone has aborted in prepared state.. so do not move forward. */
		if(pmsg.cmt_type == ABORT_M) {
			cout << "Recieved ABORT from coordinator.. " << endl;
			tr.t_state = ABORT;
			/* This state machine is halted. Go back to init state. */
			continue;
		}

		cout << "Recieved commit vote from the coordinator" << endl;
	
		/*Insert random failure here. FP 2
		 **/
		if(tr.t_state == ABORT || smdt->smdt_fp == JUST_ABORT) {
			cout << "Failure point one activated. Abort." << endl;
			tr.t_state = ABORT;
			send_veto(cs);
			continue;
		}
		
		/* We are all good, send commit= yes */
		cout << "Sending commit vote=yes to coordinator" << endl;
		ok.cmt_type = VOTE_COMMIT_REP;
		ok.cmt_vote = COMMIT_V; 
		ret = cs->c_sock_write(&ok, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		/* Wait for final commit message */
		ret = cs->c_sock_read((void *)&pmsg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		/* Arrghhh!! Someone aborted.. :( */
		if(pmsg.cmt_type == ABORT_M) {
			cout << "Someone vetoes against commit: ABORT " << endl;
			tr.t_state = ABORT;
			/* This state machine is halted. Go back to init state. */
			continue;
		}
		cout << "COMMITING THE TRANSACTION!" << endl;
		tr.t_state = COMMIT;
		/* Send transaction to database. */
	}
	cs->c_sock_close();
	return ;
}

int open_connection(smdt_t *smdt)
{
	int rc;
	c_sock *bs = new c_sock;
	if(bs == NULL) {
		cr_log<<"Unable to open a socket:" << endl;
		return -EINVAL;	
	}

	cout << "Connecting to:" <<smdt->coord_port << "IP Address:" << smdt->coord_ip <<endl;
	rc = bs->c_sock_addr(smdt->coord_ip, smdt->coord_port);
	if(rc != 0) {
		cr_log << "Invalid Addresses" << endl;
		return -EINVAL;
	}

	cout << "Socket connection launding...." << endl;
	/* Connect to coordinator */
	rc = bs->c_sock_connect();
	if(rc != 0) {
		cout << "Unable to connect to socket!" << endl;
		return -EINVAL;
	}
	cout << "Connecting client...." << endl;
	smdt->smdt_sock = bs;
	thread t1(transaction_manager, smdt);
	t1.join();
}

int main(int argc, char *argv[])
{
	char *coordip;
	int coordport;

	if(argc < 3) {
		cout << "help: ./server <coordip> <coorport> [FP point]" << endl;
		return -EINVAL;
	}

	smdt_t *smdt = new smdt_t;
	smdt->smdt_ip = argv[1];
	if(argc == 4)
		smdt->smdt_fp = atoi(argv[3]);
	else
		smdt->smdt_fp = 0;
	smdt->coord_ip = smdt->smdt_ip;
	smdt->coord_port = atoi(argv[2]);
	open_connection(smdt);
	return 0;	
}
