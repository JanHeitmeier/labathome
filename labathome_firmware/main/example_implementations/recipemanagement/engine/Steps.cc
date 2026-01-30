#pragma once

#include "esp_log.h"
#include "../../../recipemanagement/core/interfaces/engine/IStep.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepContext.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepMetadata.hh"
#include <chrono>

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
          ledRed("LED", false, true, false, "uint32_t", std::nullopt, "LED0"),
          btnRed("RedButton", true, false, true, "bool", std::nullopt, "RedButton"),
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

    void onPauseImpl(StepContext &ctx) override
    {
        auto led = ctx.getOutput(ledRed.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
            ESP_LOGI("Step_0x0001", "LED turned off during pause");
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && state() == State::Activating) {
            auto led = ctx.getOutput(ledRed.aliasName);
            if (led) {
                led->write(ParameterValue::fromGenericInt(0xFF0000FF));
                ESP_LOGI("Step_0x0001", "LED restored to red after resume");
            }
        }
    }

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
    bool wasPressed;
    uint32_t ledColorBeforePause;

public:
    YellowGreenLedButtonStep()
        : StepBase("YellowGreenLedButton", "LED yellow for configured time, green on button, off on second button", "1.0"),
          ledYellow("LED", false, true, false, "uint32_t", std::nullopt, "LED0"),
          btnGreen("GreenButton", true, false, true, "bool", std::nullopt, "GreenButton"),
          yellowTimeParam("yellowTime",
                          ParameterValue::fromTimeMilliseconds(2000),
                          "Yellow Duration",
                          "Time LED stays yellow",
                          "ms",
                          ParameterValue::fromTimeMilliseconds(500),
                          ParameterValue::fromTimeMilliseconds(10000)),
          firstPressDetected(false),
          secondPressDetected(false),
          wasPressed(false),
          ledColorBeforePause(0)
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
          wasPressed(false),
          ledColorBeforePause(0)
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
        ledColorBeforePause = 0;
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

    void onPauseImpl(StepContext &ctx) override
    {
        if (state() == State::Activating && !ctx.isTimerExpired("yellow")) {
            ledColorBeforePause = 0xFFFF00FF;
        } else if (state() == State::Active && firstPressDetected && !secondPressDetected) {
            ledColorBeforePause = 0x00FF00FF;
        } else {
            ledColorBeforePause = 0;
        }
        
        auto led = ctx.getOutput(ledYellow.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
            ESP_LOGI("Step_0x0002", "LED turned off during pause");
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && ledColorBeforePause != 0) {
            auto led = ctx.getOutput(ledYellow.aliasName);
            if (led) {
                led->write(ParameterValue::fromGenericInt(ledColorBeforePause));
                ESP_LOGI("Step_0x0002", "LED color restored after resume");
            }
        }
        ledColorBeforePause = 0;
    }

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
          ledRed("LEDRed", false, true, false, "uint32_t", std::nullopt, "LED0"),
          ledGreen("LEDGreen", false, true, false, "uint32_t", std::nullopt, "LED1"),
          btnRed("RedButton", true, false, true, "bool", std::nullopt, "RedButton"),
          btnGreen("GreenButton", true, false, true, "bool", std::nullopt, "GreenButton")
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

    void onPauseImpl(StepContext &ctx) override
    {
        auto ledR = ctx.getOutput(ledRed.aliasName);
        auto ledG = ctx.getOutput(ledGreen.aliasName);
        if (ledR) ledR->write(ParameterValue::fromGenericInt(0x00000000));
        if (ledG) ledG->write(ParameterValue::fromGenericInt(0x00000000));
        ESP_LOGI("Step_0x0003", "LEDs turned off during pause");
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && state() == State::Activating) {
            auto ledR = ctx.getOutput(ledRed.aliasName);
            auto ledG = ctx.getOutput(ledGreen.aliasName);
            if (ledR) ledR->write(ParameterValue::fromGenericInt(0xFF0000FF));
            if (ledG) ledG->write(ParameterValue::fromGenericInt(0x00FF00FF));
            ESP_LOGI("Step_0x0003", "LEDs restored after resume");
        }
    }

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


