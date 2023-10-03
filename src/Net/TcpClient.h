#pragma once
#include <iostream>
#include <winsock2.h>
#include <string>
#include <mutex>

enum class TcpError {
    Ok,
	InvalidIP,
    SockError,
    NoConnect
};

class TcpClient {
    WSADATA wsaData{};
    SOCKET m_socket = INVALID_SOCKET;
    sockaddr_in m_dest{};
    TcpError m_error = TcpError::Ok;

    bool m_hasConnect = false;
    std::recursive_mutex m_lock;

    bool _send(LPCVOID data, SIZE_T size);
    bool _recv(void* buffer, uint32_t size) const;
public:
    TcpClient(LPCSTR ip, USHORT port);
    ~TcpClient();

    bool IsConnected() const;

    TcpError Error() const {
	    return m_error;
    }

    bool Initialize();
    void CloseSocket();
    bool Reinitialize();

    bool Send(const std::string& message);
    bool Send(const BYTE* data, SIZE_T size);

    bool Receive(BYTE* buffer, uint32_t size) const;
    std::string Receive(uint32_t size) const;
};