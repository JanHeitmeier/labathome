#pragma once

#include "../../../recipemanagement/core/interfaces/engine/IStep.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepContext.hh"
#include "../../../recipemanagement/core/domain/value-objects/StepMetadata.hh"
#include <chrono>
#include <cstdint>

class OpenChamberStep : public StepBase
{
private:
	IoAliasDef servoOutput;
    IoAliasDef yellowLedOutput;
    IoAliasDef redLedOutput;
	ParamDef yellowBlinkDurationMsParam;
	ParamDef redBlinkDurationMsParam;

	bool initialWarningDone;
	bool acknowledged;
	bool closingWarningDone;
	bool transitionReady;
	bool yellowLedOn;
	bool redLedOn;

public:
	OpenChamberStep()
		: StepBase("OpenChamber", "Open chamber for loading or unloading, wait for acknowledgement, then close chamber", "1.0"),
		  servoOutput("Servo", false, true, false, "float", std::nullopt, "Servo"),
		  yellowLedOutput("YellowLed", false, true, false, "uint32_t", std::nullopt, "LED2"),
		  redLedOutput("RedLed", false, true, false, "uint32_t", std::nullopt, "LED0"),
		  yellowBlinkDurationMsParam("yellowBlinkDurationMs",
		                           ParameterValue::fromTimeMilliseconds(2000),
		                           "Yellow Blink Duration",
		                           "Duration for yellow warning LED blinking before opening door",
		                           "ms",
		                           ParameterValue::fromTimeMilliseconds(300),
		                           ParameterValue::fromTimeMilliseconds(60000)),
		  redBlinkDurationMsParam("redBlinkDurationMs",
		                        ParameterValue::fromTimeMilliseconds(2000),
		                        "Red Blink Duration",
		                        "Duration for red warning LED blinking before closing door",
		                        "ms",
		                        ParameterValue::fromTimeMilliseconds(300),
		                        ParameterValue::fromTimeMilliseconds(60000),
		                        true),
		  initialWarningDone(false),
		  acknowledged(false),
		  closingWarningDone(false),
		  transitionReady(false),
		  yellowLedOn(false),
		  redLedOn(false)
	{
		registerIoAliases({&servoOutput, &yellowLedOutput, &redLedOutput});
		registerParamDefs({&yellowBlinkDurationMsParam, &redBlinkDurationMsParam});
	}

	OpenChamberStep(const OpenChamberStep& o)
		: StepBase(o),
		  servoOutput(o.servoOutput),
		  yellowLedOutput(o.yellowLedOutput),
		  redLedOutput(o.redLedOutput),
		  yellowBlinkDurationMsParam(o.yellowBlinkDurationMsParam),
		  redBlinkDurationMsParam(o.redBlinkDurationMsParam),
		  initialWarningDone(false),
		  acknowledged(false),
		  closingWarningDone(false),
		  transitionReady(false),
		  yellowLedOn(false),
		  redLedOn(false)
	{
		registerIoAliases({&servoOutput, &yellowLedOutput, &redLedOutput});
		registerParamDefs({&yellowBlinkDurationMsParam, &redBlinkDurationMsParam});
	}

	void initialize() override
	{
		initialWarningDone = false;
		acknowledged = false;
		closingWarningDone = false;
		transitionReady = false;
		yellowLedOn = false;
		redLedOn = false;
	}

	void onActivating(StepContext& ctx) override
	{
		setLed(ctx, yellowLedOutput, 0x00000000, "OpenChamber: Yellow LED output alias is not registered");
		setLed(ctx, redLedOutput, 0x00000000, "OpenChamber: Red LED output alias is not registered");

		initialWarningDone = false;
		acknowledged = false;
		closingWarningDone = false;
		transitionReady = false;
		yellowLedOn = false;
		redLedOn = false;

		uint32_t yellowDurationMs = readParamOrDefault(yellowBlinkDurationMsParam, uint32_t(2000));
		ctx.startTimer("loadChamberYellowBlinkToggleTimer", std::chrono::milliseconds(300));
		ctx.startTimer("loadChamberYellowBlinkDurationTimer", std::chrono::milliseconds(yellowDurationMs));
	}

