#pragma once

#include "IStep.hh"
#include "StepContext.hh"
#include "../values_impl.cc"
#include <chrono>
#include <string_view>

// TwoButtonFanStep: Metadaten werden jetzt an StepBase übergeben.
// Entferne die lokalen Metadatenfelder und die lokalen Getter.
class TwoButtonFanStep : public StepBase<TwoButtonFanStep> {
public:
    // Konstruktor übergibt Metadaten an die Basis (StepBase)
    TwoButtonFanStep()
        : StepBase(0x0001, "TwoButtonFan", "Wait; if both buttons pressed run fan", "1.1"),
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
          green("GreenButton", true, false, true),
          red("RedButton", true, false, true),
          fan("Fan", false, true, false)
    {
        // Registriere Parameter und IO-Aliases
        registerParamDefs({ &waitMs, &fanMs, &fanDuty});
        registerIoAliases({ &green, &red, &fan });
    }

    // Copy-Ctor: Basis-Teil wird kopiert via StepBase(o)
    TwoButtonFanStep(const TwoButtonFanStep& o)
        : StepBase(o),
          waitMs(
              o.waitMs.key,
              o.waitMs.value ? o.waitMs.value->clone() : nullptr,
              o.waitMs.label,
              o.waitMs.description,
              cloneOptionalValuePtr(o.waitMs.minValue),
              cloneOptionalValuePtr(o.waitMs.maxValue)
          ),
          fanMs(
              o.fanMs.key,
              o.fanMs.value ? o.fanMs.value->clone() : nullptr,
              o.fanMs.label,
              o.fanMs.description,
              cloneOptionalValuePtr(o.fanMs.minValue),
              cloneOptionalValuePtr(o.fanMs.maxValue)
          ),
          fanDuty(
              o.fanDuty.key,
              o.fanDuty.value ? o.fanDuty.value->clone() : nullptr,
              o.fanDuty.label,
              o.fanDuty.description,
              cloneOptionalValuePtr(o.fanDuty.minValue),
              cloneOptionalValuePtr(o.fanDuty.maxValue)
          ),
          green(o.green.aliasName, o.green.isInput, o.green.isOutput, o.green.isSensor),
          red(o.red.aliasName, o.red.isInput, o.red.isOutput, o.red.isSensor),
          fan(o.fan.aliasName, o.fan.isInput, o.fan.isOutput, o.fan.isSensor)
    {
        registerParamDefs({ &waitMs, &fanMs, &fanDuty,});
        registerIoAliases({ &green, &red, &fan });
    }

    // Copy-Ctor mit Option, Parameter zu löschen (wie vorher)
    TwoButtonFanStep(const TwoButtonFanStep& o, bool clearParams)
        : TwoButtonFanStep(o)
    {
        if (clearParams) {
            waitMs.value = nullptr;
            fanMs.value = nullptr;
            fanDuty.value = nullptr;
            waitMs.minValue = std::nullopt;
            waitMs.maxValue = std::nullopt;
            fanMs.minValue = std::nullopt;
            fanMs.maxValue = std::nullopt;
            fanDuty.minValue = std::nullopt;
            fanDuty.maxValue = std::nullopt;
        }
    }

    // Delegieren an StepBase-Implementierungen
    StepMetadata getMetadata() const noexcept override {
        return StepBase::getMetadata();
    }

    ParamPtrList getParamDefPointers() noexcept override {
        return StepBase::getParamDefPointers();
    }

    AliasPtrList getIoAliasPointers() noexcept override {
        return StepBase::getIoAliasPointers();
    }

    std::optional<std::unique_ptr<IValue>> getParamValue(std::string_view key) const override {
        if (key.empty()) return std::nullopt;
        if (key == waitMs.key && waitMs.value) return std::optional<std::unique_ptr<IValue>>(waitMs.value->clone());
        if (key == fanMs.key && fanMs.value) return std::optional<std::unique_ptr<IValue>>(fanMs.value->clone());
        if (key == fanDuty.key && fanDuty.value) return std::optional<std::unique_ptr<IValue>>(fanDuty.value->clone());
        return std::nullopt;
    }

    bool setParamValue(std::string_view key, std::unique_ptr<IValue> value) override {
        if (!value || key.empty()) return false;
        if (key == waitMs.key) {
            if (value->kind() != ValueKind::Float) return false;
            waitMs.value = std::move(value);
            return true;
        }
        if (key == fanMs.key) {
            if (value->kind() != ValueKind::Float) return false;
            fanMs.value = std::move(value);
            return true;
        }
        if (key == fanDuty.key) {
            if (value->kind() != ValueKind::Float) return false;
            fanDuty.value = std::move(value);
            return true;
        }
        return false;
    }

    void initialize() override {
        setState(State::Inactive);
    }

    void onActivating(StepContext& ctx) override {
        auto wait_duration = readParamMsOrDefault(waitMs, 5000.0f);
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
                float duty = 100.0f;
                if (fanDuty.value) fanDuty.value->get<float>(duty);
                auto out = ctx.getOutput(fan.aliasName);
                if (out) {
                    ValueFloat tmp(duty);
                    out->write(tmp);
                }
                auto fan_duration = readParamMsOrDefault(fanMs, 10000.0f);
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

private:
    static std::optional<std::unique_ptr<IValue>> cloneOptionalValuePtr(const std::optional<std::unique_ptr<IValue>>& src) {
        if (!src) return std::nullopt;
        if (!src->get()) return std::optional<std::unique_ptr<IValue>>(nullptr);
        return std::optional<std::unique_ptr<IValue>>((*src)->clone());
    }

    static float readParamMsOrDefault(const ParamDef& p, float def) {
        if (!p.value) return def;
        if (p.value->kind() != ValueKind::Float) return def;
        float val = def;
        p.value->get<float>(val);
        return val;
    }

    // Metadaten-Felder wurden entfernt aus dem Derived — StepBase hält sie jetzt.

    ParamDef waitMs;
    ParamDef fanMs;
    ParamDef fanDuty;

    IoAliasDef green;
    IoAliasDef red;
    IoAliasDef fan;
};