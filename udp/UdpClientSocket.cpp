#include "UdpClientSocket.h"
#include "..\log\wLog.h"

CUdpClientSocket::CUdpClientSocket(USHORT port, LPCWSTR ip)
{
	m_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sock == INVALID_SOCKET)
	{
		WriteLog(L"socket error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	m_event = WSACreateEvent(); // 创建一个 初始状态 为 失信的 事件对象

	if (m_event == WSA_INVALID_EVENT)
	{
		WriteLog(L"WSACreateEvent error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	if (SOCKET_ERROR == WSAEventSelect(m_sock, m_event, FD_READ))
	{
		WriteLog(L"WSAEventSelect error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	in_addr in_addr1;
	int ret = InetPtonW(AF_INET, ip, &in_addr1);
	if (ret != 1)
	{
		// 失败
		WriteLog(L"ip 地址无效", __FILEW__, __LINE__);
		return;
	}

	m_addr.sin_addr = in_addr1;
	m_addr.sin_family = AF_INET;
	m_addr.sin_port = htons(port);


	printf("UDP client start success.\n");


	m_dwSeq = 0;
}


CUdpClientSocket::~CUdpClientSocket()
{
	if (m_sock != INVALID_SOCKET)
		closesocket(m_sock);

	if (m_event != WSA_INVALID_EVENT)
		WSACloseEvent(m_event);
}


//////////////////////////////////////////////////////////////


// 返回 0、5、ret
int CUdpClientSocket::send1(SOCKET s, const char* buf, int len, bool bSyn)
{
	char sz1[2048], sz2[2048];
	UDP_HEAD h1, h2;
	h1.seq = m_dwSeq;

	if (bSyn)
	{
		h1.flags = UDP_SYN;
	}
	else
	{
		h1.flags = UDP_PSH;
	}

	memcpy(sz1, &h1, sizeof(h1));
	memcpy(sz1 + sizeof(h1), buf, len);

	DWORD dwTimeout = 1000;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		if (sendto(s, sz1, len + sizeof(h1), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			return WSAGetLastError();
		}

	RecvAgain:

		// 要么 接收到正确的返回包，要么 超时
		int len2, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz2, &len2, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{

		}
		else if (ret == ERROR_SUCCESS)
		{
			if (len2 == sizeof(h2))
			{
				h2 = *(UDP_HEAD*)sz2;

				if ((h2.seq == m_dwSeq + 1)
					&& (h2.flags == UDP_ACK))
				{
					m_dwSeq++;
					break;
				}
			}

			WriteLog(L"\t\t\t接收到错误返回包一次", __FILEW__, __LINE__);

			goto RecvAgain; // 不断的 接收 正确的返回包 直到超时
		}
		else
		{
			return ret;
		}

		dwTimeout *= 2;

		WriteLog(L"丢包 " + std::to_wstring(5 - j) + L" 次", __FILEW__, __LINE__);
	}

	if (j == -1)
	{
		// 5 次 数据包 全 发送失败
		return 5;
	}

	return ERROR_SUCCESS;
}

int CUdpClientSocket::send(SOCKET s, std::string buf)
{
	m_dwSeq = GetRandomDword();

	int ret;
	if ((ret = send1(s, "", 0, true)) != ERROR_SUCCESS)
	{
		return ret;
	}

	const int one_len = 1400 - sizeof(UDP_HEAD);

	while (buf.length() != 0)
	{
		std::string str = buf.substr(0, one_len);
		int ret;
		if ((ret = send1(s, str.c_str(), str.length())) != ERROR_SUCCESS)
		{
			return ret;
		}

		if (str.length() < one_len)
		{
			break;
		}

		buf = buf.substr(one_len);
	}

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////////////


// 0、5、ret
int CUdpClientSocket::recv1(SOCKET s, char* buf, int* len, bool bSyn /*= false*/)
{
	char sz1[2048], sz2[2048];
	UDP_HEAD h1, h2;
	h1.seq = m_dwSeq;

	if (bSyn)
	{
		h1.flags = UDP_SYN;
	}
	else
	{
		h1.flags = UDP_POP;
	}

	memcpy(sz1, &h1, sizeof(h1));

	DWORD dwTimeout = 1000;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		if (sendto(s, sz1, sizeof(h1), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			return WSAGetLastError();
		}

	RecvAgain:

		// 要么 接收到正确的返回包，要么 超时
		int len2, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz2, &len2, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{

		}
		else if (ret == ERROR_SUCCESS)
		{
			if (len2 == sizeof(h2))
			{
				h2 = *(UDP_HEAD*)sz2;

				if ((h2.seq == m_dwSeq + 1)
					&& (h2.flags == UDP_ACK))
				{
					m_dwSeq++;
					*len = 0;
					break;
				}
			}
			else if (len2 > sizeof(h2))
			{
				h2 = *(UDP_HEAD*)sz2;

				if ((h2.seq == m_dwSeq + 1)
					&& (h2.flags == UDP_ACK))
				{
					m_dwSeq++;
					*len = len2 - sizeof(h2);
					memcpy(buf, sz2 + sizeof(h2), *len);
					break;
				}
			}

			WriteLog(L"\t\t\t接收到错误返回包一次", __FILEW__, __LINE__);

			goto RecvAgain; // 不断的 接收 正确的返回包 直到超时
		}
		else
		{
			return ret;
		}

		dwTimeout *= 2;

		WriteLog(L"丢包 " + std::to_wstring(5 - j) + L" 次", __FILEW__, __LINE__);
	}

	if (j == -1)
	{
		// 5 次 数据包 全 发送失败
		return 5;
	}

	return ERROR_SUCCESS;
}

// 0、5、ret
int CUdpClientSocket::recv(SOCKET s, std::string& buf, size_t buf_len)
{
	buf = "";

	m_dwSeq = GetRandomDword();
	char sz[2048] = {};
	int len = -1;

	int ret;
	if ((ret = recv1(s, sz, &len, true)) != ERROR_SUCCESS)
	{
		return ret;
	}

	if (len != 0)
	{
		WriteLog(L"SYN 接收失败，有效数据长度 不等于 0", __FILEW__, __LINE__);
		exit(0);
	}

	while (buf.length() < buf_len)
	{
		int ret;
		if ((ret = recv1(s, sz, &len)) != ERROR_SUCCESS)
		{
			return ret;
		}

		buf += std::string(sz, len);
	}

	return ERROR_SUCCESS;
}


////////////////////////////////////////////////////////////////////////


int CUdpClientSocket::send(std::string buf)
{
	return send(m_sock, buf);
}

int CUdpClientSocket::recv(std::string& buf, size_t buf_len)
{
	return recv(m_sock, buf, buf_len);
}
