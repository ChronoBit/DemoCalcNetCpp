#pragma once
#include <istream>

#undef min

class StreamReader {
	std::istream& m_stream;
	size_t m_end;
public:
	StreamReader(std::istream& stream) : m_stream(stream) {
		auto cur = pos();
		m_end = seek(0, std::ios::end);
		cur = seek(cur);
	}

	std::streampos pos() const {
		return m_stream.tellg();
	}

	std::streampos seek(const std::streampos& index, const int offset = std::ios::beg) const {
		m_stream.seekg(index, offset);
		return pos();
	}

	size_t remain() const {
		return m_end - pos();
	}

	bool hasRemain(size_t count) const {
		return count <= remain();
	}

	size_t read(void* buffer, size_t size) const {
		if (!m_stream.good()) return false;
		const auto need = std::min(remain(), size);
		m_stream.read((char*)buffer, size);
		return need;
	}
};