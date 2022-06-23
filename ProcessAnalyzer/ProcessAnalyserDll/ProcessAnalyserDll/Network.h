#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>


class Network
{
public:
	enum Request
	{
		NEXT_DATA = 1,
		GET_RT_DEPS,
		HOOK_PREV,
		HOOK_FUNC,
		UNHOOK_FUNC
	};

	enum DataType
	{
		SUCC_FINISHED = 1,
		NUMBER,
		FUNCTION,
		LIBRARY
	};

	Network();

	int connect();
	void close();

	int sendToHost(const char* buffer);
	int recvFromHost(char* buffer);
	int sendFlag(char val);
	int recvFlag(char val);

	size_t getBufLen();

private:
	const int DEFAULT_BUFLEN = 512;
	const char* DEFAULT_PORT = "7008";

	SOCKET m_socket = INVALID_SOCKET;
	struct addrinfo* m_address = nullptr;

	bool m_inited = false;
	bool m_connected = false;
};