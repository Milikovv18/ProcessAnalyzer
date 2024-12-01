#include "network.h"


void Network::startServer()
{
    if (m_inited) {
        return;
    }

    WSADATA wsaData;
    int iResult;

    struct addrinfo *result = NULL;
    struct addrinfo hints;

    // Initialize Winsock
    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        throw "WSAStartup failed";
    }

    ZeroMemory(&hints, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_PASSIVE;

    // Resolve the server address and port
    iResult = getaddrinfo("127.0.0.1", DEFAULT_PORT, &hints, &result);
    if (iResult != 0) {
        WSACleanup();
        throw "getaddrinfo failed";
    }

    // Create a SOCKET for connecting to server
    m_listen = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
    if (m_listen == INVALID_SOCKET) {
        freeaddrinfo(result);
        WSACleanup();
        throw "Socket failed";
    }

    // Setup the TCP listening socket
    iResult = bind(m_listen, result->ai_addr, (int)result->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        freeaddrinfo(result);
        closesocket(m_listen);
        WSACleanup();
        throw "Bind failed";
    }

    freeaddrinfo(result);

    iResult = ::listen(m_listen, SOMAXCONN);
    if (iResult == SOCKET_ERROR) {
        closesocket(m_listen);
        WSACleanup();
        throw "Listen failed";
    }

    m_inited = true;
}


void Network::accept(int procId)
{
    if (m_paired) {
        return;
    }
    if (!m_inited) {
        startServer();
    }

    m_client = ::accept(m_listen, NULL, NULL);
    if (m_client == INVALID_SOCKET)
    {
        closesocket(m_listen);
        WSACleanup();
        throw "Accept failed";
    }

    // Check code
    char passcode[7]{};
    ::recv(m_client, passcode, 7, 0);

    if (strcmp(passcode, "010101") != 0)
        throw "Fucking spy";

    // Il ne faut plus entendre
    closesocket(m_listen);

    m_procId = procId;
    m_paired = true;
}


void Network::close()
{
    m_procId = -1;
    m_inited = false;
    m_paired = false;
    m_listen = INVALID_SOCKET;
    m_client = INVALID_SOCKET;
    ::closesocket(m_client);
    ::WSACleanup();
}



int Network::sendData(const QString& data)
{
    if (!m_paired) {
        throw "Pairing first idiot";
    }

    int iResult = ::send(m_client, data.toStdString().c_str(), data.size(), 0);
    if (iResult == SOCKET_ERROR) {
        printf("send failed with error: %d\n", WSAGetLastError());
        closesocket(m_client);
        WSACleanup();
        return 1;
    }

    return 0;
}

int Network::recvData(QString& data)
{
    if (!m_paired) {
        throw "Pairing first idiot";
    }

    char tempBuf[DEFAULT_BUFLEN]{};
    int iResult = ::recv(m_client, tempBuf, DEFAULT_BUFLEN, 0);
    wchar_t tempBuf2[2*DEFAULT_BUFLEN];

    qDebug() << QString(tempBuf).left(iResult).toWCharArray(tempBuf2);
    data = tempBuf;

    return iResult;
}


bool Network::isStarted()
{
    return m_inited;
}


bool Network::isPaired()
{
    return m_paired;
}


QString Network::getPort()
{
    return DEFAULT_PORT;
}


int Network::getProcId()
{
    return m_procId;
}





