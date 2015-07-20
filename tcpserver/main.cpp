/*****************************************************************************
Name        : main.cpp
Author      : sotter
Date        : 2015年7月13日
Description : 
******************************************************************************/
#include "tcpserver.h"

int main()
{
	cppnetwork::TcpServer tcpserver;
	tcpserver.init("127.0.0.1", 7001);
	tcpserver.dispath();

	sleep(500);

	return 0;
}


