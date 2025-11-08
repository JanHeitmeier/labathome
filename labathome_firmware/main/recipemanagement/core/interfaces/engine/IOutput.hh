#pragma once

#include "../../domain/value-objects/ParameterValue.hh"

class IOutput {
public:
    virtual ~IOutput() = default;

    // name of the resource
    virtual const char* name() const noexcept = 0;

    // write a value to the output (e.g., set a relay, motor speed, etc.)
    virtual void write(const ParameterValue& v) = 0;
};
