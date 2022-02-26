#include "UdpServerSocket.h"
#include "..\log\wLog.h"

CUdpServerSocket::CUdpServerSocket(USHORT port, LPCWSTR ip)
{
	m_sock = socket(AF_INET, SOCK_DGRAM, 0);
	if (m_sock == INVALID_SOCKET)
	{
		WriteLog(L"socket error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}

	m_event = WSACreateEvent(); // 创建 一个 初始状态 为 失信的 事件对象

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

	if (SOCKET_ERROR == bind(m_sock, (sockaddr*)&m_addr, sizeof(sockaddr)))
	{
		WriteLog(L"bind error code: " + std::to_wstring(WSAGetLastError()), __FILEW__, __LINE__);
		return;
	}


	printf("UDP server start success.\n");


	m_dwSeq = 0;
}


CUdpServerSocket::~CUdpServerSocket()
{
	if (m_sock != INVALID_SOCKET)
		closesocket(m_sock);

	if (m_event != WSA_INVALID_EVENT)
		WSACloseEvent(m_event);
}

int CUdpServerSocket::bind(SOCKET s, const struct sockaddr* name, int namelen)
{
	return ::bind(s, name, namelen);
}


///////////////////////////////////////////////////////////////////


// 返回 0、5、ret
int CUdpServerSocket::recv(SOCKET s, std::string& buf, size_t buf_len)
{
	buf = "";

	char sz1[2048];

	UDP_HEAD h1, h2;

	const DWORD dwTimeout = INFINITE;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		int len1, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz1, &len1, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{
			// 不可能事件
		}
		else if (ret != ERROR_SUCCESS)
		{
			return ret;
		}
		else
		{
			if (len1 == sizeof(h1))
			{
				h1 = *(UDP_HEAD*)sz1;

				if (h1.flags == UDP_SYN)
				{
					m_dwSeq = h1.seq;

					// 重置 while 参数
					j = 5;
				}
			}
			else if (len1 > sizeof(h1))
			{
				h1 = *(UDP_HEAD*)sz1;

				if ((h1.seq == m_dwSeq + 1)
					&& (h1.flags == UDP_PSH))
				{
					// 一次 recv 成功
					buf += std::string(sz1 + sizeof(UDP_HEAD), len1 - sizeof(UDP_HEAD));

					m_dwSeq++;

					// 重置 while 参数
					j = 5;
				}
			}
			else
			{
				continue;
			}
		}


		h2.seq = m_dwSeq + 1;
		h2.flags = UDP_ACK;

		if (sendto(s, (char*)&h2, sizeof(h2), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
		{
			return WSAGetLastError();
		}

		if (buf.length() == buf_len)
		{
			break;
		}
	}

	if (j == -1)
	{
		// 5 次 数据包 全 接收失败
		return 5;
	}

	return ERROR_SUCCESS;
}


///////////////////////////////////////////////////////////////////////////////////



// 返回 0、5、ret
int CUdpServerSocket::send(SOCKET s, std::string buf)
{
	char sz1[2048], sz2[2048];

	UDP_HEAD h1, h2;

	const DWORD dwTimeout = INFINITE;

	const int one_len = 1400 - sizeof(UDP_HEAD);

	bool b = false;

	int j = 5; // 执行 5 次 循环
	while (j--)
	{
		int len1, addr_len = sizeof(sockaddr);
		int ret = recvfrom(s, sz1, &len1, dwTimeout, (sockaddr*)&m_addr, &addr_len);
		if (ret == WSA_WAIT_TIMEOUT)
		{
			// 不可能事件
		}
		else if (ret != ERROR_SUCCESS)
		{
			return ret;
		}
		else
		{
			if (len1 == sizeof(h1))
			{
				h1 = *(UDP_HEAD*)sz1;

				if (h1.flags == UDP_SYN)
				{
					m_dwSeq = h1.seq;

					b = false;

					// 重置 while 参数
					j = 5;
				}
				else if (h1.flags == UDP_POP)
				{
					if (h1.seq == m_dwSeq + 1)
					{
						// 一次 send 成功

						if (b == false)
						{
							b = true;
						}
						else
						{
							buf = buf.substr(one_len);
						}

						m_dwSeq++;

						// 重置 while 参考
						j = 5;
					}
					else
					{

					}
				}
				else
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}

		std::string str = buf.substr(0, one_len);

		h2.seq = m_dwSeq + 1;
		h2.flags = UDP_ACK;

		memcpy(sz2, &h2, sizeof(h2));
		memcpy(sz2 + sizeof(h2), str.c_str(), str.length());

		if (h1.flags == UDP_SYN)
		{
			if (sendto(s, (char*)&h2, sizeof(h2), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
			{
				return WSAGetLastError();
			}
		}
		else if (h1.flags == UDP_POP)
		{
			if (sendto(s, sz2, sizeof(h2) + str.length(), (sockaddr*)&m_addr, sizeof(sockaddr)) == SOCKET_ERROR)
			{
				return WSAGetLastError();
			}

			if (str.length() < one_len)
			{
				break;
			}

			if (str.length() == buf.length())
			{
				break;
			}
		}
		else
		{
			WriteLog(L"UDP 标志 既不是 SYN，也不是 POP", __FILEW__, __LINE__);
			exit(0);
		}
	}

	if (j == -1)
	{
		// 5 次 数据包 全 接收失败
		return 5;
	}

	return ERROR_SUCCESS;
}


/////////////////////////////////////////////////////////////////////////////////////


int CUdpServerSocket::send(std::string buf)
{
	return send(m_sock, buf);
}

int CUdpServerSocket::recv(std::string& buf, size_t buf_len)
{
	return recv(m_sock, buf, buf_len);
}
