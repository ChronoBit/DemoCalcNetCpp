#include "NetClient.h"
#include <sstream>

void NetClient::signal(bool state) {
	const auto flag = state != m_signal;
	std::lock_guard lock(m_mtx);
	m_signal = state;
	if (flag) m_cv.notify_one();
}

bool NetClient::recv() const {
	auto headBuff = m_tcp.Receive(2);
	if (headBuff.empty()) return false;
	if (const auto head = *reinterpret_cast<uint16_t*>(headBuff.data()); head != 0x6529)
		return false;

	auto lenBuff = m_tcp.Receive(4);
	if (lenBuff.empty()) return false;
	const auto len = *reinterpret_cast<uint32_t*>(lenBuff.data());
	if (len > 64 * 1024 * 1024) return false; // Too large packet

	auto data = m_tcp.Receive(len);
	if (data.empty()) return false;

	const auto opcode = *reinterpret_cast<Opcode*>(data.data());
	const auto body = findOpcode(opcode);
	if (body.empty()) return false;

	std::istringstream is(data);
	is.seekg(1, std::ios::beg);

	std::string copy(body);
	auto* packet = reinterpret_cast<IPacketBody*>(copy.data());
	packet->unpack(is);
	packet->Parse();

	return true;
}

void NetClient::runner() {
	m_tcp.Initialize();
	while (true) {
		if (!m_tcp.IsConnected()) m_tcp.Reinitialize();
		if (!m_tcp.IsConnected()) {
			Sleep(1000);
			continue;
		}
		if (!recv()) {
			if (OnDisconnect) OnDisconnect();
			m_tcp.Reinitialize();
		}
		std::this_thread::yield();
	}
}

void NetClient::queueRunner() {
	while (true) {
		m_qMtx.lock();
		if (m_packets.empty()) signal(false);
		m_qMtx.unlock();
		{
			std::unique_lock lock(m_mtx);
			m_cv.wait(lock, [this] {
				return m_signal;
			});
		}
		signal(false);
		while(!m_packets.empty()) {
			m_qMtx.lock();
			const auto& packet = m_packets.front();
			m_qMtx.unlock();
			if (!m_tcp.Send(packet)) {
				Sleep(500);
				break;
			}
			m_qMtx.lock();
			m_packets.pop_front();
			m_qMtx.unlock();
		}
	}
}

NetClient::NetClient(LPCSTR ip, USHORT port) : m_tcp(ip, port) {
	if (m_tcp.Error() != TcpError::Ok) 
		throw std::runtime_error("Failed connect");
}

void NetClient::Run() {
	std::thread(&NetClient::runner, this).detach();
	std::thread(&NetClient::queueRunner, this).detach();
}

void NetClient::Send(IPacketBody& packet) {
	const auto opcode = packet.opcode();
	if (!opcode) return;

	const auto data = packet.pack();
	constexpr uint16_t head = 0x6529;
	const auto size = static_cast<uint32_t>(data.size()) + 1;
	std::ostringstream os;
	os.write((const char*)&head, sizeof head);
	os.write((const char*)&size, sizeof size);
	os.write((const char*)&*opcode, sizeof *opcode);
	os << data;

	std::scoped_lock lock(m_qMtx);
	m_packets.push_back(os.str());
	signal(true);
}