// ==================== Step 4: FanControlStep ====================
// Run fan at 50% for 5 seconds, then set to 0%
class FanControlStep : public StepBase
{
private:
    IoAliasDef fanOutput;
    IoAliasDef fanSensor;
    
public:
    FanControlStep()
        : StepBase("FanControl", "Run fan at 50% for 5 seconds", "1.0"),
          fanOutput("Fan", false, true, false, "float", std::nullopt, "Fan"),
          fanSensor("FanSpeed", true, false, true, "float", std::nullopt, "Fan")
    {
        registerIoAliases({&fanOutput, &fanSensor});
    }

    FanControlStep(const FanControlStep &o)
        : StepBase(o),
          fanOutput(o.fanOutput),
          fanSensor(o.fanSensor)
    {
        registerIoAliases({&fanOutput, &fanSensor});
    }

    void initialize() override
    {
        setState(State::Inactive);
    }

    void onActivating(StepContext &ctx) override
    {
        auto fan = ctx.getOutput(fanOutput.aliasName);
        if (fan) {
            fan->write(ParameterValue::fromPercentage(50.0f));
            ESP_LOGI("FanControl", "Fan set to 50%%");
        }
        ctx.startTimer("fan_duration", std::chrono::milliseconds(5000));
        setState(State::Active);
    }

    void onActive(StepContext &ctx) override
    {
        if (ctx.isTimerExpired("fan_duration"))
        {
            auto fan = ctx.getOutput(fanOutput.aliasName);
            if (fan) {
                fan->write(ParameterValue::fromPercentage(0.0f));
                ESP_LOGI("FanControl", "Fan set to 0%% - step complete");
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
            ESP_LOGI("FanControl", "Fan paused (set to 0%%)");
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && state() == State::Active && !ctx.isTimerExpired("fan_duration")) {
            auto fan = ctx.getOutput(fanOutput.aliasName);
            if (fan) {
                fan->write(ParameterValue::fromPercentage(50.0f));
                ESP_LOGI("FanControl", "Fan resumed (set to 50%%)");
            }
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active && ctx.isTimerExpired("fan_duration"))
        {
            ctx.stopTimer("fan_duration");
            return true;
        }
        return false;
    }
};


// ==================== Step 5: InstructionConfirmationStep ====================
// Request user confirmation, turn LED red, request confirmation to turn off
class InstructionConfirmationStep : public StepBase
{
private:
    enum class Phase {
        WaitingFirstAck,    // Waiting for first acknowledgment
        LedOn,              // LED is on, waiting for second acknowledgment
        Completed           // Both acknowledgments done
    };
    
    IoAliasDef ledRed;
    Phase currentPhase;
    
public:
    InstructionConfirmationStep()
        : StepBase("InstructionConfirmation", "User confirmation with LED control", "1.0"),
          ledRed("LED", false, true, false, "uint32_t", std::nullopt, "LED0"),
          currentPhase(Phase::WaitingFirstAck)
    {
        registerIoAliases({&ledRed});
    }

    InstructionConfirmationStep(const InstructionConfirmationStep &o)
        : StepBase(o),
          ledRed(o.ledRed),
          currentPhase(Phase::WaitingFirstAck)
    {
        registerIoAliases({&ledRed});
    }

    void initialize() override
    {
        setState(State::Inactive);
        currentPhase = Phase::WaitingFirstAck;
    }

    void onActivating(StepContext &ctx) override
    {
        currentPhase = Phase::WaitingFirstAck;
        ctx.requestUserAcknowledgment("Please confirm to turn on the red LED");
        setState(State::Active);
        ESP_LOGI("InstructionConfirm", "Awaiting first confirmation");
    }

