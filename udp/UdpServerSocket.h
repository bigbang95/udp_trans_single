#pragma once
#include "UdpSocket.h"

class CUdpServerSocket :public CUdpSocket
{
private:
	DWORD m_dwSeq; // 当前 发送/接收 序号

public:
	CUdpServerSocket(USHORT port, LPCWSTR ip = L"0.0.0.0");
	~CUdpServerSocket();

	int bind(SOCKET s, const struct sockaddr* name, int namelen);


	// 理论上 任意长度 发送
	// 返回 成功(0)、失败
	int send(SOCKET s, std::string buf);
	int send(std::string buf);

	// 理论上 任意长度 接收
	// 返回 成功(0)、失败
	int recv(SOCKET s, std::string& buf, size_t buf_len);
	int recv(std::string& buf, size_t buf_len);
};
