#pragma once

#include <functional>
#include <memory>
#include "../entities/IValue.hh"// enthält IValue und ValueKind

class IInput {
public:
    using Callback = std::function<void(const IValue& newValue, const char* sourceAlias)>;

    virtual ~IInput() = default;
    virtual const char* name() const noexcept = 0;
    virtual ValueKind valueType() const noexcept = 0;
    virtual std::unique_ptr<IValue> read() const = 0;
    virtual void setCallback(Callback cb) = 0;
};
