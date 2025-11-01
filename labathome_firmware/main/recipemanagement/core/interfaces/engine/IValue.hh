#pragma once
#include <memory>
#include <typeinfo>
#include <cstdint>
#include <cstddef>

using UnitId = std::uint32_t;

enum class ValueKind : std::uint8_t {
    Unknown = 0,
    Bool = 1,
    Int32 = 2,
    UInt32 = 3,
    Int64 = 4,
    UInt64 = 5,
    Float = 6,
    Double = 7,
    Custom = 255
};

class IValue {
public:
    virtual ~IValue() = default;
    virtual UnitId unit() const noexcept = 0;
    virtual const char* name() const noexcept = 0;
    virtual ValueKind kind() const noexcept { return ValueKind::Unknown; }
    virtual std::unique_ptr<IValue> clone() const = 0;
    virtual const std::type_info& value_type() const noexcept = 0;
    virtual std::size_t value_size() const noexcept = 0;

    template<typename T>
    bool get(T& out) const noexcept {
        if (value_type() != typeid(T)) return false;
        if (value_size() != sizeof(T)) return false;
        return copy_to(static_cast<void*>(&out), sizeof(T));
    }

    template<typename T>
    bool set(const T& in) noexcept {
        if (value_type() != typeid(T)) return false;
        if (value_size() != sizeof(T)) return false;
        return copy_from(static_cast<const void*>(&in), sizeof(T));
    }

protected:
    virtual bool copy_to(void* out, std::size_t n) const noexcept = 0;
    virtual bool copy_from(const void* in, std::size_t n) noexcept = 0;
};
