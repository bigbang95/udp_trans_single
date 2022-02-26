/****************************************************
*
*		稳定 UDP 通讯
*
*		服务端
*
*		上传/下载 文件
*
*		Release x64 VS2019
*
*****************************************************/


#include "..\udp\UdpServerSocket.h"
#pragma comment(lib,"ws2_32.lib")


int main()
{
	//CUdpServerSocket udp(4444, L"127.0.0.1");
	CUdpServerSocket udp(4444);

	int ret;

	std::string buf;

	ret = udp.recv(buf, 4);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "recv len error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	int buf_len = *(int*)buf.c_str();

	ret = udp.recv(buf, buf_len);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "recv data error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	FILE* pf;
	if (fopen_s(&pf, "11.exe", "wb") != 0)
	{
		char sz[1024];
		strerror_s(sz, errno);
		MessageBoxA(0, sz, 0, 0);
		return -1;
	}

	fwrite(buf.c_str(), 1, buf.length(), pf);
	fclose(pf);

	printf("server 接收完成\n");



	int start = GetTickCount();

	int nLen = buf.length();

	ret = udp.send(std::string((char*)&nLen, 4));
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "send len error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	ret = udp.send(buf);
	if (ret != ERROR_SUCCESS)
	{
		MessageBoxA(0, "send data error code", std::to_string(ret).c_str(), 0);
		return -1;
	}

	printf("server send 用时：%d 毫秒\n", GetTickCount() - start);

	printf("server 发送完成\n");

	system("pause");
	return 0;
}
