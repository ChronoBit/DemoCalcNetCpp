#pragma once
#include "../Net/IPacket.h"

enum CalcError {
    Ok,
    InvalidInput,
    DivByZero
};

#include <iostream>
PACKET(CalcResponse, CalculationResult) {
	SFIELD(CalcError, error);
    SFIELD(double, result);

    void Parse() override;
};