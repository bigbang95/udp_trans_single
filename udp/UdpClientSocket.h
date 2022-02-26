#pragma once
#include "UdpSocket.h"

class CUdpClientSocket :public CUdpSocket
{
private:
	DWORD m_dwSeq; // 当前 发送/接收 序号

public:
	CUdpClientSocket(USHORT port, LPCWSTR ip);
	~CUdpClientSocket();

	// 发送 一个 UDP 数据报文
	int send1(SOCKET s, const char* buf, int len, bool bSyn = false);

	// 接收 一个 UDP 数据报文
	int recv1(SOCKET s, char* buf, int* len, bool bSyn = false);

	// 理论上 任意长度 发送
	// 返回 成功(0)、失败
	int send(SOCKET s, std::string buf);
	int send(std::string buf);

	// 理论上 任意长度 接收
	// 返回 成功(0)、失败
	int recv(SOCKET s, std::string& buf, size_t buf_len);
	int recv(std::string& buf, size_t buf_len);
};
