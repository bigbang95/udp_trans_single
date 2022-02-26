#include "UdpSocket.h"
#include "..\log\wLog.h"

CUdpSocket::CUdpSocket()
{
	m_sock = INVALID_SOCKET;
	m_event = WSA_INVALID_EVENT;
	m_nMaxPacketSize = 2048;

	srand(GetTickCount());

	startup();
}

CUdpSocket::~CUdpSocket()
{
	cleanup();
}

SOCKET CUdpSocket::GetSocket()
{
	return m_sock;
}

void CUdpSocket::SetSocket(SOCKET sock)
{
	m_sock = sock;
}

int CUdpSocket::startup()
{
	WSADATA wsadata;

	int err;

	if (err = WSAStartup(MAKEWORD(2, 2), &wsadata))
	{
		return err;
	}

	if (wsadata.wVersion != MAKEWORD(2, 2))
	{
		return -1000;
	}

	return ERROR_SUCCESS;
}

int CUdpSocket::cleanup()
{
	return WSACleanup();
}

SOCKET CUdpSocket::socket(int af, int type, int protocol)
{
	return ::socket(af, type, protocol);
}

int CUdpSocket::sendto(SOCKET s, const char* buf, int len, const struct sockaddr* to, int tolen)
{
	return ::sendto(s, buf, len, 0, to, tolen);
}

// 返回 258、-1、0、LastError；
int CUdpSocket::recvfrom(SOCKET s, char* buf, int* len, DWORD dwTimeout, struct sockaddr* from, int* fromlen)
{
	const sockaddr_in null_addr = {};

	while (1)
	{
		// 监视事件对象状态
		DWORD dwRet = WSAWaitForMultipleEvents(1, &m_event, false, dwTimeout, false);
		if (dwRet == WSA_WAIT_TIMEOUT)
		{
			return WSA_WAIT_TIMEOUT;
		}
		else if (dwRet == WSA_WAIT_FAILED)
		{
			return WSA_WAIT_FAILED;
		}
		else
		{
			// 成功 获取 recv 信号
			WSANETWORKEVENTS netEvt;
			WSAEnumNetworkEvents(m_sock, m_event, &netEvt); // 为 m_sock 查看已发生的网络事件
			if (netEvt.lNetworkEvents & FD_READ)
			{
				// 事件 含 FD_READ

				sockaddr temp;
				int temp_len = sizeof(sockaddr);

				// 接收 UDP 报文
				*len = ::recvfrom(s, buf, m_nMaxPacketSize, 0, &temp, &temp_len);
				if (*len == SOCKET_ERROR)
				{
					return WSAGetLastError();
				}
				else if (*len == 0)
				{
					int r = WSAGetLastError();
					if (r != ERROR_SUCCESS)
					{
						// 发现错误
						WriteLog(L"recvfrom 返回 0，错误代码：" + std::to_wstring(r), __FILEW__, __LINE__);
						return r;
					}
					else
					{
						// 再次接收
						// 条件：对端 空报文 不是 有效 报文，即 对端 禁止发送 空报文
						wchar_t ws[20];
						InetNtopW(AF_INET, &(((sockaddr_in*)&temp)->sin_addr), ws, 20);
						WriteLog(L"recvfrom 接收 IP 地址为：" + std::wstring(ws), __FILEW__, __LINE__);
						continue;
					}
				}
				else
				{
					if (null_addr.sin_addr.S_un.S_addr == (*(sockaddr_in*)from).sin_addr.S_un.S_addr)
					{
						*from = temp;
						return ERROR_SUCCESS;
					}

					if ((*(sockaddr_in*)from).sin_addr.S_un.S_addr == (*(sockaddr_in*)&temp).sin_addr.S_un.S_addr)
					{
						return ERROR_SUCCESS;
					}

					continue;
				}
			}
			else
			{
				WriteLog(L"网络事件不含 recv 事件", __FILEW__, __LINE__);
				exit(-1);
			}
		}
	}

	return -1000; // 不可能事件
}

DWORD GetRandomDword()
{
	//srand(GetTickCount());

	union
	{
		UINT nd;

		struct
		{
			USHORT ns1;
			USHORT ns2;
		} ns;

	} random_num;

	random_num.ns.ns1 = rand() + rand();
	random_num.ns.ns2 = rand() + rand();

	return random_num.nd;
}
