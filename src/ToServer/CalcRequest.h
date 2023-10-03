#pragma once
#include "../Net/IPacket.h"

PACKET(CalcRequest, CalculateExpression) {
	SFIELD(std::vector<std::string>, operations);
};