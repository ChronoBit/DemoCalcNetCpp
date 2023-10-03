// ReSharper disable CppClangTidyBugproneMacroParentheses
#pragma once
#include "Opcode.h"
#include <typeindex>
#include <memory>
#include <string>
#include <unordered_map>
#include <functional>
#include <istream>
#include <optional>
#include "StreamReader.h"

class IPacketBody {
public:
	virtual ~IPacketBody() = default;
	virtual void Parse() {}
	virtual std::optional<Opcode> opcode() { return {}; }
	virtual std::string pack() { return {}; }
	virtual void unpack(std::istream& stream) {}
};

struct FieldInfo {
	uint32_t offset;
	uint16_t size;
	std::type_index type;

	std::optional<std::type_index> elementType;
	uint16_t elementSize;
};

struct PacketInfo {
	std::optional<Opcode> opcode;
	bool loaded = false;
	std::vector<FieldInfo> fields;
};

using PackerFunc = std::function<std::string(void* obj, const FieldInfo& field)>;
using UnpackerFunc = std::function<void(void* obj, const FieldInfo& field, StreamReader& sr)>;
using PacketMap = std::unordered_map<std::type_index, PacketInfo>;
using OpcodeMap = std::unordered_map<Opcode, std::string>;
using DefMap = std::unordered_map<std::type_index, std::string>;

extern std::shared_ptr<PacketMap> packetsMap;
extern std::shared_ptr<OpcodeMap> opcodesMap;
extern std::shared_ptr<DefMap> defsMap;
extern std::unordered_map<std::type_index, PackerFunc> packers;
extern std::unordered_map<std::type_index, UnpackerFunc> unpackers;

inline PacketInfo* findType(const std::type_index& id) {
	auto& map = *packetsMap;
	const auto& find = map.find(id);
	if (find != map.end()) return &find->second;
	return nullptr;
}

inline std::string findOpcode(const Opcode opcode) {
	auto& map = *opcodesMap;
	const auto& find = map.find(opcode);
	if (find != map.end()) return find->second;
	return {};
}

inline std::string findDef(const std::type_index& id) {
	auto& map = *defsMap;
	const auto& find = map.find(id);
	if (find != map.end()) return find->second;
	return {};
}

#define IS_TYPE(type) std::is_same_v<T, type>

template <typename T>
struct IsVector {
    static constexpr bool value = false;
};

template <typename... Args>
struct IsVector<std::vector<Args...>> {
    static constexpr bool value = true;
};

template<typename T>
struct TypeExtractor {
    using ElementType = T;
};

template<typename T>
struct TypeExtractor<std::vector<T>> {
    using ElementType = T;
};

std::string packField(void* obj, const FieldInfo& field);
void unpackField(void* obj, const FieldInfo& field, StreamReader& sr);

template<typename Class>
class IPacket : public IPacketBody {
protected:
	template<typename T>
	T initField(T* ptr) {
		if (const auto find = findType(typeid(Class)); !find->loaded) {
			const auto offset = (uint32_t)((uint64_t)ptr - (uint64_t)this);
			if constexpr (IsVector<T>::value) {
				typename TypeExtractor<T>::ElementType value;
				const auto& id = typeid(value);
				find->fields.emplace_back(FieldInfo{offset, sizeof(T), typeid(T), id, sizeof value});
			} else if constexpr (std::is_enum_v<T>) {
				std::type_index type = typeid(T);
				switch (sizeof T) {
					case 1: type = typeid(uint8_t); break;
					case 2: type = typeid(uint16_t); break;
					case 4: type = typeid(uint32_t); break;
					case 8: type = typeid(uint64_t); break;
				}
				find->fields.emplace_back(FieldInfo{offset, sizeof(T), type, {}, 0});
		    } else {
				find->fields.emplace_back(FieldInfo{offset, sizeof(T), typeid(T), {}, 0});
			}
		}
	    if constexpr (IS_TYPE(int)) {
			return 0;
	    } else if constexpr (IS_TYPE(float)) {
	        return 0.0f;
	    } else if constexpr (IS_TYPE(double)) {
	        return 0.0;
	    }
		return {};
	}
public:
	static int init(std::optional<Opcode> opcode) {
		if (!packetsMap) {
			packetsMap = std::make_shared<PacketMap>();
		}
		if (!opcodesMap) {
			opcodesMap = std::make_shared<OpcodeMap>();
		}
		if (!defsMap) {
			defsMap = std::make_shared<DefMap>();
		}
		const auto& id = typeid(Class);
		if (findType(id)) return 0;

		auto& map = *packetsMap;
		map[id] = PacketInfo{opcode, false, {}};
		PacketInfo& info = map[id];

		Class data;
		info.loaded = true;

		std::string clear;
		if (info.fields.empty()) {
			clear = std::string((char*)&data, sizeof data);
		} else {
			clear = std::string((char*)&data, info.fields[0].offset);
		}
		(*defsMap)[id] = clear;

		if (opcode) {
			clear.resize(sizeof data);
			(*opcodesMap)[*opcode] = clear;
		}

		return 1;
	}

	std::optional<Opcode> opcode() override {
		if (const auto find = findType(typeid(Class))) 
			return find->opcode;
		return {};
	}

	std::string pack() override {
		std::string res;
		const auto info = findType(typeid(Class));
		for (const auto& field : info->fields) {
			res += packField(this, field);
		}
		return res;
	}

	void unpack(std::istream& stream) override {
		StreamReader sr(stream);
		const auto info = findType(typeid(Class));
		for (const auto& field : info->fields) {
			unpackField(this, field, sr);
		}
	}
};

#define VAR_NAME_LINE_HELPER(name, line, add) name ## _ ## line ## _ ## add
#define VAR_NAME_LINE(name, line, add) VAR_NAME_LINE_HELPER(name, line, add)

#define PACKET(name, opcode) \
class name; \
static auto VAR_NAME_LINE(__r, __LINE__, __COUNTER__) = IPacket<name>::init(opcode); \
class name : public IPacket<name>

#define SFIELD(fieldType, fieldName) public: fieldType fieldName = initField<fieldType>(&(fieldName))

#define STRUCT(name) PACKET(name, {})