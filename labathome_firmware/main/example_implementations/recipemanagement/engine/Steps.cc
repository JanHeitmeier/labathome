#pragma once

#include "esp_log.h"
#include "../../../recipemanagement/core/interfaces/engine/IStep.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepContext.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepMetadata.hh"
#include <chrono>

// ==================== Test Step 1: Fan Ramp ====================
// Skaliert den Fan von einem Start-Wert zu einem End-Wert über einen Zeitraum
class FanRampStep : public StepBase
{
private:
    IoAliasDef fanOutput;
    ParamDef startDutyParam;
    ParamDef endDutyParam;
    ParamDef durationParam;
    uint32_t startTime;

public:
    FanRampStep()
        : StepBase("FanRamp", "Ramp fan speed from start to end value over time", "1.0"),
          fanOutput("Fan", false, true, false, "float", std::nullopt, "Fan"),
          startDutyParam("startDuty", 
                        ParameterValue::fromPercentage(0.0f),
                        "Start Duty",
                        "Fan duty cycle at start",
                        "%",
                        ParameterValue::fromPercentage(0.0f),
                        ParameterValue::fromPercentage(100.0f)),
          endDutyParam("endDuty", 
                      ParameterValue::fromPercentage(100.0f),
                      "End Duty",
                      "Fan duty cycle at end",
                      "%",
                      ParameterValue::fromPercentage(0.0f),
                      ParameterValue::fromPercentage(100.0f)),
          durationParam("duration", 
                       ParameterValue::fromTimeSeconds(10),
                       "Duration",
                       "Ramp duration",
                       "s",
                       ParameterValue::fromTimeSeconds(1),
                       ParameterValue::fromTimeSeconds(300)),
          startTime(0)
    {
        registerIoAliases({&fanOutput});
        registerParamDefs({&startDutyParam, &endDutyParam, &durationParam});
    }

    FanRampStep(const FanRampStep &o)
        : StepBase(o),
          fanOutput(o.fanOutput),
          startDutyParam(o.startDutyParam),
          endDutyParam(o.endDutyParam),
          durationParam(o.durationParam),
          startTime(0)
    {
        registerIoAliases({&fanOutput});
        registerParamDefs({&startDutyParam, &endDutyParam, &durationParam});
    }

    void initialize() override
    {
        setState(State::Inactive);
        startTime = 0;
    }

