#pragma once

#include "esp_log.h"
#include "../../../recipemanagement/core/interfaces/engine/IStep.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepContext.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepMetadata.hh"
#include <chrono>
#include <string_view>

// ==================== Step 1: RedLedButtonStep ====================
// LED rot leuchten lassen, auf RedButton-Druck ausschalten, dann konfigurierbare Zeit warten
class RedLedButtonStep : public StepBase
{
private:
    IoAliasDef ledRed;
    IoAliasDef btnRed;
    ParamDef waitTimeParam;
    bool buttonWasPressed;

public:
    RedLedButtonStep()
        : StepBase("RedLedButton", "Turn LED red, wait for red button press, then wait configured time", "1.0"),
          ledRed("LED", false, true, false, "uint32_t"),
          btnRed("RedButton", true, false, true, "bool"),
          waitTimeParam("waitTime", 
                        ParameterValue::fromTimeMilliseconds(1000),
                        "Wait Time",
                        "Time to wait after button press",
                        "ms",
                        ParameterValue::fromTimeMilliseconds(100),
                        ParameterValue::fromTimeMilliseconds(10000)),
          buttonWasPressed(false)
    {
        registerIoAliases({&ledRed, &btnRed});
        registerParamDefs({&waitTimeParam});
    }

    RedLedButtonStep(const RedLedButtonStep &o)
        : StepBase(o),
          ledRed(o.ledRed),
          btnRed(o.btnRed),
          waitTimeParam(o.waitTimeParam),
          buttonWasPressed(false)
    {
        registerIoAliases({&ledRed, &btnRed});
        registerParamDefs({&waitTimeParam});
    }

    void initialize() override
    {
        setState(State::Inactive);
        buttonWasPressed = false;
    }

