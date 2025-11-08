#pragma once

#include "../../../recipemanagement/core/interfaces/engine/IInput.hh"
#include "../../../recipemanagement/core/interfaces/engine/IOutput.hh"
#include "../../../iHAL.hh"
#include <algorithm>

class GreenButtonInput : public IInput {
public:
    explicit GreenButtonInput(iHAL* hal) noexcept
        : hal_(hal) {}

    ~GreenButtonInput() override = default;

    const char* name() const noexcept override {
        return "GreenButton";
    }

    ParameterValue read() const override {
        bool state = false;
        if (hal_) state = hal_->GetButtonGreenIsPressed();
        return state;
    }

private:
    iHAL* hal_;
};


class RedButtonInput : public IInput {
public:
    explicit RedButtonInput(iHAL* hal) noexcept
        : hal_(hal) {}

    ~RedButtonInput() override = default;

    const char* name() const noexcept override {
        return "RedButton";
    }

    ParameterValue read() const override {
        bool state = false;
        if (hal_) state = hal_->GetButtonRedIsPressed();
        return state;
    }

private:
    iHAL* hal_;
};


class FanOutput : public IOutput {
public:
    explicit FanOutput(iHAL* hal, uint8_t fanIndex, const char* name = "Fan") noexcept
        : hal_(hal), index_(fanIndex), name_(name) {}

    ~FanOutput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    void write(const ParameterValue& v) override {
        if (!hal_) return;

        float duty = 0.0f;
        
        // Extract float from variant
        std::visit([&duty](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, float>) {
                duty = arg;
            } else if constexpr (std::is_same_v<T, double>) {
                duty = static_cast<float>(arg);
            } else if constexpr (std::is_same_v<T, int32_t> || std::is_same_v<T, uint32_t> ||
                                 std::is_same_v<T, int64_t> || std::is_same_v<T, uint64_t>) {
                duty = static_cast<float>(arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                duty = arg ? 100.0f : 0.0f;
            }
            // std::monostate and std::string are ignored (duty remains 0.0f)
        }, v);

        // Clamp to [0, 100]
        duty = std::clamp(duty, 0.0f, 100.0f);

        // Call HAL
        hal_->SetFanDuty(index_, duty);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;
};


class LedOutput : public IOutput {
public:
    explicit LedOutput(iHAL* hal, uint8_t ledIndex, const char* name = "LED") noexcept
        : hal_(hal), index_(ledIndex), name_(name) {}

    ~LedOutput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    void write(const ParameterValue& v) override {
        if (!hal_) return;

        // Accept either a uint32_t color value or turn off if 0/false
        uint32_t color = 0;
        
        std::visit([&color](auto&& arg) {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, uint32_t>) {
                color = arg;
            } else if constexpr (std::is_same_v<T, int32_t>) {
                color = static_cast<uint32_t>(arg);
            } else if constexpr (std::is_same_v<T, uint64_t>) {
                color = static_cast<uint32_t>(arg);
            } else if constexpr (std::is_same_v<T, int64_t>) {
                color = static_cast<uint32_t>(arg);
            } else if constexpr (std::is_same_v<T, bool>) {
                color = arg ? 0xFFFFFF : 0x000000; // White on true, off on false
            } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
                color = static_cast<uint32_t>(arg);
            }
            // std::monostate and std::string are ignored (color remains 0)
        }, v);

        hal_->ColorizeLed(index_, color);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;
};
