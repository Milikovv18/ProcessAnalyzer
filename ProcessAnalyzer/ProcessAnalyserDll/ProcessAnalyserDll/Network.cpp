#include "Network.h"


Network::Network()
{
    WSADATA wsaData;

    // Initialize Winsock
    int iResult = ::WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        OutputDebugString(L"WSAStartup failed\n");
        return;
    }

    struct addrinfo hints;
    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;

    // Resolve the server address and port
    iResult = ::getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &m_address);
    if (iResult != 0) {
        OutputDebugString(L"getaddrinfo failed");
        ::WSACleanup();
        return;
    }

    m_inited = true;
}


int Network::connect()
{
    if (!m_inited) {
        return -1;
    }

    // Attempt to connect to an address until one succeeds
    for (struct addrinfo* ptr = m_address; ptr != NULL; ptr = ptr->ai_next) {

        // Create a SOCKET for connecting to server
        m_socket = ::socket(ptr->ai_family, ptr->ai_socktype,
            ptr->ai_protocol);
        if (m_socket == INVALID_SOCKET) {
            OutputDebugString(L"socket failed");
            return 1;
        }

        // Connect to server
        int iResult = ::connect(m_socket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            ::closesocket(m_socket);
            m_socket = INVALID_SOCKET;
            continue;
        }
        break;
    }

    if (m_socket == INVALID_SOCKET) {
        OutputDebugString(L"Unable to connect to server!\n");
        return 1;
    }

    m_connected = true;

    return 0;
}


int Network::sendToHost(const char* buffer)
{
    if (!m_connected) {
        return -1;
    }

    // Send an initial buffer
    int iResult = ::send(m_socket, buffer, (int)strlen(buffer), 0);
    if (iResult == SOCKET_ERROR) {
        OutputDebugString(L"Send failed\n");
        ::closesocket(m_socket);
        ::WSACleanup();
        return -1;
    }

    return iResult;
}


int Network::recvFromHost(char* buffer)
{
    if (!m_connected) {
        return -1;
    }

    int iResult = ::recv(m_socket, buffer, DEFAULT_BUFLEN, 0);
    if (iResult == 0)
        OutputDebugString(L"Connection closed\n");
    else if (iResult == -1)
        OutputDebugString(L"recv failed\n");

    return iResult;
}


int Network::sendFlag(char val)
{
    char flag[2] = { val, '\0' };
    return sendToHost(flag);
}


int Network::recvFlag(char val)
{
    char res[512]{};
    recvFromHost(res);
    if (res[0] != val)
        return -1;
    return 0;
}


void Network::close()
{
    ::freeaddrinfo(m_address);
    ::closesocket(m_socket);
    ::WSACleanup();
}


size_t Network::getBufLen()
{
    return DEFAULT_BUFLEN;
}