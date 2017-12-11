#include "server.h"
#include "transaction.h"
static void print_trans(trans_t t)
{
	cout <<"Transaction is:" << endl;
	cout << "Transaction id:" << t.trans_id << endl;
	cout << "Acc nr:" << t.acc_nr << endl;
	cout << "*******************************************" << endl;
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
		ret = cs->c_sock_read((void *)&pmsg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		cout << "Entering the prepared state of a two phase commit" << endl;
		tr = pmsg.trans;
		print_trans(tr);

		/* Introduce random failure here. FP NO 1 */
		two_pc_msg_t ok;
		ok.cmt_type = OK_M;
		cout << "Sending OK to the coordinator" << endl;
		ret = cs->c_sock_write(&ok, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		tr.t_state = READY;
		ret = cs->c_sock_read((void *)&pmsg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		if(pmsg.cmt_type == ABORT_M) {
			tr.t_state = ABORT;
			/* This state machine is halted. Go back to init state. */
			continue;
		}

		cout << "Recieved commit vote from the coordinator" << endl;
		/* If not abort, expect message type is commit_vote */
		if(pmsg.cmt_type != VOTE_COMMIT) {
			tr.t_state = ABORT;
			/* This state machine is halted. Go back to init state. */
			continue;
		}

		cout << "Sending commit vote=yes to coordinator" << endl;
		/*Insert random failure here. FP 2*/
		ok.cmt_type = VOTE_COMMIT_REP;
		ok.cmt_vote = COMMIT_V; 
		ret = cs->c_sock_write(&ok, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		ret = cs->c_sock_read((void *)&pmsg, sizeof(two_pc_msg_t));
		if(ret < 0) {
			cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
		}

		cout << "COMMITING THE TRANSACTION!" << endl;
		if(pmsg.cmt_type == ABORT_M) {
			tr.t_state = ABORT;
			/* This state machine is halted. Go back to init state. */
			continue;
		}
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

	if(argc != 4) {
		cout << "help: ./server <coordip> <coorport> <selfport>" << endl;
		return -EINVAL;
	}

	smdt_t *smdt = new smdt_t;
	smdt->smdt_ip = argv[1];
	smdt->smdt_port = atoi(argv[3]);
	smdt->coord_ip = smdt->smdt_ip;
	smdt->coord_port = atoi(argv[2]);
	open_connection(smdt);
	return 0;	
}