    void onActive(StepContext &ctx) override
    {
        if (currentPhase == Phase::WaitingFirstAck)
        {
            if (ctx.isAcknowledged())
            {
                auto led = ctx.getOutput(ledRed.aliasName);
                if (led) {
                    led->write(ParameterValue::fromGenericInt(0xFF0000FF));
                    ESP_LOGI("InstructionConfirm", "LED turned red - awaiting second confirmation");
                }
                ctx.requestUserAcknowledgment("LED is now red. Confirm to turn it off");
                currentPhase = Phase::LedOn;
            }
        }
        else if (currentPhase == Phase::LedOn)
        {
            if (ctx.isAcknowledged())
            {
                auto led = ctx.getOutput(ledRed.aliasName);
                if (led) {
                    led->write(ParameterValue::fromGenericInt(0x00000000));
                    ESP_LOGI("InstructionConfirm", "LED turned off - step complete");
                }
                currentPhase = Phase::Completed;
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        auto led = ctx.getOutput(ledRed.aliasName);
        if (led) {
            led->write(ParameterValue::fromGenericInt(0x00000000));
        }
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    void onPauseImpl(StepContext &ctx) override
    {
        if (currentPhase == Phase::LedOn) {
            auto led = ctx.getOutput(ledRed.aliasName);
            if (led) {
                led->write(ParameterValue::fromGenericInt(0x00000000));
                ESP_LOGI("InstructionConfirm", "LED turned off during pause");
            }
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && currentPhase == Phase::LedOn) {
            auto led = ctx.getOutput(ledRed.aliasName);
            if (led) {
                led->write(ParameterValue::fromGenericInt(0xFF0000FF));
                ESP_LOGI("InstructionConfirm", "LED restored after resume");
            }
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (currentPhase == Phase::Completed)
        {
            ESP_LOGI("InstructionConfirm", "Transition condition met - both acknowledgments received");
            return true;
        }
        return false;
    }
};


// ==================== Step 6: FanCoolingTimed ====================
// Request acknowledgment, then run fan at configured duty for configured time
class FanCoolingTimed : public StepBase
{
private:
    IoAliasDef fanOutput;
    ParamDef fanDutyParam;
    ParamDef durationParam;
    
public:
    FanCoolingTimed()
        : StepBase("FanCoolingTimed", "Run fan at configured duty for configured time", "1.0"),
          fanOutput("Fan", false, true, false, "float", std::nullopt, "Fan"),
          fanDutyParam("fanDuty",
                       ParameterValue::fromPercentage(50.0f),
                       "Fan Duty",
                       "Fan duty cycle in percent",
                       "%",
                       ParameterValue::fromPercentage(10.0f),
                       ParameterValue::fromPercentage(100.0f)),
          durationParam("duration",
                        ParameterValue::fromTimeMilliseconds(5000),
                        "Duration",
                        "Fan runtime duration",
                        "ms",
                        ParameterValue::fromTimeMilliseconds(1000),
                        ParameterValue::fromTimeMilliseconds(60000))
    {
        registerIoAliases({&fanOutput});
        registerParamDefs({&fanDutyParam, &durationParam});
    }

    FanCoolingTimed(const FanCoolingTimed &o)
        : StepBase(o),
          fanOutput(o.fanOutput),
          fanDutyParam(o.fanDutyParam),
          durationParam(o.durationParam)
    {
        registerIoAliases({&fanOutput});
        registerParamDefs({&fanDutyParam, &durationParam});
    }

    void initialize() override
    {
        setState(State::Inactive);
    }

    void onActivating(StepContext &ctx) override
    {
        ctx.requestUserAcknowledgment("Acknowledge that Fan starts !");
        setState(State::Activating);
        ESP_LOGI("FanCoolingTimed", "Awaiting acknowledgment to start fan");
    }

    void onActive(StepContext &ctx) override
    {
        if (state() == State::Activating)
        {
            if (ctx.isAcknowledged())
            {
                float duty = readParamOrDefault(fanDutyParam, 50.0f);
                uint32_t durationMs = readParamOrDefault(durationParam, uint32_t(5000));
                
                auto fan = ctx.getOutput(fanOutput.aliasName);
                if (fan) {
                    fan->write(ParameterValue::fromPercentage(duty));
                    ESP_LOGI("FanCoolingTimed", "Fan set to %.1f%% for %lu ms", duty, (unsigned long)durationMs);
                }
                
                ctx.startTimer("fan_duration", std::chrono::milliseconds(durationMs));
                setState(State::Active);
            }
        }
        else if (state() == State::Active)
        {
            if (ctx.isTimerExpired("fan_duration"))
            {
                auto fan = ctx.getOutput(fanOutput.aliasName);
                if (fan) {
                    fan->write(ParameterValue::fromPercentage(0.0f));
                    ESP_LOGI("FanCoolingTimed", "Fan stopped - step complete");
                }
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
        if (state() == State::Active) {
            auto fan = ctx.getOutput(fanOutput.aliasName);
            if (fan) {
                fan->write(ParameterValue::fromPercentage(0.0f));
                ESP_LOGI("FanCoolingTimed", "Fan paused (set to 0%%)");
            }
        }
    }

    void onResumeImpl(StepContext &ctx) override
    {
        if (wasPaused() && state() == State::Active && !ctx.isTimerExpired("fan_duration")) {
            float duty = readParamOrDefault(fanDutyParam, 50.0f);
            auto fan = ctx.getOutput(fanOutput.aliasName);
            if (fan) {
                fan->write(ParameterValue::fromPercentage(duty));
                ESP_LOGI("FanCoolingTimed", "Fan resumed (set to %.1f%%)", duty);
            }
        }
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        if (state() == State::Active && ctx.isTimerExpired("fan_duration"))
        {
            ctx.stopTimer("fan_duration");
            return true;
        }
        return false;
    }
};


// ==================== Step 7: SensorMovementTest ====================
// Monitor movement sensor until user acknowledges
class SensorMovementTest : public StepBase
{
private:
    IoAliasDef movementSensor;
    
public:
    SensorMovementTest()
        : StepBase("SensorMovementTest", "Monitor movement sensor until acknowledged", "1.0"),
          movementSensor("Movement", true, false, true, "bool", std::nullopt, "Movement")
    {
        registerIoAliases({&movementSensor});
    }

    SensorMovementTest(const SensorMovementTest &o)
        : StepBase(o),
          movementSensor(o.movementSensor)
    {
        registerIoAliases({&movementSensor});
    }

    void initialize() override
    {
        setState(State::Inactive);
    }

    void onActivating(StepContext &ctx) override
    {
        ctx.requestUserAcknowledgment("Monitor movement sensor - acknowledge when done");
        setState(State::Active);
        ESP_LOGI("SensorMovementTest", "Movement sensor monitoring started");
    }

    void onActive(StepContext &ctx) override
    {
        // Read movement sensor continuously
        auto sensor = ctx.getInput(movementSensor.aliasName);
        if (sensor) {
            auto val = sensor->read();
            if (val.isType(ParameterType::BOOLEAN)) {
                bool movement = val.getBoolean();
                ESP_LOGI("SensorMovementTest", "MOV %d", movement ? 1 : 0);
            }
        }
    }

    void onDeactivating(StepContext &ctx) override
    {
        setState(State::Deactivated);
        ESP_LOGI("SensorMovementTest", "Movement sensor monitoring stopped");
    }

    void onDeactivated(StepContext & /*ctx*/) override {}

    void onPauseImpl(StepContext &ctx) override
    {
        ESP_LOGI("SensorMovementTest", "Monitoring paused");
    }

    void onResumeImpl(StepContext &ctx) override
    {
        ESP_LOGI("SensorMovementTest", "Monitoring resumed");
    }

    bool isTransitionConditionMet(StepContext &ctx) override
    {
        // Step is done when user acknowledges
        if (state() == State::Active && ctx.isAcknowledged())
        {
            ESP_LOGI("SensorMovementTest", "Acknowledged - transition condition met");
            return true;
        }
        return false;
    }
};