	void onActive(StepContext& ctx) override
	{
		if (!initialWarningDone) {
			if (ctx.isTimerExpired("loadChamberYellowBlinkToggleTimer")) {
				yellowLedOn = !yellowLedOn;
				setLed(ctx,
				       yellowLedOutput,
				       yellowLedOn ? 0xFFFF00FF : 0x00000000,
				       "OpenChamber: Yellow LED output alias is not registered");
				ctx.startTimer("loadChamberYellowBlinkToggleTimer", std::chrono::milliseconds(300));
			}

			if (ctx.isTimerExpired("loadChamberYellowBlinkDurationTimer")) {
				ctx.stopTimer("loadChamberYellowBlinkDurationTimer");
				ctx.stopTimer("loadChamberYellowBlinkToggleTimer");
				yellowLedOn = false;
				setLed(ctx, yellowLedOutput, 0x00000000, "OpenChamber: Yellow LED output alias is not registered");

				initialWarningDone = true;
				setServoAngle(ctx, 180.0f);
				ctx.requestUserAcknowledgment("Load/Unload Chamber. Acknowledge when done!");
			}
			return;
		}

		if (!acknowledged) {
			if (ctx.isAcknowledged()) {
				acknowledged = true;
				redLedOn = false;
				uint32_t redDurationMs = readParamOrDefault(redBlinkDurationMsParam, uint32_t(2000));
				ctx.startTimer("loadChamberRedBlinkToggleTimer", std::chrono::milliseconds(300));
				ctx.startTimer("loadChamberRedBlinkDurationTimer", std::chrono::milliseconds(redDurationMs));
				return;
			}
			return;
		}

		if (!closingWarningDone) {
			if (ctx.isTimerExpired("loadChamberRedBlinkToggleTimer")) {
				redLedOn = !redLedOn;
				setLed(ctx,
				       redLedOutput,
				       redLedOn ? 0xFF0000FF : 0x00000000,
				       "OpenChamber: Red LED output alias is not registered");
				ctx.startTimer("loadChamberRedBlinkToggleTimer", std::chrono::milliseconds(300));
			}

			if (ctx.isTimerExpired("loadChamberRedBlinkDurationTimer")) {
				ctx.stopTimer("loadChamberRedBlinkDurationTimer");
				ctx.stopTimer("loadChamberRedBlinkToggleTimer");
				redLedOn = false;
				setLed(ctx, redLedOutput, 0x00000000, "OpenChamber: Red LED output alias is not registered");
				setServoAngle(ctx, 20.0f);
				closingWarningDone = true;
				transitionReady = false;
				ctx.startTimer("openChamberSettleTimer", std::chrono::milliseconds(800));
			}
		}

		if (closingWarningDone && !transitionReady && ctx.isTimerExpired("openChamberSettleTimer")) {
			ctx.stopTimer("openChamberSettleTimer");
			transitionReady = true;
		}
	}

	void onDeactivating(StepContext& ctx) override
	{
		ctx.stopTimer("loadChamberYellowBlinkToggleTimer");
		ctx.stopTimer("loadChamberYellowBlinkDurationTimer");
		ctx.stopTimer("loadChamberRedBlinkToggleTimer");
		ctx.stopTimer("loadChamberRedBlinkDurationTimer");
		ctx.stopTimer("openChamberSettleTimer");

		initialWarningDone = false;
		acknowledged = false;
		closingWarningDone = false;
		transitionReady = false;
		yellowLedOn = false;
		redLedOn = false;
		setLed(ctx, yellowLedOutput, 0x00000000, "OpenChamber: Yellow LED output alias is not registered");
		setLed(ctx, redLedOutput, 0x00000000, "OpenChamber: Red LED output alias is not registered");
	}

