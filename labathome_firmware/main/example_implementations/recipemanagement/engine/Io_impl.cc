#pragma once

#include "../../../recipemanagement/core/interfaces/engine/IInput.hh"
#include "../../../recipemanagement/core/interfaces/engine/IOutput.hh"
#include "../../../iHAL.hh"
#include <algorithm>

//Button Inputs

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

//Sensor Inputs
class MovementInput : public IInput {
public:
    explicit MovementInput(iHAL* hal) noexcept
        : hal_(hal) {}

    ~MovementInput() override = default;

    const char* name() const noexcept override {
        return "Movement";
    }

    ParameterValue read() const override {
        bool state = false;
        if (hal_) state = hal_->IsMovementDetected();
        return ParameterValue::fromBoolean(state);
    }

private:
    iHAL* hal_;
};

class FanDutySensorInput : public IInput {
public:
    explicit FanDutySensorInput(iHAL* hal, uint8_t fanIndex, const char* name = "FanDutySensor") noexcept
        : hal_(hal), index_(fanIndex), name_(name) {}

    ~FanDutySensorInput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    ParameterValue read() const override {
        float duty = 0.0f;
        if (hal_) {
            hal_->GetFanDuty(index_, &duty);
        }
        return ParameterValue::fromPercentage(duty);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;
};


class TemperatureInput : public IInput {
public:
    explicit TemperatureInput(iHAL* hal, const char* name = "Temperature") noexcept
        : hal_(hal), name_(name) {}

    ~TemperatureInput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    ParameterValue read() const override {
        float temperature = 0.0f;
        if (hal_) {
            hal_->GetAirTemperatureDS18B20(&temperature);
        }
        return ParameterValue::fromTemperature(temperature, TemperatureUnit::CELSIUS);
    }

private:
    iHAL* hal_;
    const char* name_;
};

class HeaterTemperatureInput : public IInput {
public:
    explicit HeaterTemperatureInput(iHAL* hal, const char* name = "HeaterTemperature") noexcept
        : hal_(hal), name_(name) {}

    ~HeaterTemperatureInput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    ParameterValue read() const override {
        float temperature = 0.0f;
        if (hal_) {
            hal_->GetHeaterTemperature(&temperature);
        }
        return ParameterValue::fromTemperature(temperature, TemperatureUnit::CELSIUS);
    }

private:
    iHAL* hal_;
    const char* name_;
};


// ==================== Outputs ====================

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

        if (v.isType(ParameterType::RPM)) {
            duty = static_cast<float>(v.getRPM()) / 100.0f;
        } else {
            duty = v.toFloat();
            if (v.isType(ParameterType::BOOLEAN)) {
                duty = duty > 0.0f ? 100.0f : 0.0f;
            }
        }

        duty = std::clamp(duty, 0.0f, 100.0f);
        hal_->SetFanDuty(index_, duty);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;
};


class HeaterOutput : public IOutput {
public:
    explicit HeaterOutput(iHAL* hal, const char* name = "Heater") noexcept
        : hal_(hal), name_(name) {}

    ~HeaterOutput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    void write(const ParameterValue& v) override {
        if (!hal_) return;

        float duty = v.toFloat();
        if (v.isType(ParameterType::BOOLEAN)) {
            duty = duty > 0.0f ? 100.0f : 0.0f;
        }

        duty = std::clamp(duty, 0.0f, 100.0f);
        hal_->SetHeaterDuty(duty);
    }

private:
    iHAL* hal_;
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

        uint32_t color = 0;
        
        if (v.isType(ParameterType::COLOR)) {
            color = v.getColor();
        } else if (v.isType(ParameterType::GENERIC_INT)) {
            color = static_cast<uint32_t>(v.getGenericInt());
        } else if (v.isType(ParameterType::BOOLEAN)) {
            color = v.getBoolean() ? 0xFFFFFF : 0x000000;
        }

        hal_->ColorizeLed(index_, color);
    }

private:
    iHAL* hal_;
    uint8_t index_;
    const char* name_;
};

class ServoZeroOutput : public IOutput {
public:
    explicit ServoZeroOutput(iHAL* hal, const char* name = "Servo") noexcept
        : hal_(hal), name_(name) {}

    ~ServoZeroOutput() override = default;

    const char* name() const noexcept override {
        return name_;
    }

    void write(const ParameterValue& v) override {
        if (!hal_) return;

        float angle = 0.0f;

        if (v.isType(ParameterType::ANGLE)) {
            angle = v.getAngle();
        } else if (v.isType(ParameterType::GENERIC_INT)) {
            angle = static_cast<float>(v.getGenericInt());
        } else if (v.isType(ParameterType::PERCENTAGE)) {
            angle = (v.getPercentage() / 100.0f) * 180.0f;
        } else if (v.isType(ParameterType::BOOLEAN)) {
            angle = v.getBoolean() ? 180.0f : 0.0f;
        }

        angle = std::clamp(angle, 0.0f, 180.0f);
        hal_->SetServoPosition(1, angle);
    }

private:
    iHAL* hal_;
    const char* name_;
};