    void onActivating(StepContext &ctx) override
    {
        // LED rot leuchten lassen (0xFF0000FF = red in RGBA format)
        auto led = ctx.getOutput(ledRed.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0xFF0000FF)); // RGBA: Red
            ESP_LOGI("Step_0x0001", "LED set to RED, waiting for button press");
        } else {
            ESP_LOGE("Step_0x0001", "ERROR: LED output not found!");
        }
        setState(State::Activating);
        buttonWasPressed = false;
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            // Prüfe ob RedButton gedrückt wurde
            auto btn = ctx.getInput(btnRed.aliasName);
            if (btn)
            {
                auto val = btn->read();
                if (val.isType(ParameterType::BOOLEAN))
                {
                    if (val.getBoolean() && !buttonWasPressed)
                    {
                        buttonWasPressed = true;
                        
                        // LED ausschalten
                        auto led2 = ctx.getOutput(ledRed.aliasName);
                        if (led2) {
                            led2->write(ParameterValue::fromGenericInt(0x00000000));
                        }
                        
                        // Konfigurierbare Wartezeit aus Parameter lesen
                        uint32_t waitMs = readParamOrDefault(waitTimeParam, uint32_t(1000));
                        ctx.startTimer("wait", std::chrono::milliseconds(waitMs));
                        setState(State::Active);
                        ESP_LOGI("Step_0x0001", "Button pressed - LED off, waiting 1000ms");
                    }
                }
            }
        }
        else if (state() == State::Active)
        {
            // Warte auf Timer - DON'T stop it here, let transition condition check it
            if (ctx.isTimerExpired("wait"))
            {
                ESP_LOGI("Step_0x0001", "Timer expired - ready for transition");
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        // LED sicherheitshalber ausschalten
        auto led = ctx.getOutput(ledRed.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        // Transition nur wenn State::Active und Timer abgelaufen
        if (state() == State::Active)
        {
            bool timerDone = ctx.isTimerExpired("wait");
            if (timerDone) {
                ESP_LOGI("Step_0x0001", "==> Transition condition MET - cleaning up timer");
                ctx.stopTimer("wait");
            }
            return timerDone;
        }
        return false;
    }
};


// ==================== Step 2: YellowGreenLedButtonStep ====================
// LED gelb für konfigurierbare Zeit, dann auf Knopfdruck grün, dann auf erneutem Knopfdruck fertig
class YellowGreenLedButtonStep : public StepBase
{
private:
    IoAliasDef ledYellow;
    IoAliasDef btnGreen;
    ParamDef yellowTimeParam;
    bool firstPressDetected;
    bool secondPressDetected;
    bool wasPressed; // Debounce helper

public:
    YellowGreenLedButtonStep()
        : StepBase("YellowGreenLedButton", "LED yellow for configured time, green on button, off on second button", "1.0"),
          ledYellow("LED", false, true, false, "uint32_t"),
          btnGreen("GreenButton", true, false, true, "bool"),
          yellowTimeParam("yellowTime",
                          ParameterValue::fromTimeMilliseconds(2000),
                          "Yellow Duration",
                          "Time LED stays yellow",
                          "ms",
                          ParameterValue::fromTimeMilliseconds(500),
                          ParameterValue::fromTimeMilliseconds(10000)),
          firstPressDetected(false),
          secondPressDetected(false),
          wasPressed(false)
    {
        registerIoAliases({&ledYellow, &btnGreen});
        registerParamDefs({&yellowTimeParam});
    }

    YellowGreenLedButtonStep(const YellowGreenLedButtonStep &o)
        : StepBase(o),
          ledYellow(o.ledYellow),
          btnGreen(o.btnGreen),
          yellowTimeParam(o.yellowTimeParam),
          firstPressDetected(false),
          secondPressDetected(false),
          wasPressed(false)
    {
        registerIoAliases({&ledYellow, &btnGreen});
        registerParamDefs({&yellowTimeParam});
    }

    void initialize() override
    {
        setState(State::Inactive);
        firstPressDetected = false;
        secondPressDetected = false;
        wasPressed = false;
    }

    void onActivating(StepContext &ctx) override
    {
        // LED gelb leuchten lassen (0xFFFF00FF = yellow in RGBA format)
        auto led = ctx.getOutput(ledYellow.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0xFFFF00FF)); // RGBA: Yellow
        }
        
        // Konfigurierbare Timer-Dauer aus Parameter lesen
        uint32_t yellowMs = readParamOrDefault(yellowTimeParam, uint32_t(2000));
        ctx.startTimer("yellow", std::chrono::milliseconds(yellowMs));
        setState(State::Activating);
        firstPressDetected = false;
        secondPressDetected = false;
        wasPressed = false;
        ESP_LOGI("Step_0x0002", "LED set to YELLOW for 2s");
    }

    void onActive(StepContext &ctx) override
    {
        auto btn = ctx.getInput(btnGreen.aliasName);
        bool currentlyPressed = false;
        
        if (btn)
        {
            auto val = btn->read();
            if (val.isType(ParameterType::BOOLEAN))
            {
                currentlyPressed = val.getBoolean();
            }
        }

        if (state() == State::Activating)
        {
            // Warte auf Timer-Ablauf
            if (ctx.isTimerExpired("yellow"))
            {
                ctx.stopTimer("yellow");
                setState(State::Active);
                ESP_LOGI("Step_0x0002", "2s elapsed - waiting for button press");
            }
        }
        else if (state() == State::Active)
        {
            if (!firstPressDetected)
            {
                // Warte auf ersten Button-Druck (rising edge)
                if (currentlyPressed && !wasPressed)
                {
                    firstPressDetected = true;
                    
                    // LED auf grün schalten (0x00FF00FF = green in RGBA format)
                    auto led = ctx.getOutput(ledYellow.aliasName);
                    if (led) {
                        led->write(ParameterValue::fromGenericInt(0x00FF00FF)); // RGBA: Green
                    }
                    ESP_LOGI("Step_0x0002", "First press - LED now GREEN");
                }
            }
            else if (!secondPressDetected)
            {
                // Warte auf zweiten Button-Druck (rising edge)
                if (currentlyPressed && !wasPressed)
                {
                    secondPressDetected = true;
                    
                    // LED ausschalten
                    auto led = ctx.getOutput(ledYellow.aliasName);
                    if (led) {
                        led->write(ParameterValue::fromGenericInt(0x00000000));
                    }
                    ESP_LOGI("Step_0x0002", "Second press - LED off, step complete");
                }
            }
        }
        
        wasPressed = currentlyPressed;
    }

    void onDeactivating(StepContext &ctx) override
    {
        // LED sicherheitshalber ausschalten
        auto led = ctx.getOutput(ledYellow.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active)
        {
            // Fertig wenn beide Buttons gedrückt wurden
            bool done = firstPressDetected && secondPressDetected;
            if (done) {
                ESP_LOGI("Step_0x0002", "Transition condition met - both presses detected");
            }
            return done;
        }
        return false;
    }
};


