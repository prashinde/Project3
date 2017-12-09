#include "sock.h"

int c_sock :: c_sock_addr(string ip, int port)
{
	int rc;
	const char *ip_ca = ip.c_str();
	this->sock = socket(AF_INET, SOCK_STREAM, 0);
	if(this->sock == -1) {
		cr_log << "Unable to create a socket.:" << errno << endl;
		return errno;
	}

	this->server_addr.sin_family = AF_INET;
	
	rc = inet_aton(ip_ca, &(this->server_addr.sin_addr));
	if(rc == 0) {
		cr_log << "Server ip address invalid: " << ip << endl;
		return -EINVAL;
	}

	int optval = 1;
	setsockopt(this->sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

	this->server_addr.sin_port = htons(port);
	return 0;
}

int c_sock::c_sock_connect()
{
	int rc;

	rc = connect(this->sock, (struct sockaddr *)&(this->server_addr), sizeof(this->server_addr));
	if (rc < 0) {
		cr_log << "Server not connected." << endl;
		return rc;
	}

	return 0;
}

int c_sock :: c_sock_bind()
{
	int rc;

	rc = bind(this->sock, (struct sockaddr *)&(this->server_addr), sizeof(this->server_addr));
	if(rc < 0) {
		cr_log << "Bind failed.:" << errno << endl;
		return rc;
	}
	return 0;
}

int c_sock :: c_sock_listen()
{
	listen(this->sock, 128);
}

c_sock *c_sock :: c_sock_accept()
{
	c_sock *cs = new c_sock;
	if(cs == NULL) {
		cr_log << "Client socket failed.." << "\n";
		return NULL;
	}

	cs->addrlen = sizeof(this->cli_addr);
	this->addrlen = sizeof(this->cli_addr);
	cs->sock = accept(this->sock, (struct sockaddr *)&cs->cli_addr, &cs->addrlen);
	if(cs->sock < 0) {
		cr_log << "Unable to accept the connection:" << errno << endl;
		delete cs;
		return NULL;
	}

	return cs;
}

ssize_t c_sock::c_sock_read(void *buffer, size_t len)
{
	ssize_t rcv = 0;
	ssize_t rc = 0;
	while(len > 0) {
		rcv = recv(this->sock, buffer, len, 0);
		buffer = ((char*)buffer)+rcv;
		len = len - rcv;
		rc += rcv;
	}

	return rc;
}

ssize_t c_sock::c_sock_write(void *buffer, size_t len)
{
	return send(this->sock, buffer, len, 0);
}

void c_sock::c_sock_close()
{
	close(this->sock);
}
