#pragma once

#include "IInput.hh"    // enthält IInput, IValue und ValueKind
#include "./../../iHAL.hh"      // dein Hardware-Abstraction-Layer
#include "values_impl.cc"    // concrete Value-Klassen wie ValueBool

#include <memory>

class GreenButtonInput : public IInput {
public:
    explicit GreenButtonInput(iHAL* hal) noexcept
        : hal_(hal), callback_(nullptr) {}

    ~GreenButtonInput() override = default;

    const char* name() const noexcept override {
        return "GreenButton";
    }

    ValueKind valueType() const noexcept override {
        return ValueKind::Bool;
    }

    std::unique_ptr<IValue> read() const override {
        bool state = false;
        if (hal_) state = hal_->GetButtonGreenIsPressed();
        return std::make_unique<ValueBool>(state);
    }

    void setCallback(Callback cb) override {
        callback_ = std::move(cb);
    }

    void notifyCallbackIfSet(const char* sourceAlias = nullptr) const {
        if (!callback_) return;
        auto val = read();
        if (val) callback_(*val, sourceAlias);
    }

private:
    iHAL* hal_;
    Callback callback_;
};


class RedButtonInput : public IInput {
public:
    explicit RedButtonInput(iHAL* hal) noexcept
        : hal_(hal), callback_(nullptr) {}

    ~RedButtonInput() override = default;

    const char* name() const noexcept override {
        return "RedButton";
    }

    ValueKind valueType() const noexcept override {
        return ValueKind::Bool;
    }

    std::unique_ptr<IValue> read() const override {
        bool state = false;
        if (hal_) state = hal_->GetButtonRedIsPressed();
        return std::make_unique<ValueBool>(state);
    }

    void setCallback(Callback cb) override {
        callback_ = std::move(cb);
    }

    void notifyCallbackIfSet(const char* sourceAlias = nullptr) const {
        if (!callback_) return;
        auto val = read();
        if (val) callback_(*val, sourceAlias);
    }

private:
    iHAL* hal_;
    Callback callback_;
};


class FanOutput : public IOutput {
public:
    explicit FanOutput(iHAL* hal, uint8_t fanIndex, const char* name = "Fan") noexcept
        : hal_(hal), index_(fanIndex), name_(name) {}

    ~FanOutput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    ValueKind valueKind() const noexcept override {
        return ValueKind::Float;
    }

    // Accepts IValue carrying a float/double/int; converts to float percent and calls HAL.
    void write(const IValue& v) override {
        if (!hal_) return;

        // Try best-effort conversions for supported types
        float duty = 0.0f;
        bool ok = try_extract_to_float(v, duty);
        if (!ok) {
            // unsupported type — ignore or log as appropriate for your system
            return;
        }

        // Clamp to [0, 100]
        duty = std::clamp(duty, 0.0f, 100.0f);

        // Call HAL; ignore result or handle ErrorCode as needed
        hal_->SetFanDuty(index_, duty);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;

    // Tries various conversions based on runtime type info and ValueKind
    static bool try_extract_to_float(const IValue& v, float& out) noexcept {
        // Fast path using ValueKind if available
        switch (v.kind()) {
            case ValueKind::Float: {
                float tmp = 0.0f;
                if (v.get<float>(tmp)) { out = tmp; return true; }
                return false;
            }
            case ValueKind::Double: {
                double tmp = 0.0;
                if (v.get<double>(tmp)) { out = static_cast<float>(tmp); return true; }
                return false;
            }
            case ValueKind::Int32: {
                std::int32_t tmp = 0;
                if (v.get<std::int32_t>(tmp)) { out = static_cast<float>(tmp); return true; }
                return false;
            }
            case ValueKind::UInt32: {
                std::uint32_t tmp = 0;
                if (v.get<std::uint32_t>(tmp)) { out = static_cast<float>(tmp); return true; }
                return false;
            }
            case ValueKind::Int64: {
                std::int64_t tmp = 0;
                if (v.get<std::int64_t>(tmp)) { out = static_cast<float>(tmp); return true; }
                return false;
            }
            case ValueKind::UInt64: {
                std::uint64_t tmp = 0;
                if (v.get<std::uint64_t>(tmp)) { out = static_cast<float>(tmp); return true; }
                return false;
            }
            case ValueKind::Bool: {
                bool tmp = false;
                if (v.get<bool>(tmp)) { out = tmp ? 100.0f : 0.0f; return true; }
                return false;
            }
            default: break;
        }

        // // Fallback: match by type_info (covers custom kinds)
        // const std::type_info& ti = v.value_type();
        // if (ti == typeid(float)) {
        //     float tmp = 0.0f; return v.get<float>(tmp) ? (out = tmp, true) : false;
        // }
        // if (ti == typeid(double)) {
        //     double tmp = 0.0; return v.get<double>(tmp) ? (out = static_cast<float>(tmp), true) : false;
        // }
        // if (ti == typeid(std::int32_t)) {
        //     std::int32_t tmp = 0; return v.get<std::int32_t>(tmp) ? (out = static_cast<float>(tmp), true) : false;
        // }
        // if (ti == typeid(std::uint32_t)) {
        //     std::uint32_t tmp = 0; return v.get<std::uint32_t>(tmp) ? (out = static_cast<float>(tmp), true) : false;
        // }
        // if (ti == typeid(std::int64_t)) {
        //     std::int64_t tmp = 0; return v.get<std::int64_t>(tmp) ? (out = static_cast<float>(tmp), true) : false;
        // }
        // if (ti == typeid(std::uint64_t)) {
        //     std::uint64_t tmp = 0; return v.get<std::uint64_t>(tmp) ? (out = static_cast<float>(tmp), true) : false;
        // }
        // if (ti == typeid(bool)) {
        //     bool tmp = false; return v.get<bool>(tmp) ? (out = tmp ? 100.0f : 0.0f, true) : false;
        // }

        // unsupported
        return false;
    }
};
