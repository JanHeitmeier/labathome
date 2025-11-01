#pragma once
#include "IValue.hh"
#include <type_traits>
#include <cstring>
#include <memory>

template<typename Derived, typename T, typename Tag>
class ValueBase : public IValue {
    static_assert(std::is_trivial<T>::value, "non-trivial types not supported");
public:
    constexpr ValueBase() noexcept : _v{} {}
    explicit ValueBase(T v) noexcept : _v(v) {}
    UnitId unit() const noexcept override { return Tag::id; }
    const char* name() const noexcept override { return Tag::name; }
    std::unique_ptr<IValue> clone() const override {
        return std::make_unique<Derived>(static_cast<const Derived&>(*this));
    }
    const std::type_info& value_type() const noexcept override { return typeid(T); }
    std::size_t value_size() const noexcept override { return sizeof(T); }

    ValueKind kind() const noexcept override {
        if constexpr (std::is_same_v<T, bool>) return ValueKind::Bool;
        if constexpr (std::is_same_v<T, float>) return ValueKind::Float;
        if constexpr (std::is_same_v<T, double>) return ValueKind::Double;
        if constexpr (std::is_same_v<T, std::int32_t>) return ValueKind::Int32;
        if constexpr (std::is_same_v<T, std::uint32_t>) return ValueKind::UInt32;
        if constexpr (std::is_same_v<T, std::int64_t>) return ValueKind::Int64;
        if constexpr (std::is_same_v<T, std::uint64_t>) return ValueKind::UInt64;
        return ValueKind::Custom;
    }

protected:
    bool copy_to(void* out, std::size_t n) const noexcept override {
        if (!out || n != sizeof(T)) return false;
        std::memcpy(out, &_v, n);
        return true;
    }
    bool copy_from(const void* in, std::size_t n) noexcept override {
        if (!in || n != sizeof(T)) return false;
        std::memcpy(&_v, in, n);
        return true;
    }
    T _v;
};
