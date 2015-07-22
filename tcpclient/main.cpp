/*****************************************************************************
Name        : main.cpp
Author      : sotter
Date        : 2015年7月13日
Description : 
******************************************************************************/
#include "tcpconnection.h"

int main()
{
	cppnetwork::TcpConnection tcpserver;
	tcpserver.init();
	tcpserver.connect("127.0.0.1", 7001);

	sleep(50);
	return 0;
}


