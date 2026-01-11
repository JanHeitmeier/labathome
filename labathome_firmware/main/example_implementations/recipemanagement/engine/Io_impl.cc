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
        return ParameterValue::fromBoolean(state);
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
        return ParameterValue::fromBoolean(state);
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
        
        // Verwende typsichere Getter der neuen ParameterValue-Architektur
        if (v.isType(ParameterType::PERCENTAGE)) {
            duty = v.getPercentage();
        } else if (v.isType(ParameterType::BOOLEAN)) {
            duty = v.getBoolean() ? 100.0f : 0.0f;
        } else if (v.isType(ParameterType::RPM)) {
            // RPM zu Prozent (0-10000 RPM → 0-100%)
            duty = static_cast<float>(v.getRPM()) / 100.0f;
        } else if (v.isType(ParameterType::GENERIC_INT)) {
            duty = static_cast<float>(v.getGenericInt());
        }

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

        // Verwende typsichere Getter der neuen ParameterValue-Architektur
        uint32_t color = 0;
        
        if (v.isType(ParameterType::GENERIC_INT)) {
            color = static_cast<uint32_t>(v.getGenericInt());
        } else if (v.isType(ParameterType::BOOLEAN)) {
            color = v.getBoolean() ? 0xFFFFFFFF : 0x00000000; // White on true, off on false
        }

        hal_->ColorizeLed(index_, color);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;
};
