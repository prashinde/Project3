#include "server.h"
#include <vector>
#include <sstream>
#include <algorithm>
#include <iterator>

static string SCREATE("create");
static string SUPDATE("update");
static string SQUERY("query");
static string SQUIT("quit");

#define SAFE_QUIT 0xff

void send_async(rq_msg_t msg, c_sock *bs)
{
	rep_msg_t *rep;
	ssize_t ret = bs->c_sock_write((void *)&msg, sizeof(rq_msg_t));
	if(ret < 0) {
		cout << "Error in writing into the socket:" << ret << " Errno:"<< errno << endl;
	}
}

rep_msg_t *send_sync(rq_msg_t msg, c_sock *bs)
{
	rep_msg_t *rep;
	ssize_t ret = bs->c_sock_write((void *)&msg, sizeof(rq_msg_t));
	if(ret < 0) {
		cout << "Error in writing into the socket:" << ret << " Errno:"<< errno << endl;
	}

	rep = new rep_msg_t;
	ret = bs->c_sock_read((void *)rep, sizeof(rep_msg_t));
	if(ret < 0) {
		cout << "Error in reading from the socket:" << ret << " Errno:"<< errno << endl;
	}
	return rep;
}

rep_cr_msg send_create_msg(unsigned long amount, c_sock *bs)
{
	rep_msg_t *rep;
	rq_msg_t rmsg;
	rep_cr_msg rep_msg;

	rmsg.u.cr_msg.amount = amount;
	rmsg.type = CREATE_REQ;
	rep = (rep_msg_t*)send_sync(rmsg, bs);
	rep_msg = rep->u.cr_msg;
	return rep_msg;
}

rep_update_msg send_update_msg(unsigned long acc_nr, unsigned long amount, c_sock *bs)
{
	rep_msg_t *rep;
	rq_msg_t   rmsg;
	rep_update_msg rep_msg;
	
	rmsg.u.up_msg.acc_nr = acc_nr;
	rmsg.u.up_msg.amount = amount;
	rmsg.type = UPDATE_REQ;
	rep = (rep_msg_t*)send_sync(rmsg, bs);
	rep_msg = rep->u.up_msg;
	return rep_msg;
}

rep_query_msg send_query_msg(unsigned long acc_nr, c_sock *bs)
{
	rep_msg_t *rep;
	rq_msg_t rmsg;
	rep_query_msg rep_msg;

	rmsg.u.q_msg.acc_nr = acc_nr;
	rmsg.type = QUERY_REQ;
	rep = (rep_msg_t*)send_sync(rmsg, bs);
	rep_msg = rep->u.q_msg;
	return rep_msg;
}

void send_quit_msg(c_sock *bs)
{
	rq_msg_t rmsg;
	rmsg.type = QUIT;

	send_async(rmsg, bs);
	return ;
}

int create_record(vector<string> cmd, c_sock *bs)
{
	rep_cr_msg rcm;
	unsigned long amount = strtoul(cmd[1].c_str(), NULL, 10);
	rcm = send_create_msg(amount, bs);
	if(rcm.err_code == 0)
		cout << "OK " << rcm.acc_nr << endl;
	else
		cout << "ERROR" << endl;
	return 0;	
}

int update_record(vector<string> cmd, c_sock *bs)
{
	rep_update_msg rum;
	unsigned long acc_nr = strtoul(cmd[1].c_str(), NULL, 10);
	unsigned long amount = strtoul(cmd[2].c_str(), NULL, 10);
	rum = send_update_msg(acc_nr, amount, bs);
	if(rum.err_code == 0)
		cout << "OK " << rum.amount << endl;
	else
		cout << "ERROR" << endl;
	return 0;	
}

int query_record(vector<string> cmd, c_sock *bs)
{
	rep_query_msg rum;
	unsigned long acc_nr = strtoul(cmd[1].c_str(), NULL, 10);
	rum = send_query_msg(acc_nr, bs);
	if(rum.err_code == 0)
		cout << "OK " << rum.amount << endl;
	else
		cout << "ERROR " << rum.err_code << endl;
	return 0;	
}

static int process_cmd(vector<string> cmd, c_sock *bs)
{
	
	if(cmd[0] == SCREATE) {
		if(cmd.size() != 2) {
			cout << "Invalid create command" << endl;
			return -EINVAL;
		}
		create_record(cmd, bs);
		return 0;
	} else if(cmd[0] == SUPDATE) {
		if(cmd.size() != 3) {
			cout << "Invalid create command" << endl;
			return -EINVAL;
		}
		update_record(cmd, bs);
		return 0;
	} else if(cmd[0] == SQUERY) {
		if(cmd.size() != 2) {
			cout << "Invalid create command" << endl;
			return -EINVAL;
		}
		query_record(cmd, bs);
		return 0;
	} else if(cmd[0] == SQUIT) {
		send_quit_msg(bs);
		return SAFE_QUIT;
	}

	cout << "INVALID COMMAND" << endl;
	return -EAGAIN;
}

static void lower_case(string &str)
{
	transform(str.begin(), str.end(), str.begin(), ::tolower);
}

int main(int argc, char *argv[])
{
	int rc;
	coomdt_t *cmdt;

	if(argc != 3) {
		cout << "help: ./coord <coord-ip-address> <coord-port>" << endl;
		return -EINVAL;
	}

	cout << "Establish session with Coordinator.." << endl;
	c_sock *bs = new c_sock;
	if(bs == NULL) {
		cr_log<<"Unable to open a socket:" << endl;
		return -EINVAL;	
	}

	rc = bs->c_sock_addr(argv[1], atoi(argv[2]));
	if(rc != 0) {
		cr_log << "Invalid Addresses" << endl;
		return -EINVAL;
	}

	rc = bs->c_sock_connect();
	if(rc != 0) {
		cout << "Unable to connect to socket!" << endl;
		//return -EINVAL;
	}

	cout << "Loggin into the client shell.." << endl;
	while(1) {
		string command;
		cout << "> ";
		getline(cin, command);
		if(command.length() < 4) {
			cout << command << endl;
			continue;
		}
		lower_case(command);
		istringstream iss(command);
		vector<string> tokens{istream_iterator<string>{iss},
							  istream_iterator<string>{}};
		rc = process_cmd(tokens, bs);
		if(rc == SAFE_QUIT)
			break;
	}
	return 0;
}
