#pragma once

#include "IValue.hh"
#include <memory>

class IOutput {
public:
    virtual ~IOutput() = default;

    // name of the resource
    virtual const char* name() const noexcept = 0;

    // value kind supported by this output
    virtual ValueKind valueKind() const noexcept = 0;

    // write a value to the output (e.g., set a relay, motor speed, etc.)
    // The implementation may check type/size and perform conversions where sensible.
    virtual void write(const IValue& v) = 0;
};