	void onDeactivated(StepContext& /*ctx*/) override {}

	bool isTransitionConditionMet(StepContext& ctx) override
	{
		(void)ctx;
		return transitionReady;
	}

private:
	void setServoAngle(StepContext& ctx, float angle)
	{
		auto servo = ctx.getOutput(servoOutput.aliasName);
		if (servo) {
			servo->write(ParameterValue::fromAngle(angle));
		} else {
			ctx.log("OpenChamber: Servo output alias is not registered");
		}
	}

	void setLed(StepContext& ctx, IoAliasDef& ledDef, uint32_t rgbaColor, const char* missingAliasLog)
	{
		auto led = ctx.getOutput(ledDef.aliasName);
		if (led) {
			led->write(ParameterValue::fromColor(rgbaColor));
		} else {
			ctx.log(missingAliasLog);
		}
	}

};

class CoolingStep : public StepBase
{
private:
	IoAliasDef fanOutput;
	IoAliasDef temperatureInput;
	ParamDef dutyParam;
	ParamDef durationMsParam;

public:
	CoolingStep()
		: StepBase("Cooling", "Run fan with configured duty for a configured duration", "1.0"),
		  fanOutput("Fan", false, true, false, "float", std::nullopt, "Fan"),
		  temperatureInput("Temperature", true, false, true, "float", std::nullopt, "Temperature", "degC"),
		  dutyParam("duty",
		          ParameterValue::fromPercentage(60.0f),
		          "Fan Duty",
		          "Fan duty cycle while cooling",
		          "%",
		          ParameterValue::fromPercentage(0.0f),
		          ParameterValue::fromPercentage(100.0f)),
		  durationMsParam("durationMs",
		                ParameterValue::fromTimeMilliseconds(10000),
		                "Cooling Duration",
		                "Duration for running the fan",
		                "ms",
		                ParameterValue::fromTimeMilliseconds(100),
		                ParameterValue::fromTimeMilliseconds(600000))
	{
		registerIoAliases({&fanOutput, &temperatureInput});
		registerParamDefs({&dutyParam, &durationMsParam});
	}

	CoolingStep(const CoolingStep& o)
		: StepBase(o),
		  fanOutput(o.fanOutput),
		  temperatureInput(o.temperatureInput),
		  dutyParam(o.dutyParam),
		  durationMsParam(o.durationMsParam)
	{
		registerIoAliases({&fanOutput, &temperatureInput});
		registerParamDefs({&dutyParam, &durationMsParam});
	}

	void initialize() override
	{
	}

	void onActivating(StepContext& ctx) override
	{
		auto fan = ctx.getOutput(fanOutput.aliasName);
		if (!fan) {
			ctx.log("Cooling: Fan output alias is not registered");
			ctx.startTimer("coolingTimer", std::chrono::milliseconds(1));
			return;
		}

		float duty = readParamOrDefault(dutyParam, 60.0f);
		uint32_t durationMs = readParamOrDefault(durationMsParam, uint32_t(10000));

		fan->write(ParameterValue::fromPercentage(duty));
		ctx.startTimer("coolingTimer", std::chrono::milliseconds(durationMs));
	}

	void onActive(StepContext& /*ctx*/) override
	{
	}

	void onDeactivating(StepContext& ctx) override
	{
		auto fan = ctx.getOutput(fanOutput.aliasName);
		if (fan) {
			fan->write(ParameterValue::fromPercentage(0.0f));
		}

		ctx.stopTimer("coolingTimer");
	}

	void onDeactivated(StepContext& /*ctx*/) override {}

	bool isTransitionConditionMet(StepContext& ctx) override
	{
		return ctx.isTimerExpired("coolingTimer");
	}
};

class TemperatureUntilAckStep : public StepBase
{
private:
	IoAliasDef temperatureInput;

public:
	TemperatureUntilAckStep()
		: StepBase("TemperatureUntilAck", "Read temperature sensor continuously until acknowledgement", "1.0"),
		  temperatureInput("Temperature", true, false, true, "float", std::nullopt, "Temperature", "degC")
	{
		registerIoAliases({&temperatureInput});
	}

