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
        : StepBase(0x0001, "RedLedButton", "Turn LED red, wait for red button press, then wait configured time", "1.0"),
          ledRed("LED", false, true, false, "uint32_t"),
          btnRed("RedButton", true, false, true, "bool"),
          waitTimeParam("waitTime", 
                        ParameterValue(uint32_t(1000)),
                        "Wait Time",
                        "Time to wait after button press",
                        "ms",
                        ParameterValue(uint32_t(100)),
                        ParameterValue(uint32_t(10000))),
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
        ESP_LOGE("RedLedButtonStep", "CHECKPOINT S1: onActivating called");
        // LED rot leuchten lassen (0xFF0000FF = red in RGBA format)
        auto led = ctx.getOutput(ledRed.aliasName);
        ESP_LOGE("RedLedButtonStep", "CHECKPOINT S2: Got LED output");
        if (led) {
            ESP_LOGE("RedLedButtonStep", "CHECKPOINT S3: Writing red color to LED");
            led->write(uint32_t(0xFF0000FF)); // RGBA: Red
            ESP_LOGE("RedLedButtonStep", "CHECKPOINT S4: Write completed");
        } else {
            ESP_LOGE("RedLedButtonStep", "ERROR S5: LED output is null!");
        }
        setState(State::Activating);
        buttonWasPressed = false;
        ctx.log("RedLedButton: LED is red, waiting for button press");
        ESP_LOGE("RedLedButtonStep", "CHECKPOINT S6: onActivating completed");
    }

    void onActive(StepContext &ctx) override
    {
        ESP_LOGE("RedLedButtonStep", "CHECKPOINT S10: onActive called");
        if (state() == State::Activating)
        {
            ESP_LOGE("RedLedButtonStep", "CHECKPOINT S11: State is Activating");
            
            // Test: LED einschalten
            ESP_LOGE("RedLedButtonStep", "CHECKPOINT S11a: Getting LED output");
            auto led = ctx.getOutput(ledRed.aliasName);
            if (led) {
                ESP_LOGE("RedLedButtonStep", "CHECKPOINT S11b: LED output found, writing red");
                led->write(uint32_t(0xFF0000FF)); // RGBA: Red
                ESP_LOGE("RedLedButtonStep", "CHECKPOINT S11c: LED write completed");
            } else {
                ESP_LOGE("RedLedButtonStep", "ERROR S11d: LED output is null");
            }
            
            // Prüfe ob RedButton gedrückt wurde
            auto btn = ctx.getInput(btnRed.aliasName);
            if (btn)
            {
                auto val = btn->read();
                if (auto *pBool = std::get_if<bool>(&val))
                {
                    if (*pBool && !buttonWasPressed)
                    {
                        buttonWasPressed = true;
                        
                        // LED ausschalten
                        auto led2 = ctx.getOutput(ledRed.aliasName);
                        if (led2) {
                            led2->write(uint32_t(0x000000));
                        }
                        
                        // Konfigurierbare Wartezeit aus Parameter lesen
                        uint32_t waitMs = readParamOrDefault(waitTimeParam, uint32_t(1000));
                        ctx.startTimer("wait", std::chrono::milliseconds(waitMs));
                        setState(State::Active);
                        ctx.log("RedLedButton: Button pressed, LED off, waiting");
                    }
                }
            }
        }
        else if (state() == State::Active)
        {
            ESP_LOGE("RedLedButtonStep", "CHECKPOINT S12: State is Active");
            // Warte auf Timer
            if (ctx.isTimerExpired("wait"))
            {
                ctx.stopTimer("wait");
                ctx.log("RedLedButton: Step complete");
            }
        }
        else
        {
            ESP_LOGE("RedLedButtonStep", "ERROR S13: State is neither Activating nor Active");
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        // LED sicherheitshalber ausschalten
        auto led = ctx.getOutput(ledRed.aliasName);
        if (led) {
            led->write(uint32_t(0x00000000));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            // Warte auf Button-Druck
            auto btn = ctx.getInput(btnRed.aliasName);
            if (btn)
            {
                auto val = btn->read();
                if (auto *pBool = std::get_if<bool>(&val))
                {
                    return *pBool;
                }
            }
            return false;
        }
        if (state() == State::Active)
        {
            // Warte auf Timer
            return ctx.isTimerExpired("wait");
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
        : StepBase(0x0002, "YellowGreenLedButton", "LED yellow for configured time, green on button, off on second button", "1.0"),
          ledYellow("LED", false, true, false, "uint32_t"),
          btnGreen("GreenButton", true, false, true, "bool"),
          yellowTimeParam("yellowTime",
                          ParameterValue(uint32_t(2000)),
                          "Yellow Duration",
                          "Time LED stays yellow",
                          "ms",
                          ParameterValue(uint32_t(500)),
                          ParameterValue(uint32_t(10000))),
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
            led->write(uint32_t(0xFFFF00FF)); // RGBA: Yellow
        }
        
        // Konfigurierbare Timer-Dauer aus Parameter lesen
        uint32_t yellowMs = readParamOrDefault(yellowTimeParam, uint32_t(2000));
        ctx.startTimer("yellow", std::chrono::milliseconds(yellowMs));
        setState(State::Activating);
        firstPressDetected = false;
        secondPressDetected = false;
        wasPressed = false;
        ctx.log("YellowGreenLedButton: LED yellow, waiting");
    }

    void onActive(StepContext &ctx) override
    {
        auto btn = ctx.getInput(btnGreen.aliasName);
        bool currentlyPressed = false;
        
        if (btn)
        {
            auto val = btn->read();
            if (auto *pBool = std::get_if<bool>(&val))
            {
                currentlyPressed = *pBool;
            }
        }

        if (state() == State::Activating)
        {
            // Warte auf Timer-Ablauf
            if (ctx.isTimerExpired("yellow"))
            {
                ctx.stopTimer("yellow");
                setState(State::Active);
                ctx.log("YellowGreenLedButton: 2s elapsed, waiting for first button press");
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
                        led->write(uint32_t(0x00FF00FF)); // RGBA: Green
                    }
                    ctx.log("YellowGreenLedButton: First press - LED green, waiting for second press");
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
                        led->write(uint32_t(0x00000000));
                    }
                    ctx.log("YellowGreenLedButton: Second press - Step complete");
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
            led->write(uint32_t(0x00000000));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            // Warte auf 2s Timer
            return ctx.isTimerExpired("yellow");
        }
        if (state() == State::Active)
        {
            // Fertig wenn beide Buttons gedrückt wurden
            return firstPressDetected && secondPressDetected;
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
        : StepBase(0x0003, "TwoLedTwoButton", "Red+Green LEDs on, both buttons pressed to finish", "1.0"),
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
            ledR->write(uint32_t(0x0000FF)); // GRB: Red
        }
        if (ledG) {
            ledG->write(uint32_t(0xFF0000)); // GRB: Green
        }
        
        setState(State::Activating);
        ctx.log("TwoLedTwoButton: Red+Green LEDs on, waiting for both buttons");
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
                if (auto *pBool = std::get_if<bool>(&val))
                {
                    redPressed = *pBool;
                }
            }
            
            if (btnG)
            {
                auto val = btnG->read();
                if (auto *pBool = std::get_if<bool>(&val))
                {
                    greenPressed = *pBool;
                }
            }
            
            if (redPressed && greenPressed)
            {
                // Beide LEDs ausschalten
                auto ledR = ctx.getOutput(ledRed.aliasName);
                auto ledG = ctx.getOutput(ledGreen.aliasName);
                
                if (ledR) {
                    ledR->write(uint32_t(0x000000));
                }
                if (ledG) {
                    ledG->write(uint32_t(0x000000));
                }
                
                setState(State::Active);
                ctx.log("TwoLedTwoButton: Both buttons pressed - Step complete");
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        // LEDs sicherheitshalber ausschalten
        auto ledR = ctx.getOutput(ledRed.aliasName);
        auto ledG = ctx.getOutput(ledGreen.aliasName);
        
        if (ledR) {
            ledR->write(uint32_t(0x000000));
        }
        if (ledG) {
            ledG->write(uint32_t(0x000000));
        }
        
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext &ctx) override
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
                if (auto *pBool = std::get_if<bool>(&val))
                {
                    redPressed = *pBool;
                }
            }
            
            if (btnG)
            {
                auto val = btnG->read();
                if (auto *pBool = std::get_if<bool>(&val))
                {
                    greenPressed = *pBool;
                }
            }
            
            return redPressed && greenPressed;
        }
        if (state() == State::Active)
        {
            // Sofort fertig nach dem beide Buttons gedrückt wurden
            return true;
        }
        return false;
    }
};
