#ifndef NETWORK_H
#define NETWORK_H

#include <winsock2.h>
#include <ws2tcpip.h>

#include <QString>
#include <QDebug>


class Network
{
public:
    enum Request
    {
        NEXT_DATA = 1,
        GET_RT_DEPS,
        HOOK_PREVIEW,
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

    static void startServer();
    static void accept(int procId);
    static void close();

    static int sendData(const QString& data);
    static int recvData(QString& data);

    static bool isStarted();
    static bool isPaired();
    static int getProcId();
    static QString getPort();

private:
    inline static int m_procId = -1;

    inline static const char* DEFAULT_PORT{"7008"};
    inline static const u_int64 DEFAULT_BUFLEN{512};

    inline static bool m_inited;
    inline static bool m_paired;

    inline static SOCKET m_listen;
    inline static SOCKET m_client;
};

#endif // NETWORK_H
