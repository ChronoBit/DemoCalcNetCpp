#include "IPacket.h"
#include <functional>

enum {
	MAX_STRING_SIZE = 32536,
	MAX_LIST_SIZE = 4096
};

#define TYPE_DEF(type) typeid(type), [](void* ptr, const FieldInfo& field)
#define UNPACK_DEF(type) typeid(type), [](void* ptr, const FieldInfo& field, StreamReader& sr)

std::shared_ptr<PacketMap> packetsMap;
std::shared_ptr<OpcodeMap> opcodesMap;
std::shared_ptr<DefMap> defsMap;
std::unordered_map<std::type_index, PackerFunc> packers = {
	{TYPE_DEF(bool) {
		return std::string((char*)ptr, sizeof(bool));
	}},
	{TYPE_DEF(uint8_t) {
		return std::string((char*)ptr, sizeof(uint8_t));
	}},
	{TYPE_DEF(int8_t) {
		return std::string((char*)ptr, sizeof(int8_t));
	}},
	{TYPE_DEF(uint16_t) {
		return std::string((char*)ptr, sizeof(uint16_t));
	}},
	{TYPE_DEF(int16_t) {
		return std::string((char*)ptr, sizeof(int16_t));
	}},
	{TYPE_DEF(uint32_t) {
		return std::string((char*)ptr, sizeof(uint32_t));
	}},
	{TYPE_DEF(int32_t) {
		return std::string((char*)ptr, sizeof(int32_t));
	}},
	{TYPE_DEF(uint64_t) {
		return std::string((char*)ptr, sizeof(uint64_t));
	}},
	{TYPE_DEF(int64_t) {
		return std::string((char*)ptr, sizeof(int64_t));
	}},
	{TYPE_DEF(float) {
		return std::string((char*)ptr, sizeof(float));
	}},
	{TYPE_DEF(double) {
		return std::string((char*)ptr, sizeof(double));
	}},
	{TYPE_DEF(const std::string&) {
		const auto res = *(std::string*)ptr;
		const auto sz = (int)res.size();
		return std::string((const char*)&sz, sizeof(int)) + res;
	}},
};

std::string packField(void* obj, const FieldInfo& field) {
	if (field.elementType) {
		const auto& pVector = *(std::vector<uint8_t>*)((uint64_t)obj + field.offset);
		if (pVector.empty()) return {};

		std::string res;
		if (const auto f = findType(*field.elementType)) {
			const auto size = (uint32_t)(pVector.size() / field.elementSize);
			res += std::string((const char*)&size, sizeof(int));
			for (uint32_t i = 0; i < size; i++) {
				const auto& ptr = (void*)((uint64_t)pVector.data() + (uint64_t)i * field.elementSize);
				for (const auto& cField : f->fields) {
					res += packField(ptr, cField);
				}
			}
			return res;
		}

		const auto find = packers.find(*field.elementType);
		if (find == packers.end()) return {};

		const auto size = (uint32_t)(pVector.size() / field.elementSize);
		res += std::string((const char*)&size, sizeof(int));
		for (uint32_t i = 0; i < size; i++) {
			const auto& ptr = (void*)((uint64_t)pVector.data() + (uint64_t)i * field.elementSize);
			res += find->second(ptr, field);
		}

		return res;
	}
	const auto find = packers.find(field.type);
	if (find == packers.end()) return {};
	return find->second((void*)((uint64_t)obj + field.offset), field);
}

#define UNPACK_VAL(type, error) if (sr.read(ptr, sizeof(type)) < sizeof(type)) throw std::exception(error)

std::unordered_map<std::type_index, UnpackerFunc> unpackers = {
	{UNPACK_DEF(bool) {
		UNPACK_VAL(bool, "Invalid bool");
	}},
	{UNPACK_DEF(uint8_t) {
		UNPACK_VAL(uint8_t, "Invalid uint8");
	}},
	{UNPACK_DEF(int8_t) {
		UNPACK_VAL(int8_t, "Invalid int8");
	}},
	{UNPACK_DEF(uint16_t) {
		UNPACK_VAL(uint16_t, "Invalid uint16");
	}},
	{UNPACK_DEF(int16_t) {
		UNPACK_VAL(int16_t, "Invalid int16");
	}},
	{UNPACK_DEF(uint32_t) {
		UNPACK_VAL(uint32_t, "Invalid uint32");
	}},
	{UNPACK_DEF(int32_t) {
		UNPACK_VAL(int32_t, "Invalid uint32");
	}},
	{UNPACK_DEF(uint64_t) {
		UNPACK_VAL(uint64_t, "Invalid uint64");
	}},
	{UNPACK_DEF(int64_t) {
		UNPACK_VAL(int64_t, "Invalid int64");
	}},
	{UNPACK_DEF(float) {
		UNPACK_VAL(float, "Invalid float");
	}},
	{UNPACK_DEF(double) {
		UNPACK_VAL(double, "Invalid double");
	}},
	{UNPACK_DEF(const std::string&) {
		uint32_t size = 0;
		if (sr.read(&size, sizeof size) < sizeof size) 
			throw std::exception("Inavlid string size");
		if (size > MAX_STRING_SIZE)
			throw std::exception("String is too long");
		auto& res = *(std::string*)ptr;
		res.resize(size);
		if (sr.read(res.data(), size) < size)
			throw std::exception("Inavlid string data");
	}},
};

void unpackField(void* obj, const FieldInfo& field, StreamReader& sr) {
	if (field.elementType) {
		auto& pVector = *(std::vector<uint8_t>*)((uint64_t)obj + field.offset);

		uint32_t size = 0;
		if (sr.read(&size, sizeof size) < sizeof size) 
			throw std::exception("Inavlid list size");
		if (size > MAX_LIST_SIZE)
			throw std::exception("List size is too long");
		pVector.resize((uint64_t)size * field.elementSize);
		
		if (const auto f = findType(*field.elementType)) {
			for (uint32_t i = 0; i < size; i++) {
				const auto& ptr = (void*)((uint64_t)pVector.data() + (uint64_t)i * field.elementSize);
				const auto def = findDef(*field.elementType);
				memcpy(ptr, def.data(), def.size());
				for (const auto& cField : f->fields) {
					unpackField(ptr, cField, sr);
				}
			}
			return;
		}

		const auto find = unpackers.find(*field.elementType);
		if (find == unpackers.end()) return;
		for (uint32_t i = 0; i < size; i++) {
			const auto& ptr = (void*)((uint64_t)pVector.data() + (uint64_t)i * field.elementSize);
			find->second(ptr, field, sr);
		}
		return;
	}
	const auto find = unpackers.find(field.type);
	if (find == unpackers.end()) return;
	find->second((void*)((uint64_t)obj + field.offset), field, sr);
}