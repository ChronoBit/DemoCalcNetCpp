#include "TcpClient.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

constexpr auto PART_SIZE = 65536;

bool TcpClient::_send(LPCVOID data, SIZE_T size) {
	std::scoped_lock lock(m_lock);

	if (!m_hasConnect) {
		return false;
	}

	const auto res = send(m_socket, static_cast<LPCSTR>(data), static_cast<int>(size), 0) != SOCKET_ERROR;
	if (!res) m_hasConnect = false;
	return res;
}

bool TcpClient::_recv(void* buffer, uint32_t size) const {
	 return recv(m_socket, (char*)buffer, size, 0) != SOCKET_ERROR;
}

TcpClient::TcpClient(LPCSTR ip, USHORT port) {
	m_dest.sin_family = AF_INET;
	m_dest.sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &m_dest.sin_addr) != 1) {
		m_error = TcpError::InvalidIP;
		return;
    }
}

TcpClient::~TcpClient() {
	CloseSocket();
}

bool TcpClient::IsConnected() const {
	return m_hasConnect;
}

bool TcpClient::Initialize() {
	std::scoped_lock lock(m_lock);

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		m_error = TcpError::SockError;
		return false;
	}

	m_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_socket == INVALID_SOCKET) {
		m_error = TcpError::SockError;
		m_hasConnect = false;
		WSACleanup();
		return false;
	}

	if (connect(m_socket, reinterpret_cast<sockaddr*>(&m_dest), sizeof m_dest) == SOCKET_ERROR) {
        CloseSocket();
		m_error = TcpError::NoConnect;
		return false;
    }

	m_hasConnect = true;
	return true;
}

void TcpClient::CloseSocket() {
	std::scoped_lock lock(m_lock);

	if (m_socket == INVALID_SOCKET) 
		return;

	closesocket(m_socket);
	WSACleanup();
	m_socket = INVALID_SOCKET;
	m_hasConnect = false;
}

bool TcpClient::Reinitialize() {
	CloseSocket();
	return Initialize();
}

bool TcpClient::Send(const std::string& message) {
	return Send((BYTE*)message.data(), message.size());
}

bool TcpClient::Send(const BYTE* data, SIZE_T size) {
	if (size > PART_SIZE) {
		for (SIZE_T i = 0; i < size; i += PART_SIZE) {
			const auto total = min(size - i, PART_SIZE);
			if (!_send(&data[i], total)) {
				return false;
			}
		}
		return true;
	}

	return _send(data, size);
}

bool TcpClient::Receive(BYTE* buffer, uint32_t size) const {
	if (size > PART_SIZE) {
		for (uint32_t i = 0; i < size; i += PART_SIZE) {
			const auto total = min(size - i, PART_SIZE);
			if (!_recv(&buffer[i], total)) {
				return false;
			}
		}
		return true;
	}
    return _recv(buffer, size);
}

std::string TcpClient::Receive(uint32_t size) const {
	std::string res;
	res.resize(size);
	if (!Receive((BYTE*)res.data(), size)) return {};
	return res;
}
