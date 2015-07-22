/*****************************************************************************
 Name        : tcpconnection.h
 Author      : sotter
 Date        : 2015年7月21日
 Description :
 ******************************************************************************/

#ifndef LIBNETWORK_1_0_TEST_TESTSERVER_TCPCONNECTION_H_
#define LIBNETWORK_1_0_TEST_TESTSERVER_TCPCONNECTION_H_

#include <set>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>

namespace cppnetwork
{

class Select;

class TcpConnection;

class SockEventHandler
{
public:
	virtual void on_read_event(int fd) = 0;
	virtual void on_write_event(int fd) = 0;
};

class Select
{
public:
	Select();

	~Select();

	bool init();

	bool stop();

	bool add_event(int fd, bool read, bool write);

	bool remove_event(int fd, bool read, bool write);

	void event_loop(SockEventHandler *handler);

private:

	bool _stop;
	int _max_fd;

	std::set<int> _event_fds;
	fd_set _read_fds[1024];
	fd_set _write_fds[1024];
	fd_set _except_fds[1024];

	fd_set _read_fds_in[1024];
	fd_set _write_fds_in[1024];
};

enum
{
	CLOSED = 0,
	CONNECTING = 1,
	CONNECTED = 2
};

class TcpConnection : public SockEventHandler
{
public:

	TcpConnection();

	virtual ~TcpConnection();

	bool init();

	//默认3秒钟连接超时
	bool connect(const char *host, unsigned short port, int timeout = 3000);

	virtual void on_conn(int fd);

	virtual void on_read(int fd, const char *data, int len);

	virtual void on_close(int fd);

public:

	void dispath();

	virtual void on_read_event(int fd);

	virtual void on_write_event(int fd);

	int read(int fd);

	void write(int fd, const char *data, int len);

	void event_loop();

private:
	bool set_address(const char *host, unsigned short port);
private:

	int _conn_fd;

	std::set<int> _cient_fds;

	struct sockaddr_in _address;

	Select  _select;
};

} /* namespace cppnetwork */

#endif /* LIBNETWORK_1_0_TEST_TESTSERVER_TCPSERVER_H_ */
