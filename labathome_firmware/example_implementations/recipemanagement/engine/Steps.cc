#pragma once

#include "IStep.hh"
#include "StepContext.hh"
#include "../values_impl.cc"
#include <chrono>
#include <string_view>

class TwoButtonFanStep : public StepBase<TwoButtonFanStep> {
private:
    //eigene Werte als ParamDefs defineren
    ParamDef waitMs;
    ParamDef fanMs;
    ParamDef fanDuty;
    //eigene IO-Aliase als IoAliasDef definieren
    IoAliasDef green;
    IoAliasDef red;
    IoAliasDef fan;
public:
    // Konstruktor übergibt Metadaten an die Basis (StepBase)
    TwoButtonFanStep()
        : StepBase(0x0001, "TwoButtonFan", "Wait; if both buttons pressed run fan", "1.1"),
        //Parameter initialisieren
          waitMs(
              "waitMs",
              std::make_unique<ValueFloat>(5000.0f),
              "Wait Milliseconds",
              "Delay before checking buttons (ms)",
              std::optional<std::unique_ptr<IValue>>(std::make_unique<ValueFloat>(0.0f)),
              std::optional<std::unique_ptr<IValue>>(std::make_unique<ValueFloat>(60000.0f))
          ),
          fanMs(
              "fanMs",
              std::make_unique<ValueFloat>(10000.0f),
              "Fan Milliseconds",
              "Run fan duration (ms)",
              std::optional<std::unique_ptr<IValue>>(std::make_unique<ValueFloat>(0.0f)),
              std::optional<std::unique_ptr<IValue>>(std::make_unique<ValueFloat>(600000.0f))
          ),
          fanDuty(
              "fanDuty",
              std::make_unique<ValueFloat>(100.0f),
              "Fan Duty",
              "Percent duty for fan",
              std::optional<std::unique_ptr<IValue>>(std::make_unique<ValueFloat>(0.0f)),
              std::optional<std::unique_ptr<IValue>>(std::make_unique<ValueFloat>(100.0f))
          ),
          //IO-Aliase initialisieren
          green("GreenButton", true, false, true),
          red("RedButton", true, false, true),
          fan("Fan", false, true, false)
    {
        // Registriere Parameter und IO-Aliases damit die Logik auf Variablen zugreifen kann. 
        registerParamDefs({ &waitMs, &fanMs, &fanDuty});
        registerIoAliases({ &green, &red, &fan });
    }
    //Kopierkonstruktor, der alle Parameterwerte kopiert
    TwoButtonFanStep(const TwoButtonFanStep& o)
        : StepBase(o),
          waitMs(o.waitMs),
          fanMs(o.fanMs),
          fanDuty(o.fanDuty),
          green(o.green),
          red(o.red),
          fan(o.fan)
    {
        registerParamDefs({ &waitMs, &fanMs, &fanDuty });
        registerIoAliases({ &green, &red, &fan });
    }

    TwoButtonFanStep(const TwoButtonFanStep& o, bool clearParams)
        : TwoButtonFanStep(o)
    {
        if (clearParams) {
            clearAllParamValues();
        }
    }

    //Ab hier wird die Step-Logik implementiert

    void initialize() override {
        setState(State::Inactive);
    }

    void onActivating(StepContext& ctx) override {
        auto wait_duration = readParamOrDefault(waitMs, 5000.0f);
        ctx.startTimer("wait", std::chrono::milliseconds(static_cast<int64_t>(wait_duration)));
        setState(State::Activating);
        ctx.log("TwoButtonFan: activating");
    }

    void onActive(StepContext& ctx) override {
        if (state() == State::Activating) {
            if (!ctx.isTimerExpired("wait")) return;
            ctx.stopTimer("wait");

            auto inG = ctx.getInput(green.aliasName);
            auto inR = ctx.getInput(red.aliasName);

            bool g = false, r = false;
            if (inG) {
                auto v = inG->read();
                if (v && v->kind() == ValueKind::Bool) v->get<bool>(g);
            }
            if (inR) {
                auto v = inR->read();
                if (v && v->kind() == ValueKind::Bool) v->get<bool>(r);
            }

            if (g && r) {
                float duty = readParamOrDefault(fanDuty, 100.0f);
                auto out = ctx.getOutput(fan.aliasName);
                if (out) {
                    ValueFloat tmp(duty);
                    out->write(tmp);
                }
                auto fan_duration = readParamOrDefault(fanMs, 10000.0f);
                ctx.startTimer("fan", std::chrono::milliseconds(static_cast<int64_t>(fan_duration)));
                setState(State::Active);
                ctx.log("TwoButtonFan: fan started");
            } else {
                setState(State::Deactivating);
            }
        } else if (state() == State::Active) {
            if (ctx.isTimerExpired("fan")) {
                auto out = ctx.getOutput(fan.aliasName);
                if (out) {
                    ValueFloat tmp(0.0f);
                    out->write(tmp);
                }
                ctx.stopTimer("fan");
                setState(State::Deactivating);
                ctx.log("TwoButtonFan: fan finished");
            }
        }
    }

    void onDeactivating(StepContext& ctx) override {
        setState(State::Deactivated);
    }

    void onDeactivated(StepContext& /*ctx*/) override {}

    bool isTransitionConditionMet(StepContext& ctx) override {
        if (state() == State::Activating) return ctx.isTimerExpired("wait");
        if (state() == State::Active) return ctx.isTimerExpired("fan");
        return true;
    }


};