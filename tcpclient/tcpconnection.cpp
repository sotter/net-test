/*****************************************************************************
 Name        : tcpserver.cpp
 Author      : sotter
 Date        : 2015年7月10日
 Description : 
 ******************************************************************************/

#include <sys/time.h>
#include <time.h>
#include <string>
#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "tcpconnection.h"

using namespace std;

namespace cppnetwork
{

static std::string addr2string(struct sockaddr_in &addr);
static std::string getsockaddr(int fd);
static std::string getpeeraddr(int fd);
static int setnonblocking(int fd);

static std::string addr2string(struct sockaddr_in &addr)
{
	char dest[32];
	unsigned long ad = ntohl(addr.sin_addr.s_addr);
	sprintf(dest, "%d.%d.%d.%d:%d", static_cast<int>((ad >> 24) & 255),
	        static_cast<int>((ad >> 16) & 255), static_cast<int>((ad >> 8) & 255),
	        static_cast<int>(ad & 255), ntohs(addr.sin_port));
	return dest;
}

//static std::string getsockaddr(int fd)
//{
//	struct sockaddr_in address;
//
//	memset(&address, 0, sizeof(address));
//	unsigned int len = sizeof(address);
//	if (getsockname(fd, (struct sockaddr*) &address, &len) != 0)
//	{
//		printf("getsockname error %s \n", strerror(errno));
//		return "";
//	}
//
//	return addr2string(address);
//}

static std::string getpeeraddr(int fd)
{
	struct sockaddr_in address;
	memset(&address, 0, sizeof(address));
	unsigned int len = sizeof(address);

	if (getpeername(fd, (struct sockaddr*) &address, &len) != 0)
	{
		printf("getsockname error %s \n", strerror(errno));
		return "";
	}

	return addr2string(address);
}

static int setnonblocking(int fd)
{
	//注意返回就的文件描述符属性以便将来恢复文件描述符属性
	int old_option = fcntl(fd, F_GETFL);
	int new_option = old_option | O_NONBLOCK;
	fcntl(fd, F_SETFL, new_option);
	return old_option;
}

TcpConnection::TcpConnection()
{
	_conn_fd = -1;
}

TcpConnection::~TcpConnection()
{
	_select.stop();
	if(_conn_fd > 0)
	{
		close(_conn_fd);
		_conn_fd = -1;
	}
}

bool TcpConnection::init()
{
	dispath();
	return true;
}

bool TcpConnection::connect(const char *host, unsigned short port, int timeout)
{
	if(! set_address(host, port))
	{
		return false;
	}

	if(_conn_fd > 0)
	{
		::close(_conn_fd);
		_conn_fd = -1;
	}

	_conn_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(_conn_fd < 0)
	{
		return false;
	}

	setnonblocking(_conn_fd);
	int ret = ::connect(_conn_fd, (struct sockaddr*) &_address, sizeof(_address)); //connect连接服务端
	if (ret == 0)
	{
		printf("connect %s %d success\n", host, port);
		return true;
	}
	else if (errno != EINPROGRESS)
	{
		printf("connect fail errno:%s\n", strerror(errno));
		return false;
	}

	fd_set writefds;
    FD_ZERO(&writefds);
	FD_SET(_conn_fd, &writefds);

	struct timeval tmout;
	tmout.tv_sec = timeout / 1000;
	tmout.tv_usec = (timeout % 1000) * 1000;
	ret = select(_conn_fd + 1, NULL, &writefds, NULL, &tmout); //select监听sockfd上在超时时间timeout内是否可写
	if (ret <= 0)
	{
		printf("select ret:%d\n", ret);
		//若可写事件没有发生则连接超时，返回-1
		return false;
	}

	//开始检测connect返回结果
	int error = 0;
	socklen_t length = sizeof(error);
	if (getsockopt(_conn_fd, SOL_SOCKET, SO_ERROR, &error, &length) < 0)
	{
		printf("getsockopt fail:%s\n", strerror(errno));
		return false;
	}
	else if(error > 0)
	{
		printf("connect after select fail:%s\n", strerror(error));
		return false;
	}
	else if (error == 0)
	{
		printf("connect %s %d, success\n", host, port);
		_select.add_event(_conn_fd, true, false);
		return true;
	}
	else
	{
		printf("###### 不能跑到这！！！！########\n");
		return false;
	}
}

void *thread_fun(void *arg)
{
	TcpConnection *server = (TcpConnection*)arg;
	server->event_loop();

	return NULL;
}

void TcpConnection::dispath()
{
	pthread_t pid;
	pthread_attr_t thread_attr;
	pthread_attr_init(&thread_attr);
	pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	pthread_create(&pid, NULL, thread_fun, (void*)this);
}

void TcpConnection::event_loop()
{
	_select.event_loop(this);
}

bool TcpConnection::set_address(const char *host, unsigned short port)
{
	memset(static_cast<void *>(&_address), 0, sizeof(_address));

	_address.sin_family = AF_INET;
	_address.sin_port = htons(static_cast<short>(port));

	bool rc = true;
	if (host == NULL || host[0] == '\0')
	{
		_address.sin_addr.s_addr = htonl(INADDR_ANY);
	}
	else
	{
		char c;
		const char *p = host;
		bool is_ipaddr = true;
		while ((c = (*p++)) != '\0')
		{
			if ((c != '.') && (!((c >= '0') && (c <= '9'))))
			{
				is_ipaddr = false;
				break;
			}
		}

		if (is_ipaddr)
		{
			_address.sin_addr.s_addr = inet_addr(host);
		}
		else
		{
			struct timeval begin, end;
			gettimeofday(&begin, NULL);
			struct hostent *hostname = gethostbyname(host);
			gettimeofday(&end, NULL);

			if (hostname != NULL)
			{
				memcpy(&(_address.sin_addr), *(hostname->h_addr_list), sizeof(struct in_addr));
				printf("GET DNS OK, %s => %s", host, inet_ntoa(_address.sin_addr));
			}
			else
			{
				rc = false;
			}
		}
	}

	return rc;
}

void TcpConnection::on_read_event(int fd)
{
	if(fd == _conn_fd)
	{
		struct sockaddr_in client_addr;
		int len = sizeof(client_addr);
		int fd = ::accept(_conn_fd, (struct sockaddr*)&client_addr, (socklen_t*)&len);
		if(fd > 0)
		{
			printf("on connect from %s fd:%d \n", addr2string(client_addr).c_str(), fd);
			setnonblocking(fd);
			on_conn(fd);
			_select.add_event(fd, true, false);
		}
		else
		{
			printf("accept fail :%s \n", strerror(errno));
		}
	}
	else
	{
		read(fd);
	}
}

void TcpConnection::on_write_event(int fd)
{
	return;
}

void TcpConnection::on_conn(int fd)
{
	return;
}

//SIMPLE TCP SERVER 没有分包机制
void TcpConnection::on_read(int fd, const char *data, int len)
{
	printf("on read from fd:%d %s len:%d %*.s \n", fd, getpeeraddr(fd).c_str(), len, len, data);
}

void TcpConnection::on_close(int fd)
{
	_select.remove_event(fd, true, true);
	close(fd);
	printf("close fd:%d\n", fd);
}

int TcpConnection::read(int fd)
{
	int offset = 0;
	char buffer[1500];
	int buffer_len = 1500;

	while (offset < buffer_len - 1)
	{
		int recv_bytes = (int)::recv(fd, (void*) buffer, buffer_len - offset, 0);

		if (recv_bytes > 0)
		{
			offset += recv_bytes;
			continue;
		}
		else if (recv_bytes == 0)
		{
			on_close(fd);
			break;
		}
		else
		{
			if (errno == EAGAIN)
			{
				printf("recv from fd:%d finished recv:%d \n", fd, offset);
			}
			else
			{
				printf("recv error:%s \n", strerror(errno));
				on_close(fd);
			}
			break;
		}
	}

	if(offset > 0)
	{
		on_read(fd, buffer, offset);
	}

	return offset;
}

void TcpConnection::write(int fd, const char *data, int len)
{
	if (data == NULL || len < 0)
	{
		return ;
	}

	fd_set write_fds;
	int offset = 0;
	struct timeval tval;

	tval.tv_sec = 1;
	tval.tv_usec = 0;

	while (offset < len)
	{
		len = (int) ::write(fd, data + offset, len - offset);
		if (len > 0)
		{
			offset += len;
			continue;
		}
		else
		{
			if (errno == EAGAIN)
			{
				FD_ZERO(&write_fds);
				FD_SET(fd, &write_fds);
				int res = select(fd + 1, NULL, &write_fds, NULL, &tval);
				if (res > 0)
				{
					continue;
				}
				else if (res < 0)
				{
					printf("wirite select error-%d-%s \n", errno, strerror(errno));
				}
				else
				{
					printf("write select timeout");
					break;
				}
			}
			else
			{
				printf("fd:%d write fail:%s\n", fd, strerror(errno));
				break;
			}
		}
	}

	if (offset == len)
	{
		printf("send fd:%d len:%d finish \n", fd, offset);
	}

	return;
}

Select::Select()
{
	_stop = false;
	_max_fd = 1;
	memset(_read_fds, 0, sizeof(_read_fds));
	memset(_write_fds, 0, sizeof(_write_fds));
	memset(_except_fds, 0, sizeof(_except_fds));
}

Select::~Select()
{
	_stop = true;
}

bool Select::init()
{
	return true;
}

bool Select::stop()
{
	_stop = true;
	return true;
}

bool Select::add_event(int fd, bool read, bool write)
{
	if (fd > 1024)
		return false;

	FD_SET(fd, _except_fds);

	if (read)
	{
		FD_SET(fd, _read_fds);
	}
	if (write)
	{
		FD_SET(fd, _write_fds);
	}

	if(fd > _max_fd)
		_max_fd = fd;


	_event_fds.insert(fd);

	return true;
}

bool Select::remove_event(int fd, bool read, bool write)
{
	if(fd > 1024)
	{
		return false;
	}

	if(_max_fd == fd)
	{
		//重新选取最大fd值
		_event_fds.erase(fd);
		_max_fd = 1;
		std::set<int>::iterator iter = _event_fds.begin();
		for(; iter != _event_fds.end(); ++iter)
		{
			_max_fd =  (*iter > _max_fd) ? *iter : _max_fd;
		}
	}
	else
	{
		_event_fds.erase(fd);
	}

	FD_CLR(fd, _except_fds);

	if(read)
		FD_CLR(fd, _read_fds);
	if(write)
		FD_CLR(fd, _write_fds);

	return true;
}

void Select::event_loop(SockEventHandler *handler)
{
	struct timeval timeout;
	while (!_stop)
	{
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		//每次重置一下
		memcpy(_read_fds_in, _read_fds, sizeof(_read_fds));
		memcpy(_write_fds_in, _write_fds, sizeof(_write_fds));

		int res = select(_max_fd + 1, _read_fds_in, _write_fds_in, NULL, &timeout);
		if (res == -1)
		{
			printf("select error :%s \n", strerror(errno));
		}

		if (res > 0)
		{
			for (int fd = 0; fd <= _max_fd; fd++)
			{
				if (FD_ISSET(fd, _read_fds_in))
				{
//					printf("readevent fd:%d \n", fd);
					handler->on_read_event(fd);
				}
				if (FD_ISSET(fd, _write_fds_in))
				{
//					printf("writeevent fd:%d \n", fd);
					handler->on_write_event(fd);
				}
			}
		}
	}

	return;
}

} /* namespace cppnetwork */