// ==================== Step 3: TwoLedTwoButtonStep ====================
// 2 LEDs (rot + grün) leuchten, beide Buttons gleichzeitig → LEDs aus, dann fertig
class TwoLedTwoButtonStep : public StepBase
{
private:
    IoAliasDef ledRed;
    IoAliasDef ledGreen;
    IoAliasDef btnRed;
    IoAliasDef btnGreen;

public:
    TwoLedTwoButtonStep()
        : StepBase("TwoLedTwoButton", "Red+Green LEDs on, both buttons pressed to finish", "1.0"),
          ledRed("LEDRed", false, true, false, "uint32_t"),
          ledGreen("LEDGreen", false, true, false, "uint32_t"),
          btnRed("RedButton", true, false, true, "bool"),
          btnGreen("GreenButton", true, false, true, "bool")
    {
        registerIoAliases({&ledRed, &ledGreen, &btnRed, &btnGreen});
    }

    TwoLedTwoButtonStep(const TwoLedTwoButtonStep &o)
        : StepBase(o),
          ledRed(o.ledRed),
          ledGreen(o.ledGreen),
          btnRed(o.btnRed),
          btnGreen(o.btnGreen)
    {
        registerIoAliases({&ledRed, &ledGreen, &btnRed, &btnGreen});
    }

    void initialize() override
    {
        setState(State::Inactive);
    }

    void onActivating(StepContext &ctx) override
    {
        // Beide LEDs einschalten
        auto ledR = ctx.getOutput(ledRed.aliasName);
        auto ledG = ctx.getOutput(ledGreen.aliasName);
        
        if (ledR) {
            ledR->write(ParameterValue::fromGenericInt(0xFF0000FF)); // RGBA: Red
        }
        if (ledG) {
            ledG->write(ParameterValue::fromGenericInt(0x00FF00FF)); // RGBA: Green
        }
        
        setState(State::Activating);
        ESP_LOGI("Step_0x0003", "Both LEDs on - waiting for both buttons");
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            // Prüfe ob beide Buttons gleichzeitig gedrückt sind
            auto btnR = ctx.getInput(btnRed.aliasName);
            auto btnG = ctx.getInput(btnGreen.aliasName);
            
            bool redPressed = false, greenPressed = false;
            
            if (btnR)
            {
                auto val = btnR->read();
                if (val.isType(ParameterType::BOOLEAN))
                {
                    redPressed = val.getBoolean();
                }
            }
            
            if (btnG)
            {
                auto val = btnG->read();
                if (val.isType(ParameterType::BOOLEAN))
                {
                    greenPressed = val.getBoolean();
                }
            }
            
            if (redPressed && greenPressed)
            {
                // Beide LEDs ausschalten
                auto ledR = ctx.getOutput(ledRed.aliasName);
                auto ledG = ctx.getOutput(ledGreen.aliasName);
                
                if (ledR) {
                    ledR->write(ParameterValue::fromGenericInt(0x00000000));
                }
                if (ledG) {
                    ledG->write(ParameterValue::fromGenericInt(0x00000000));
                }
                
                setState(State::Active);
                ESP_LOGI("Step_0x0003", "Both buttons pressed - LEDs off");
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        // LEDs sicherheitshalber ausschalten
        auto ledR = ctx.getOutput(ledRed.aliasName);
        auto ledG = ctx.getOutput(ledGreen.aliasName);
        
        if (ledR) {
            ledR->write(ParameterValue::fromGenericInt(0x00000000));
        }
        if (ledG) {
            ledG->write(ParameterValue::fromGenericInt(0x00000000));
        }
        
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active)
        {
            // Sofort fertig nach dem beide Buttons gedrückt wurden
            ESP_LOGI("Step_0x0003", "Transition condition met - step complete");
            return true;
        }
        return false;
    }
};
