#pragma once

#include <functional>
#include "../../domain/value-objects/ParameterValue.hh"

class IInput {
public:
    virtual ~IInput() = default;
    virtual const char* name() const noexcept = 0;
    virtual ParameterValue read() const = 0;
};
