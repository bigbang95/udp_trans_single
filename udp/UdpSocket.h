#pragma once
/*****************************************
*
*		稳定的 UDP 通讯
*
*		Author: bigbang95
*
******************************************/

#include<WinSock2.h>
#include<WS2tcpip.h>
#include<string>

#define UDP_SYN	0b1
#define UDP_ACK	0b10
#define UDP_PSH	0b100
#define UDP_POP	0b1000


#pragma pack(push)
#pragma pack(1)

struct UDP_HEAD
{
	UINT seq;
	USHORT flags;
};

#pragma pack(pop)


class CUdpSocket
{
protected:
	SOCKET m_sock;
	WSAEVENT m_event;
	sockaddr_in m_addr;
	int m_nMaxPacketSize;

public:
	CUdpSocket();
	~CUdpSocket();

	SOCKET GetSocket();
	void SetSocket(SOCKET sock);

	int startup();
	int cleanup();

	SOCKET socket(int af, int type, int protocol);

	// 发送 一次
	int sendto(SOCKET s, const char* buf, int len, const struct sockaddr* to, int tolen);

	// 返回 258、-1、0、LastError；
	int recvfrom(SOCKET s, char* buf, int* len, DWORD dwTimeout, struct sockaddr* from, int* fromlen);



	// 理论上 任意长度 发送
	virtual int send(SOCKET s, std::string buf) = 0;
	virtual int send(std::string buf) = 0;

	// 理论上 任意长度 接收
	virtual int recv(SOCKET s, std::string& buf, size_t buf_len) = 0;
	virtual int recv(std::string& buf, size_t buf_len) = 0;
};

DWORD GetRandomDword();
