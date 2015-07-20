/*****************************************************************************
 Name        : tcpserver.h
 Author      : sotter
 Date        : 2015年7月10日
 Description : 简单的TCPSERVER，专为测试使用
 ******************************************************************************/

#ifndef LIBNETWORK_1_0_TEST_TESTSERVER_TCPSERVER_H_
#define LIBNETWORK_1_0_TEST_TESTSERVER_TCPSERVER_H_

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
class TcpServer;

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

class TcpServer : public SockEventHandler
{
public:

	TcpServer();

	virtual ~TcpServer();

	bool init(const char *host, unsigned short port);

	void dispath();

	virtual void on_read_event(int fd);

	virtual void on_write_event(int fd);

	virtual void on_conn(int fd);

	virtual void on_read(int fd, const char *data, int len);

	virtual void on_close(int fd);

	int read(int fd);

	void write(int fd, const char *data, int len);

	void event_loop();

private:

	bool listen();

	bool set_address(const char *host, unsigned short port);

private:

	int _server_fd;

	std::set<int> _cient_fds;

	struct sockaddr_in _address;

	Select  _select;
};

} /* namespace cppnetwork */

#endif /* LIBNETWORK_1_0_TEST_TESTSERVER_TCPSERVER_H_ */
