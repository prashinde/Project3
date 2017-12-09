#include "server.h"


void cmdt_open_server(coomdt_t *cmdt)
{
}

int main(int argc, char *argv[])
{
	coomdt_t *cmdt;
	if(argc != 4) {
		cout << "help: ./coord <ip-address> <client-port> <server-port>" << endl;
		return -EINVAL;
	}

	cmdt = new coomdt_t;
	if(cmdt == NULL) {
		cout << "Coordinator memory error!";
		return -ENOMEM;
	}

	cmdt->cmdt_ip_addr = argv[1];
	cmdt->cmdt_port = atoi(argv[3]);
	cmdt->cmdt_cl_ip = argv[1];
	cmdt->cmdt_cl_port = atoi(argv[2]);

	cmdt_open_server(cmdt);
	cmdt_open_client(cmdt);
	delete cmdt;
	return 0;
}