	TemperatureUntilAckStep(const TemperatureUntilAckStep& o)
		: StepBase(o),
		  temperatureInput(o.temperatureInput)
	{
		registerIoAliases({&temperatureInput});
	}

	void initialize() override
	{
	}

	void onActivating(StepContext& ctx) override
	{
		ctx.requestUserAcknowledgment("Temperature test running. Acknowledge to continue.");
	}

	void onActive(StepContext& ctx) override
	{
		auto temperature = ctx.getInput(temperatureInput.aliasName);
		if (temperature) {
			(void)temperature->read();
		} else {
			ctx.log("TemperatureUntilAck: Temperature input alias is not registered");
		}
	}

	void onDeactivating(StepContext& /*ctx*/) override
	{
	}

	void onDeactivated(StepContext& /*ctx*/) override {}

	bool isTransitionConditionMet(StepContext& ctx) override
	{
		return ctx.isAcknowledged();
	}
};

class HeatingStep : public StepBase
{
private:
	IoAliasDef heaterTemperatureInput;
	IoAliasDef chamberTemperatureInput;
	IoAliasDef heaterOutput;
	ParamDef dutyParam;
	ParamDef heatDurationMsParam;
	ParamDef prePauseMsParam;
	ParamDef postPauseMsParam;

	bool prePauseDone;
	bool heatingDone;
	bool postPauseDone;
	bool heaterEnabled;

public:
	HeatingStep()
		: StepBase("Heating", "Pre-pause, heat chamber with configured power, then post-pause", "1.0"),
		  heaterTemperatureInput("HeaterTemperature", true, false, true, "float", std::nullopt, "HeaterTemperature", "degC"),
		  chamberTemperatureInput("Temperature", true, false, true, "float", std::nullopt, "Temperature", "degC"),
		  heaterOutput("Heater", false, true, false, "float", std::nullopt, "Heater"),
		  dutyParam("duty",
		          ParameterValue::fromPercentage(20.0f),
		          "Heater Duty",
		          "Heater power during heating",
		          "%",
		          ParameterValue::fromPercentage(10.0f),
		          ParameterValue::fromPercentage(40.0f)),
		  heatDurationMsParam("heatDurationMs",
		                  ParameterValue::fromTimeMilliseconds(10000),
		                  "Heat Duration",
		                  "Duration of active heating phase",
		                  "ms",
		                  ParameterValue::fromTimeMilliseconds(1000),
		                  ParameterValue::fromTimeMilliseconds(600000)),
		  prePauseMsParam("prePauseMs",
		               ParameterValue::fromTimeMilliseconds(2000),
		               "Pre Pause",
		               "Delay before heater is enabled",
		               "ms",
		               ParameterValue::fromTimeMilliseconds(1000),
		               ParameterValue::fromTimeMilliseconds(5000)),
		  postPauseMsParam("postPauseMs",
		                ParameterValue::fromTimeMilliseconds(2000),
		                "Post Pause",
		                "Delay after heater is disabled before transition",
		                "ms",
		                ParameterValue::fromTimeMilliseconds(1000),
		                ParameterValue::fromTimeMilliseconds(5000)),
		  prePauseDone(false),
		  heatingDone(false),
		  postPauseDone(false),
		  heaterEnabled(false)
	{
		registerIoAliases({&heaterTemperatureInput, &chamberTemperatureInput, &heaterOutput});
		registerParamDefs({&dutyParam, &heatDurationMsParam, &prePauseMsParam, &postPauseMsParam});
	}

