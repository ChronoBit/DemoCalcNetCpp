#pragma once
#include "TcpClient.h"
#include "IPacket.h"
#include <deque>

// Central TCP worker
class NetClient {
	TcpClient m_tcp;
	std::deque<std::string> m_packets;

	std::condition_variable m_cv;
	std::mutex m_mtx, m_qMtx;
	bool m_signal = false;

	void signal(bool state);

	bool recv() const;
	[[noreturn]] void runner();
	[[noreturn]] void queueRunner();
public:
	NetClient(LPCSTR ip, USHORT port);

	std::function<void()> OnDisconnect;
	
	bool IsConnected() const {
		return m_tcp.IsConnected();
	}
	
	// Runs automatic receiver
	void Run();
	// Send packet to server
	void Send(IPacketBody& packet);
};