    void onActivating(StepContext &ctx) override
    {
        startTime = 0;
        ctx.startTimer("ramp", std::chrono::milliseconds(1));
        setState(State::Activating);
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating && ctx.isTimerExpired("ramp"))
        {
            ctx.stopTimer("ramp");
            startTime = esp_log_timestamp();
            setState(State::Active);
        }
        else if (state() == State::Active)
        {
            uint32_t now = esp_log_timestamp();
            uint32_t elapsed = now - startTime;
            
            float startDuty = readParamOrDefault(startDutyParam, 0.0f);
            float endDuty = readParamOrDefault(endDutyParam, 100.0f);
            uint32_t durationMs = readParamOrDefault(durationParam, uint32_t(10000));
            
            float progress = static_cast<float>(elapsed) / static_cast<float>(durationMs);
            if (progress > 1.0f) progress = 1.0f;
            
            float currentDuty = startDuty + (endDuty - startDuty) * progress;
            
            auto fan = ctx.getOutput(fanOutput.aliasName);
            if (fan) {
                fan->write(ParameterValue::fromPercentage(currentDuty));
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        auto fan = ctx.getOutput(fanOutput.aliasName);
        if (fan) {
            fan->write(ParameterValue::fromPercentage(0.0f));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    void onPauseImpl(StepContext &ctx) override
    {
        auto fan = ctx.getOutput(fanOutput.aliasName);
        if (fan) {
            fan->write(ParameterValue::fromPercentage(0.0f));
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && state() == State::Active) {
            startTime = esp_log_timestamp();
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active)
        {
            uint32_t now = esp_log_timestamp();
            uint32_t elapsed = now - startTime;
            uint32_t durationMs = readParamOrDefault(durationParam, uint32_t(10000));
            return elapsed >= durationMs;
        }
        return false;
    }
};

// ==================== Test Step 2: LED Sequence ====================
// Lässt 4 LEDs nacheinander mit konfigurierbaren Farben und Zeiten leuchten
class LedSequenceStep : public StepBase
{
private:
    IoAliasDef led0, led1, led2, led3;
    ParamDef color0, color1, color2, color3;
    ParamDef time0, time1, time2, time3;
    uint8_t currentLed;

public:
    LedSequenceStep()
        : StepBase("LedSequence", "Light 4 LEDs in sequence with configurable colors and times", "1.0"),
          led0("LED0", false, true, false, "uint32_t", std::nullopt, "LED0"),
          led1("LED1", false, true, false, "uint32_t", std::nullopt, "LED1"),
          led2("LED2", false, true, false, "uint32_t", std::nullopt, "LED2"),
          led3("LED3", false, true, false, "uint32_t", std::nullopt, "LED3"),
          color0("color0", ParameterValue::fromGenericInt(0xFF0000FF), "Color LED0", "Color for first LED (RGBA)", "RGBA"),
          color1("color1", ParameterValue::fromGenericInt(0x00FF00FF), "Color LED1", "Color for second LED (RGBA)", "RGBA"),
          color2("color2", ParameterValue::fromGenericInt(0x0000FFFF), "Color LED2", "Color for third LED (RGBA)", "RGBA"),
          color3("color3", ParameterValue::fromGenericInt(0xFFFF00FF), "Color LED3", "Color for fourth LED (RGBA)", "RGBA"),
          time0("time0", ParameterValue::fromTimeMilliseconds(500), "Time LED0", "Duration for first LED", "ms",
                ParameterValue::fromTimeMilliseconds(100), ParameterValue::fromTimeMilliseconds(10000)),
          time1("time1", ParameterValue::fromTimeMilliseconds(500), "Time LED1", "Duration for second LED", "ms",
                ParameterValue::fromTimeMilliseconds(100), ParameterValue::fromTimeMilliseconds(10000)),
          time2("time2", ParameterValue::fromTimeMilliseconds(500), "Time LED2", "Duration for third LED", "ms",
                ParameterValue::fromTimeMilliseconds(100), ParameterValue::fromTimeMilliseconds(10000)),
          time3("time3", ParameterValue::fromTimeMilliseconds(500), "Time LED3", "Duration for fourth LED", "ms",
                ParameterValue::fromTimeMilliseconds(100), ParameterValue::fromTimeMilliseconds(10000)),
          currentLed(0)
    {
        registerIoAliases({&led0, &led1, &led2, &led3});
        registerParamDefs({&color0, &color1, &color2, &color3, &time0, &time1, &time2, &time3});
    }

    LedSequenceStep(const LedSequenceStep &o)
        : StepBase(o),
          led0(o.led0), led1(o.led1), led2(o.led2), led3(o.led3),
          color0(o.color0), color1(o.color1), color2(o.color2), color3(o.color3),
          time0(o.time0), time1(o.time1), time2(o.time2), time3(o.time3),
          currentLed(0)
    {
        registerIoAliases({&led0, &led1, &led2, &led3});
        registerParamDefs({&color0, &color1, &color2, &color3, &time0, &time1, &time2, &time3});
    }

    void initialize() override
    {
        setState(State::Inactive);
        currentLed = 0;
    }

    void onActivating(StepContext &ctx) override
    {
        currentLed = 0;
        activateLed(ctx, 0);
        setState(State::Activating);
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            setState(State::Active);
        }
        else if (state() == State::Active)
        {
            if (ctx.isTimerExpired("ledTimer"))
            {
                ctx.stopTimer("ledTimer");
                deactivateLed(ctx, currentLed);
                currentLed++;
                
                if (currentLed < 4) {
                    activateLed(ctx, currentLed);
                }
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        for (uint8_t i = 0; i < 4; i++) {
            deactivateLed(ctx, i);
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    void onPauseImpl(StepContext &ctx) override
    {
        deactivateLed(ctx, currentLed);
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && state() == State::Active) {
            activateLed(ctx, currentLed);
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        return state() == State::Active && currentLed >= 4;
    }

private:
    void activateLed(StepContext &ctx, uint8_t ledIndex)
    {
        IoAliasDef* ledDef = nullptr;
        ParamDef* colorDef = nullptr;
        ParamDef* timeDef = nullptr;
        
        switch(ledIndex) {
            case 0: ledDef = &led0; colorDef = &color0; timeDef = &time0; break;
            case 1: ledDef = &led1; colorDef = &color1; timeDef = &time1; break;
            case 2: ledDef = &led2; colorDef = &color2; timeDef = &time2; break;
            case 3: ledDef = &led3; colorDef = &color3; timeDef = &time3; break;
            default: return;
        }
        
        auto led = ctx.getOutput(ledDef->aliasName);
        if (led) {
            uint32_t color = readParamOrDefault(*colorDef, uint32_t(0xFFFFFFFF));
            led->write(ParameterValue::fromGenericInt(color));
        }
        
        uint32_t timeMs = readParamOrDefault(*timeDef, uint32_t(500));
        ctx.startTimer("ledTimer", std::chrono::milliseconds(timeMs));
    }
    
    void deactivateLed(StepContext &ctx, uint8_t ledIndex)
    {
        IoAliasDef* ledDef = nullptr;
        
        switch(ledIndex) {
            case 0: ledDef = &led0; break;
            case 1: ledDef = &led1; break;
            case 2: ledDef = &led2; break;
            case 3: ledDef = &led3; break;
            default: return;
        }
        
        auto led = ctx.getOutput(ledDef->aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
    }
};

// ==================== Test Step 3: Movement LED Trigger ====================
// Bei Movement-Erkennung wird eine LED für eine konfigurierbare Zeit eingeschaltet
class MovementLedTriggerStep : public StepBase
{
private:
    IoAliasDef movementInput;
    IoAliasDef ledOutput;
    ParamDef ledColorParam;
    ParamDef durationParam;
    bool movementDetected;
    bool ledActive;

public:
    MovementLedTriggerStep()
        : StepBase("MovementLedTrigger", "Turns on LED when movement detected for configured time", "1.0"),
          movementInput("Movement", true, false, true, "bool", std::nullopt, "Movement"),
          ledOutput("LED", false, true, false, "uint32_t", std::nullopt, "LED0"),
          ledColorParam("ledColor", 
                       ParameterValue::fromGenericInt(0xFF00FFFF),
                       "LED Color",
                       "Color to display when movement detected (RGBA)",
                       "RGBA"),
          durationParam("duration", 
                       ParameterValue::fromTimeMilliseconds(2000),
                       "Duration",
                       "How long LED stays on after movement",
                       "ms",
                       ParameterValue::fromTimeMilliseconds(100),
                       ParameterValue::fromTimeMilliseconds(30000)),
          movementDetected(false),
          ledActive(false)
    {
        registerIoAliases({&movementInput, &ledOutput});
        registerParamDefs({&ledColorParam, &durationParam});
    }

    MovementLedTriggerStep(const MovementLedTriggerStep &o)
        : StepBase(o),
          movementInput(o.movementInput),
          ledOutput(o.ledOutput),
          ledColorParam(o.ledColorParam),
          durationParam(o.durationParam),
          movementDetected(false),
          ledActive(false)
    {
        registerIoAliases({&movementInput, &ledOutput});
        registerParamDefs({&ledColorParam, &durationParam});
    }

    void initialize() override
    {
        setState(State::Inactive);
        movementDetected = false;
        ledActive = false;
    }

    void onActivating(StepContext &ctx) override
    {
        movementDetected = false;
        ledActive = false;
        ctx.startTimer("stepTimer", std::chrono::seconds(60));
        setState(State::Activating);
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            setState(State::Active);
        }
        else if (state() == State::Active)
        {
            auto movement = ctx.getInput(movementInput.aliasName);
            if (movement)
            {
                auto val = movement->read();
                if (val.isType(ParameterType::BOOLEAN) && val.getBoolean())
                {
                    if (!movementDetected)
                    {
                        movementDetected = true;
                        ledActive = true;
                        
                        uint32_t color = readParamOrDefault(ledColorParam, uint32_t(0xFF00FFFF));
                        auto led = ctx.getOutput(ledOutput.aliasName);
                        if (led) {
                            led->write(ParameterValue::fromGenericInt(color));
                        }
                        
                        uint32_t durationMs = readParamOrDefault(durationParam, uint32_t(2000));
                        ctx.startTimer("ledTimer", std::chrono::milliseconds(durationMs));
                    }
                }
            }
            
            if (ledActive && ctx.isTimerExpired("ledTimer"))
            {
                ctx.stopTimer("ledTimer");
                auto led = ctx.getOutput(ledOutput.aliasName);
                if (led) {
                    led->write(ParameterValue::fromGenericInt(0x00000000));
                }
                ledActive = false;
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        auto led = ctx.getOutput(ledOutput.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    void onPauseImpl(StepContext &ctx) override
    {
        auto led = ctx.getOutput(ledOutput.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && ledActive) {
            uint32_t color = readParamOrDefault(ledColorParam, uint32_t(0xFF00FFFF));
            auto led = ctx.getOutput(ledOutput.aliasName);
            if (led) {
                led->write(ParameterValue::fromGenericInt(color));
            }
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active)
        {
            return ctx.isTimerExpired("stepTimer");
        }
        return false;
    }
};

// ==================== Test Step 4: Acknowledgement LED and Fan ====================
// LED leuchtet bis User Acknowledge drückt, dann läuft Fan für konfigurierbare Zeit
class AcknowledgeLedFanStep : public StepBase
{
private:
    IoAliasDef ledOutput;
    IoAliasDef fanOutput;
    ParamDef ledColorParam;
    ParamDef fanIntensityParam;
    ParamDef fanDurationParam;
    bool acknowledged;

public:
    AcknowledgeLedFanStep()
        : StepBase("AcknowledgeLedFan", "LED on until user acknowledges, then run fan at configured intensity", "1.0"),
          ledOutput("LED", false, true, false, "uint32_t", std::nullopt, "LED0"),
          fanOutput("Fan", false, true, false, "float", std::nullopt, "Fan"),
          ledColorParam("ledColor", 
                       ParameterValue::fromGenericInt(0xFFFFFFFF),
                       "LED Color",
                       "Color while waiting for acknowledgement (RGBA)",
                       "RGBA"),
          fanIntensityParam("fanIntensity", 
                           ParameterValue::fromPercentage(75.0f),
                           "Fan Intensity",
                           "Fan duty cycle after acknowledgement",
                           "%",
                           ParameterValue::fromPercentage(0.0f),
                           ParameterValue::fromPercentage(100.0f)),
          fanDurationParam("fanDuration", 
                          ParameterValue::fromTimeSeconds(5),
                          "Fan Duration",
                          "How long fan runs after acknowledgement",
                          "s",
                          ParameterValue::fromTimeSeconds(1),
                          ParameterValue::fromTimeSeconds(300)),
          acknowledged(false)
    {
        registerIoAliases({&ledOutput, &fanOutput});
        registerParamDefs({&ledColorParam, &fanIntensityParam, &fanDurationParam});
    }

    AcknowledgeLedFanStep(const AcknowledgeLedFanStep &o)
        : StepBase(o),
          ledOutput(o.ledOutput),
          fanOutput(o.fanOutput),
          ledColorParam(o.ledColorParam),
          fanIntensityParam(o.fanIntensityParam),
          fanDurationParam(o.fanDurationParam),
          acknowledged(false)
    {
        registerIoAliases({&ledOutput, &fanOutput});
        registerParamDefs({&ledColorParam, &fanIntensityParam, &fanDurationParam});
    }

    void initialize() override
    {
        setState(State::Inactive);
        acknowledged = false;
    }

    void onActivating(StepContext &ctx) override
    {
        acknowledged = false;
        
        uint32_t color = readParamOrDefault(ledColorParam, uint32_t(0xFFFFFFFF));
        auto led = ctx.getOutput(ledOutput.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(color));
        }
        
        ctx.requestUserAcknowledgment("Please confirm to proceed with fan operation");
        setState(State::Activating);
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            if (ctx.isAcknowledged())
            {
                acknowledged = true;
                
                auto led = ctx.getOutput(ledOutput.aliasName);
                if (led) {
                    led->write(ParameterValue::fromGenericInt(0x00000000));
                }
                
                float fanIntensity = readParamOrDefault(fanIntensityParam, 75.0f);
                auto fan = ctx.getOutput(fanOutput.aliasName);
                if (fan) {
                    fan->write(ParameterValue::fromPercentage(fanIntensity));
                }
                
                uint32_t durationMs = readParamOrDefault(fanDurationParam, uint32_t(5000));
                ctx.startTimer("fanTimer", std::chrono::milliseconds(durationMs));
                setState(State::Active);
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        auto led = ctx.getOutput(ledOutput.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
        
        auto fan = ctx.getOutput(fanOutput.aliasName);
        if (fan) {
            fan->write(ParameterValue::fromPercentage(0.0f));
        }
        
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    void onPauseImpl(StepContext &ctx) override
    {
        auto led = ctx.getOutput(ledOutput.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
        
        auto fan = ctx.getOutput(fanOutput.aliasName);
        if (fan) {
            fan->write(ParameterValue::fromPercentage(0.0f));
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused())
        {
            if (!acknowledged) {
                uint32_t color = readParamOrDefault(ledColorParam, uint32_t(0xFFFFFFFF));
                auto led = ctx.getOutput(ledOutput.aliasName);
                if (led) {
                    led->write(ParameterValue::fromGenericInt(color));
                }
            } else {
                float fanIntensity = readParamOrDefault(fanIntensityParam, 75.0f);
                auto fan = ctx.getOutput(fanOutput.aliasName);
                if (fan) {
                    fan->write(ParameterValue::fromPercentage(fanIntensity));
                }
            }
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active)
        {
            bool timerDone = ctx.isTimerExpired("fanTimer");
            if (timerDone) {
                ctx.stopTimer("fanTimer");
            }
            return timerDone;
        }
        return false;
    }
};