	HeatingStep(const HeatingStep& o)
		: StepBase(o),
		  heaterTemperatureInput(o.heaterTemperatureInput),
		  chamberTemperatureInput(o.chamberTemperatureInput),
		  heaterOutput(o.heaterOutput),
		  dutyParam(o.dutyParam),
		  heatDurationMsParam(o.heatDurationMsParam),
		  prePauseMsParam(o.prePauseMsParam),
		  postPauseMsParam(o.postPauseMsParam),
		  prePauseDone(false),
		  heatingDone(false),
		  postPauseDone(false),
		  heaterEnabled(false)
	{
		registerIoAliases({&heaterTemperatureInput, &chamberTemperatureInput, &heaterOutput});
		registerParamDefs({&dutyParam, &heatDurationMsParam, &prePauseMsParam, &postPauseMsParam});
	}

	void initialize() override
	{
		prePauseDone = false;
		heatingDone = false;
		postPauseDone = false;
		heaterEnabled = false;
	}

	void onActivating(StepContext& ctx) override
	{
		prePauseDone = false;
		heatingDone = false;
		postPauseDone = false;
		heaterEnabled = false;

		setHeaterDuty(ctx, 0.0f);
		uint32_t prePauseMs = readParamOrDefault(prePauseMsParam, uint32_t(2000));
		ctx.startTimer("heatingPrePauseTimer", std::chrono::milliseconds(prePauseMs));
	}

	void onActive(StepContext& ctx) override
	{
		if (!prePauseDone) {
			if (ctx.isTimerExpired("heatingPrePauseTimer")) {
				ctx.stopTimer("heatingPrePauseTimer");
				prePauseDone = true;
				float duty = readParamOrDefault(dutyParam, 20.0f);
				setHeaterDuty(ctx, duty);
				heaterEnabled = true;
				uint32_t heatDurationMs = readParamOrDefault(heatDurationMsParam, uint32_t(10000));
				ctx.startTimer("heatingDurationTimer", std::chrono::milliseconds(heatDurationMs));
			}
			return;
		}

		if (!heatingDone) {
			if (ctx.isTimerExpired("heatingDurationTimer")) {
				ctx.stopTimer("heatingDurationTimer");
				heatingDone = true;
				if (heaterEnabled) {
					setHeaterDuty(ctx, 0.0f);
					heaterEnabled = false;
				}
				uint32_t postPauseMs = readParamOrDefault(postPauseMsParam, uint32_t(2000));
				ctx.startTimer("heatingPostPauseTimer", std::chrono::milliseconds(postPauseMs));
			}
			return;
		}

		if (!postPauseDone && ctx.isTimerExpired("heatingPostPauseTimer")) {
			ctx.stopTimer("heatingPostPauseTimer");
			postPauseDone = true;
		}
	}

	void onDeactivating(StepContext& ctx) override
	{
		ctx.stopTimer("heatingPrePauseTimer");
		ctx.stopTimer("heatingDurationTimer");
		ctx.stopTimer("heatingPostPauseTimer");

		if (heaterEnabled) {
			setHeaterDuty(ctx, 0.0f);
			heaterEnabled = false;
		}
	}

	void onDeactivated(StepContext& /*ctx*/) override {}

	bool isTransitionConditionMet(StepContext& ctx) override
	{
		if (isChamberTemperatureAtOrAbove(ctx, 40.0f)) {
			return true;
		}
		return postPauseDone;
	}

private:
	void setHeaterDuty(StepContext& ctx, float duty)
	{
		auto heater = ctx.getOutput(heaterOutput.aliasName);
		if (heater) {
			heater->write(ParameterValue::fromPercentage(duty));
		} else {
			ctx.log("Heating: Heater output alias is not registered");
		}
	}

	bool isChamberTemperatureAtOrAbove(StepContext& ctx, float thresholdC)
	{
		auto chamberTemp = ctx.getInput(chamberTemperatureInput.aliasName);
		if (!chamberTemp) {
			ctx.log("Heating: Chamber temperature input alias is not registered");
			return false;
		}

		ParameterValue value = chamberTemp->read();
		if (!value.isType(ParameterType::TEMPERATURE)) {
			return false;
		}

		return value.getTemperature() >= thresholdC;
	}
};
