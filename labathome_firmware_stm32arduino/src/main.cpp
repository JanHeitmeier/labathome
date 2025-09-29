#include <cstdio>
#include "Arduino.h"
#include <Wire.h>
#include "USBPowerDelivery.h"
#include <SimpleFOC.h>

#include "log.h"
#include <common.hh>
#include <gpioG4.hh>
#include <single_led.hh>
#include "encoder.hh"
#include "errorcheck.hh"
#include "stm32_esp32_communication.hh"
#include "errormanager.hh"

#if defined(ARDUINO_LABATHOME_15_3)
  // #include <TMCStepper.h>
  #include <TMC2209.h>
  #include <HardwareSerial.h>
  #include "busservo.hh"
#endif

#include "hal.hh"

constexpr uint8_t DRIVER_ADDRESS{0b00};
constexpr float R_SENSE{0.11f};

float brushless_motor_velocity{0.0};
constexpr int POLE_PAIRS{7};
BLDCMotor bldcmotor = BLDCMotor(POLE_PAIRS);
BLDCDriver3PWM bldcdriver = BLDCDriver3PWM(PIN::BL_DRV1, PIN::BL_DRV2, PIN::BL_DRV3, PIN::BL_ENABLE);

constexpr time_t TIMEOUT_FOR_FAILSAFE{3000};
bool alreadySetToFailsafe{false};
bool gotDataInfoAlreadyPrinted{false};

// Time related
uint32_t timeToPutActorsInFailsafe{TIMEOUT_FOR_FAILSAFE};

ErrorManager<8, uint32_t> errMan;

SINGLE_LED::M *led{nullptr};
HardwareTimer *TIM_BL_DRV{nullptr}; // TIM1 for Brushless
constexpr uint32_t BL_DRV_PH1_CH = 1;
constexpr uint32_t BL_DRV_PH2_CH = 2;
constexpr uint32_t BL_DRV_PH3_CH = 3;
ROTARY_ENCODER::M encoder;
constexpr uint32_t ROTENC_A_CH = 1;
constexpr uint32_t ROTENC_B_CH = 2;
HardwareTimer *TIM_HALL{nullptr}; // TIM3 for Hall Sensors, ch 2,3,4
constexpr uint32_t HALL_PH1_CH_CH = 4;
constexpr uint32_t HALL_PH2_CH_CH = 3;
constexpr uint32_t HALL_PH3_CH_CH = 2;


#ifdef ARDUINO_LABATHOME_15_0
HardwareTimer *TIM_LED_WHITE{nullptr}; // TIM4 for Power LED, ch 4
constexpr uint32_t LED_WHITE_CH = 4;
HardwareTimer *TIM_SERVO_0_1{nullptr}; // TIM15 for Servo0+1, ch1,2
constexpr uint32_t SERVO0_CH = 1;
constexpr uint32_t SERVO1_CH = 2;
HardwareTimer *TIM_SERVO_2{nullptr}; // TIM16 for Servo2, ch1
constexpr uint32_t SERVO2_CH = 1;
HardwareTimer *TIM_FAN{nullptr}; // TIM17 for Fan, ch1
constexpr uint32_t FAN_CH = 1;


#elif defined(ARDUINO_LABATHOME_15_1)
HardwareTimer *TIM_SERVO{nullptr}; // TIM1
constexpr uint32_t SERVO0_CH = 1;
constexpr uint32_t SERVO1_CH = 2;
constexpr uint32_t SERVO2_CH = 3;
constexpr uint32_t SERVO3_CH = 4;
HardwareTimer *TIM_FAN_LED_WHITE{nullptr}; // TIM15
constexpr uint32_t FAN_CH = 1;
constexpr uint32_t LED_WHITE_CH = 2;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;


#elif defined(ARDUINO_LABATHOME_15_2) 
HardwareTimer *TIM_SERVO{nullptr}; // TIM1
constexpr uint32_t SERVO0_CH = 1;
constexpr uint32_t SERVO1_CH = 2;
constexpr uint32_t SERVO2_CH = 3;
constexpr uint32_t SERVO3_CH = 4;
HardwareTimer *TIM_FAN_LED_WHITE{nullptr}; // TIM15
constexpr uint32_t LED_WHITE_CH = 1;
constexpr uint32_t FAN_CH = 2;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;


#elif defined(ARDUINO_LABATHOME_15_3)
HardwareTimer *TIM_SERVO{nullptr}; // TIM1
constexpr uint32_t SERVO0_CH = 1;
constexpr uint32_t SERVO1_CH = 2;
constexpr uint32_t SERVO2_CH = 3;
constexpr uint32_t SERVO3_CH = 4;
HardwareTimer *TIM_FAN_LED_WHITE{nullptr}; // TIM15
constexpr uint32_t LED_WHITE_CH = 1;
constexpr uint32_t FAN_CH = 2;
OPAMP_HandleTypeDef hopamp2;
OPAMP_HandleTypeDef hopamp3;
HardwareSerial StepperSerial(PIN::UART3_RX, PIN::UART3_TX);
//Important: Define TX-Pin in the "const PinMap PinMap_UART_TX[]"-structure as OpenDrain
HardwareSerial ServoBusSerial(PIN::ADC1_U1R, PIN::ADC0_U1T);
// TMC2209Stepper stepperdriver(PIN::UART3_RX, PIN::UART3_TX, R_SENSE, DRIVER_ADDRESS);
HardwareSerial & stepper_serial_stream = StepperSerial;
TMC2209 stepperdriver;
//SMS_STS servoBus;
ServoBus<true, GPIO::Pin::C4> servoBus(&ServoBusSerial);


#else
#error "No known board defined"
#endif


SINGLE_LED::BlinkPattern WAITING_FOR_CONNECTION(500, 500);
SINGLE_LED::BlinkPattern UNDER_CONTROL_FROM_MASTER(100, 900);
SINGLE_LED::BlinkPattern PROBLEM(100, 100);

void SetPhysicalOutputs();
uint8_t CollectBitInputs();

uint32_t receivedEvents{0};
uint32_t requestEvents{0};
E2S_t e2s_buffer;
S2E_t s2e_buffer;

// I2C callbacks

extern "C" void HAL_I2C_SlaveTxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_EnableListen_IT(hi2c);
}

extern "C" void HAL_I2C_SlaveRxCpltCallback(I2C_HandleTypeDef *hi2c)
{
  SetPhysicalOutputs();
  timeToPutActorsInFailsafe = millis() + 3000;
  ClearBitIdx(s2e_buffer.Status, 2);
  // HAL_I2C_EnableListen_IT(hi2c);
}

extern "C" void HAL_I2C_AddrCallback(I2C_HandleTypeDef *hi2c, uint8_t TransferDirection, uint16_t AddrMatchCode)
{

  UNUSED(AddrMatchCode);
  if (TransferDirection == I2C_DIRECTION_RECEIVE) // TransferDirection is master perspective
  {
    requestEvents++;
    HAL_ERROR_CHECK(HAL_I2C_Slave_Seq_Transmit_IT(hi2c, (uint8_t *)&s2e_buffer, sizeof(s2e_buffer), I2C_LAST_FRAME));
    // HAL_ERROR_CHECK(HAL_I2C_Slave_Seq_Transmit_IT(hi2c, test_buf, 12, I2C_LAST_FRAME));
  }
  else
  {
    receivedEvents++;
    HAL_ERROR_CHECK(HAL_I2C_Slave_Seq_Receive_IT(hi2c, (uint8_t *)&e2s_buffer, sizeof(e2s_buffer), I2C_LAST_FRAME));
  }
  timeToPutActorsInFailsafe = millis() + TIMEOUT_FOR_FAILSAFE;
}

extern "C" void HAL_I2C_ListenCpltCallback(I2C_HandleTypeDef *hi2c)
{
  HAL_I2C_EnableListen_IT(hi2c);
}

extern "C" void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)
{
  uint32_t error_code = HAL_I2C_GetError(hi2c);
  SetPhysicalOutputs();
  HAL_I2C_EnableListen_IT(hi2c);
}

namespace USB_PD
{
  void requestVoltage()
  {

    // check if 12V is supported
    for (int i = 0; i < PowerSink.numSourceCapabilities; i += 1)
    {
      if (PowerSink.sourceCapabilities[i].minVoltage <= 12000 && PowerSink.sourceCapabilities[i].maxVoltage >= 12000)
      {
        PowerSink.requestPower(12000);
        return;
      }
    }
    log_warn("Neither 20V nor 12V is supported");
  }

  void handleUsbPdEvent(PDSinkEventType eventType)
  {

    if (eventType == PDSinkEventType::sourceCapabilitiesChanged)
    {
      // source capabilities have changed
      if (PowerSink.isConnected())
      {
        log_info("PD supply is connected");
        for (int i = 0; i < PowerSink.numSourceCapabilities; i += 1)
        {
          log_info("minVoltage: %d maxVoltage:%d current:%d\n", PowerSink.sourceCapabilities[i].minVoltage, PowerSink.sourceCapabilities[i].maxVoltage, PowerSink.sourceCapabilities[i].maxCurrent);
        }
        requestVoltage();
      }
      else
      {
        // no supply or no USB PD capable supply is connected
        PowerSink.requestPower(5000); // reset to 5V
      }
    }
    else if (eventType == PDSinkEventType::voltageChanged)
    {
      // voltage has changed
      if (PowerSink.activeVoltage != 0)
      {
        log_info("Voltage: %d mV @ %d mA (max)", PowerSink.activeVoltage, PowerSink.activeCurrent);
      }
      else
      {
        log_info("Disconnected");
      }
    }
    else if (eventType == PDSinkEventType::powerRejected)
    {
      // rare case: power supply rejected requested power
      log_info("Power request rejected");
      log_info("Voltage: %d mV @ %d mA (max)", PowerSink.activeVoltage, PowerSink.activeCurrent);
    }
  }
}

unsigned int degree2us(unsigned int degrees)
{
  return 1000 + degrees * 150 / 27;
}

void SetServoAngle(uint8_t servo_0_1_2_3_4_5, uint8_t angle_0_180)
{
  if (angle_0_180 > 180)
    angle_0_180 = 180;
  uint32_t us = degree2us(angle_0_180);
  // log_info("Set Servo %d to %lu us", servo_0_1_2_3, us);
#ifdef ARDUINO_LABATHOME_15_0
  switch (servo_0_1_2_3_4_5)
  {
  case 0:
    TIM_SERVO_0_1->setCaptureCompare(SERVO0_CH, us, MICROSEC_COMPARE_FORMAT);
    break;
  case 1:
    TIM_SERVO_0_1->setCaptureCompare(SERVO1_CH, us, MICROSEC_COMPARE_FORMAT);
    break;
  case 2:
    TIM_SERVO_2->setCaptureCompare(SERVO2_CH, us, MICROSEC_COMPARE_FORMAT);
    break;
  default:
    break;
  }
#elif defined(ARDUINO_LABATHOME_15_1)
  if (servo_0_1_2_3_4_5 > 3)
    return;
  TIM_SERVO->setCaptureCompare(servo_0_1_2_3_4_5 + 1, us, MICROSEC_COMPARE_FORMAT);
#elif defined(ARDUINO_LABATHOME_15_2) || defined(ARDUINO_LABATHOME_15_3)
  if (servo_0_1_2_3_4_5 < 4)
  {
    TIM_SERVO->setCaptureCompare(servo_0_1_2_3_4_5 + 1, us, MICROSEC_COMPARE_FORMAT);
  }
#endif
}

void SetFanSpeed(uint8_t power_0_100)
{
  if (power_0_100 > 100)
  {
    power_0_100 = 100;
  }
#ifdef ARDUINO_LABATHOME_15_0
  TIM_FAN->setCaptureCompare(FAN_CH, power_0_100, PERCENT_COMPARE_FORMAT);
#elif defined(ARDUINO_LABATHOME_15_1) || defined(ARDUINO_LABATHOME_15_2) || defined(ARDUINO_LABATHOME_15_3)
  TIM_FAN_LED_WHITE->setCaptureCompare(FAN_CH, power_0_100, PERCENT_COMPARE_FORMAT);
#endif
}

void SetLedPowerPower(uint8_t power_0_100)
{
  if (power_0_100 > 100)
  {
    power_0_100 = 100;
  }
#ifdef ARDUINO_LABATHOME_15_0
  TIM_LED_WHITE->setCaptureCompare(LED_WHITE_CH, power_0_100, PERCENT_COMPARE_FORMAT);
#elif defined(ARDUINO_LABATHOME_15_1) || defined(ARDUINO_LABATHOME_15_2) || defined(ARDUINO_LABATHOME_15_3)
  TIM_FAN_LED_WHITE->setCaptureCompare(LED_WHITE_CH, power_0_100, PERCENT_COMPARE_FORMAT);
#endif
}

void SetPhysicalOutputs()
{
  // write rx_buffer to my outputs
#if defined(ARDUINO_LABATHOME_15_0) || defined(ARDUINO_LABATHOME_15_1) || defined(ARDUINO_LABATHOME_15_2)
  digitalWrite(PIN::RELAY, e2s_buffer.Relay);
#endif
  digitalWrite(PIN::BL_RESET, e2s_buffer.Blreset);
#if defined(ARDUINO_LABATHOME_15_1) || defined(ARDUINO_LABATHOME_15_2) || defined(ARDUINO_LABATHOME_15_3)
  digitalWrite(PIN::BL_SLEEP, e2s_buffer.Blsleep);
  // digitalWrite(PIN::LCD_RESET, e2s_buffer.LcdReset);
#endif
  // TODO USBC
  // HAL_DAC_SetValue(&hdac1, DAC1_CHANNEL_1, DAC_ALIGN_12B_R, ParseU16(esp2stm_buf, DAC1_POS));
  // HAL_DAC_SetValue(&hdac1, DAC1_CHANNEL_2, DAC_ALIGN_12B_R, ParseU16(esp2stm_buf, DAC2_POS));
  SetServoAngle(0, e2s_buffer.Servo[0]);
  SetServoAngle(1, e2s_buffer.Servo[1]);
  SetServoAngle(2, e2s_buffer.Servo[2]);
  SetServoAngle(3, e2s_buffer.Servo[3]);
  SetFanSpeed(e2s_buffer.Fan);
  SetLedPowerPower(e2s_buffer.LedPower);
  // SetHeaterPower: loop1ms automatically fetches correct value from buffer
  // TODO Brushless Mode
  if (e2s_buffer.ThreephaseMode == 0)
  {
    brushless_motor_velocity = e2s_buffer.ThreephaseP1;
  }
  // TODO Brushless Speeds
}

void setActorsToSafeSetting()
{
  memset((void *)&e2s_buffer, 0, sizeof(e2s_buffer));
  SetPhysicalOutputs();
}

void app_loop20ms(uint32_t now)
{
  led->Loop(now);

  // I2C_SLAVE::e2s_buffer[E2S::HEATER_POS] = encoder.GetTicks(); // COMMENT OUT FOR TESTING ONLY
  s2e_buffer.ButtonRed = digitalRead(PIN::BTN_RED);
  s2e_buffer.ButtonYellow = (!digitalRead(PIN::BTN_YELLOW));
  s2e_buffer.Movement = digitalRead(PIN::MOVEMENT);
  s2e_buffer.Blfault = (!digitalRead(PIN::BL_FAULT));
  s2e_buffer.Rotenc = encoder.GetTicks();
  s2e_buffer.Brightness = analogRead(PIN::BRIGHTNESS);
  s2e_buffer.UsbpdVoltage_mv = PowerSink.activeVoltage;
  //s2e_buffer.Adc0 = analogRead(PIN::ADC_0_U1T);
  //s2e_buffer.Adc1 = analogRead(PIN::ADC_1_U1R);
#if defined(ARDUINO_LABATHOME_15_2) || defined(ARDUINO_LABATHOME_15_3)
  s2e_buffer.PIN_PB12 = digitalRead(PIN::PIN_PB12);
  s2e_buffer.Adc2_24V = analogRead(PIN::ADC_2_24V);
// s2e_buffer.Adc3_Bl_i_sense=analogRead(PIN::BL_ISENSE); wrong, we do not read the input directly, but the OPAMPs output
#endif

  if (now >= timeToPutActorsInFailsafe)
  {
    if (!alreadySetToFailsafe)
    {
      log_warn("No Data from ESP32 - goto failsafe!");
      alreadySetToFailsafe = true;
      gotDataInfoAlreadyPrinted = false;
      setActorsToSafeSetting();
      SetBitIdx(s2e_buffer.Status, 2);
      led->AnimatePixel(millis(), &PROBLEM);
    }
  }
  else
  {
    if (!gotDataInfoAlreadyPrinted)
    {
      log_info("Got Data from ESP32");
      gotDataInfoAlreadyPrinted = true;
      led->AnimatePixel(millis(), &UNDER_CONTROL_FROM_MASTER);
    }
    alreadySetToFailsafe = false;
  }
}

char logBuffer1[96];
char logBuffer2[96];

void app_loop1000ms(uint32_t now)
{
  (void)now;

  // char* buf = errMan.GetLast8AsCharBuf_DoNotForgetToFree();
  byteBuf2hexCharBuf(logBuffer1, 96, (uint8_t *)&s2e_buffer, sizeof(s2e_buffer));
  byteBuf2hexCharBuf(logBuffer2, 96, (uint8_t *)&e2s_buffer, sizeof(e2s_buffer));
  log_info("enc=%d s2e=%s, e2s=%s", encoder.GetTicks(), logBuffer1, logBuffer2);

}

void app_loop_superfast(uint32_t now)
{
  // Heater; Wert ist von 0-100; Zyklus dauert 1000ms
  static uint32_t startOfCycle = 0;
  static bool heaterBefore=false;
  time_t passedTime = now - startOfCycle;
  if (passedTime >= (10 * 100))
  {
    startOfCycle = now;
    if (e2s_buffer.Heater > 0)
    {
      if(!heaterBefore){
        digitalWrite(PIN::HEATER, HIGH);
        log_info("Heater ON");
      }
      heaterBefore=true;
    }
    else
    {
      digitalWrite(PIN::HEATER, LOW);
      heaterBefore=false;
    }
  }
  else if (passedTime >= 10 * e2s_buffer.Heater)
  {
    if(heaterBefore){
      log_info("Heater OFF");
    }
    digitalWrite(PIN::HEATER, LOW);
    heaterBefore=false;
  }
}

void setup()
{

  led = new SINGLE_LED::M(PIN::LED_INFO, true, &WAITING_FOR_CONNECTION);
  led->Begin(millis(), &WAITING_FOR_CONNECTION, 10000);  
  Serial.begin(115200);
  
  log_info("Application started log_info");//works
  printf("Application started printf\n");//works
  Serial.printf("Application started Serial.printf\n");//works
  Serial4.printf("Application started Serial4.printf\n");//works
  
  log_info("Init gpio");
  pinMode(PIN::BTN_RED, INPUT);
  pinMode(PIN::BTN_YELLOW, INPUT);
  pinMode(PIN::MOVEMENT, INPUT);
  pinMode(PIN::BL_FAULT, INPUT);
  pinMode(PIN::HEATER, OUTPUT);
  pinMode(PIN::BRIGHTNESS, INPUT_ANALOG);

#if (defined(ARDUINO_LABATHOME_15_1) || defined(ARDUINO_LABATHOME_15_2))
  pinMode(PIN::RELAY, OUTPUT);
  pinMode(PIN::BL_SLEEP, OUTPUT);
  pinMode(PIN::BL_ISENSE, INPUT_ANALOG); // OPAMP2+
  pinMode(PIN::OPAMP3_M, INPUT_ANALOG);  // OPAMP3-
  pinMode(PIN::OPAMP3_P, INPUT_ANALOG);  // OPAMP3+
  pinMode(PIN::OPAMP3_Q, INPUT_ANALOG);  // OPAMP3+
  pinMode(PIN::SUPPLY_24V_SENSE, INPUT_ANALOG);

  hopamp2.Instance = OPAMP2;
  hopamp2.Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
  hopamp2.Init.Mode = OPAMP_PGA_MODE;
  hopamp2.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO1;
  hopamp2.Init.InternalOutput = ENABLE;
  hopamp2.Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp2.Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_NO;
  hopamp2.Init.PgaGain = OPAMP_PGA_GAIN_4_OR_MINUS_3;
  hopamp2.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
  HAL_ERROR_CHECK(HAL_OPAMP_Init(&hopamp2));
  HAL_ERROR_CHECK(HAL_OPAMP_Start(&hopamp2));
#endif

#ifdef ARDUINO_LABATHOME_15_2
  pinMode(PIN::ADC_2_24V, INPUT_ANALOG);
  pinMode(PIN::LCD_RESET, INPUT);
  pinMode(PIN::BL_RESET, OUTPUT);
  pinMode(PIN::PIN_PB12, INPUT);
  //pinMode(PIN::ADC_0, INPUT_ANALOG);
  //pinMode(PIN::ADC_1, INPUT_ANALOG);
#endif

#ifdef ARDUINO_LABATHOME_15_3
  pinMode(PIN::BL_SLEEP, OUTPUT);
  pinMode(PIN::BL_ISENSE, INPUT_ANALOG); // OPAMP2+
  pinMode(PIN::OPAMP3_M, INPUT_ANALOG);  // OPAMP3-
  pinMode(PIN::OPAMP3_P, INPUT_ANALOG);  // OPAMP3+
  pinMode(PIN::OPAMP3_Q, INPUT_ANALOG);  // OPAMP3+
  pinMode(PIN::SUPPLY_24V_SENSE, INPUT_ANALOG);

  hopamp2.Instance = OPAMP2;
  hopamp2.Init.PowerMode = OPAMP_POWERMODE_NORMALSPEED;
  hopamp2.Init.Mode = OPAMP_PGA_MODE;
  hopamp2.Init.NonInvertingInput = OPAMP_NONINVERTINGINPUT_IO1;
  hopamp2.Init.InternalOutput = ENABLE;
  hopamp2.Init.TimerControlledMuxmode = OPAMP_TIMERCONTROLLEDMUXMODE_DISABLE;
  hopamp2.Init.PgaConnect = OPAMP_PGA_CONNECT_INVERTINGINPUT_NO;
  hopamp2.Init.PgaGain = OPAMP_PGA_GAIN_4_OR_MINUS_3;
  hopamp2.Init.UserTrimming = OPAMP_TRIMMING_FACTORY;
  HAL_ERROR_CHECK(HAL_OPAMP_Init(&hopamp2));
  HAL_ERROR_CHECK(HAL_OPAMP_Start(&hopamp2));
  pinMode(PIN::ADC_2_24V, INPUT_ANALOG);
  pinMode(PIN::LCD_RESET, INPUT);//because it is normally driven by the ESP32
  pinMode(PIN::BL_RESET, OUTPUT);
  pinMode(PIN::PIN_PB12, INPUT);
#endif

  log_info("Init Rotary Encoder");
  encoder.Setup();

  log_info("Init Timer");
  // Problem: Die All-In-One-Funktion "setPWM" kann nur ganzzahlige Prozentangaben setzen, Dies ist zu Wenig für Servos
  // Lösung: Hier diese Funktion zum grundsätzlichen Initialisieren verwenden und oben die "setCaptureCompare"
#ifdef ARDUINO_LABATHOME_15_0
  TIM_LED_WHITE = new HardwareTimer(TIM4);
  TIM_SERVO_0_1 = new HardwareTimer(TIM15);
  TIM_SERVO_2 = new HardwareTimer(TIM16);
  TIM_FAN = new HardwareTimer(TIM17);

  TIM_LED_WHITE->setPWM(LED_WHITE_CH, PB_9_ALT1, 1000, 0);
  TIM_SERVO_0_1->setPWM(SERVO0_CH, PA_2_ALT1, 50, 0);
  TIM_SERVO_0_1->setPWM(SERVO1_CH, PA_3_ALT1, 50, 0);
  TIM_SERVO_2->setPWM(SERVO2_CH, PA_6_ALT1, 50, 0);
  TIM_FAN->setPWM(FAN_CH, PA_7_ALT3, 50, 0);
#elif defined(ARDUINO_LABATHOME_15_1)
  TIM_SERVO = new HardwareTimer(TIM1);
  TIM_SERVO->setPWM(SERVO0_CH, PC_0, 50, 0);
  TIM_SERVO->setPWM(SERVO1_CH, PC_1, 50, 0);
  TIM_SERVO->setPWM(SERVO2_CH, PC_2, 50, 0);
  TIM_SERVO->setPWM(SERVO3_CH, PC_3, 50, 0);

  TIM_FAN_LED_WHITE = new HardwareTimer(TIM15);
  TIM_FAN_LED_WHITE->setPWM(FAN_CH, PA_2_ALT1, 50, 0);
  TIM_FAN_LED_WHITE->setPWM(LED_WHITE_CH, PA_3_ALT1, 50, 0);

#elif defined(ARDUINO_LABATHOME_15_2)
  TIM_SERVO = new HardwareTimer(TIM1);
  TIM_SERVO->setPWM(SERVO0_CH, PC_0, 50, 0);
  TIM_SERVO->setPWM(SERVO1_CH, PC_1, 50, 0);
  TIM_SERVO->setPWM(SERVO2_CH, PC_2, 50, 0);
  TIM_SERVO->setPWM(SERVO3_CH, PC_3, 50, 0);

  TIM_FAN_LED_WHITE = new HardwareTimer(TIM15);
  TIM_FAN_LED_WHITE->setPWM(FAN_CH, PA_3_ALT1, 50, 0); // PA2 and PA3 changed from LabAtHome15.1 to 15.2
  TIM_FAN_LED_WHITE->setPWM(LED_WHITE_CH, PA_2_ALT1, 50, 0);

#elif defined(ARDUINO_LABATHOME_15_3)
  TIM_SERVO = new HardwareTimer(TIM1);
  TIM_SERVO->setPWM(SERVO0_CH, PC_0, 50, 0);
  TIM_SERVO->setPWM(SERVO1_CH, PC_1, 50, 0);
  TIM_SERVO->setPWM(SERVO2_CH, PC_2, 50, 0);
  TIM_SERVO->setPWM(SERVO3_CH, PC_3, 50, 0);

  TIM_FAN_LED_WHITE = new HardwareTimer(TIM15);
  TIM_FAN_LED_WHITE->setPWM(FAN_CH, PA_3_ALT1, 50, 0); // PA2 and PA3 changed from LabAtHome15.1 to 15.2
  TIM_FAN_LED_WHITE->setPWM(LED_WHITE_CH, PA_2_ALT1, 50, 0);

  log_info("Init ServoBus");
  ServoBusSerial.begin(1000000, SERIAL_8N1);
  //servoBus.pSerial = &ServoBusSerial;
  bool dummy=true;
  while(false){
    log_info("servoBus.Ping(5) returns %d",servoBus.Ping(5));
    servoBus.WritePosEx(5, 4095, 3400, 50);//servo(ID1) speed=3400，acc=50，move to position=4095.
    delay(2000);
    servoBus.WritePosEx(5, 2000, 1500, 50);//servo(ID1) speed=3400，acc=50，move to position=2000.
     delay(2000);
    led->Force(dummy);
    dummy = !dummy;
  }
  log_info("Init StepperSerial");
  StepperSerial.begin(115200);
  pinMode(PIN::STEPPER_EN, OUTPUT);
  pinMode(PIN::FAN_SERVO4_STEPPER3_DIR, OUTPUT);
  pinMode(PIN::LED_WHITE_P_SERVO5_STEPPER3_STEP, OUTPUT);
  digitalWrite(PIN::STEPPER_EN, LOW); // Enable driver in hardware
  /*
    stepperdriver.begin();
    stepperdriver.toff(5);                 // Enables driver in software
    stepperdriver.rms_current(600);        // Set motor RMS current
    stepperdriver.microsteps(16);          // Set microsteps to 1/16th
    //StealthChop=Leise
    stepperdriver.pwm_autoscale(true);     // Needed for stealthChop
    */
  stepperdriver.setup(StepperSerial, 115200, TMC2209::SERIAL_ADDRESS_0);
  delay(2000);

  stepperdriver.setRunCurrent(100);
  stepperdriver.enableCoolStep();
  stepperdriver.enable();
  stepperdriver.moveAtVelocity(20000);

  

#endif


  log_info("Init USB PD");
  PowerSink.start(USB_PD::handleUsbPdEvent);

  log_info("Init I2C");
  memset(&s2e_buffer, 0, sizeof(s2e_buffer));
  setActorsToSafeSetting();

  Wire.setSDA(PIN::SDA);
  Wire.setSCL(PIN::SCL);
  Wire.begin(I2C_SETUP::STM32_I2C_ADDRESS);

  SimpleFOCDebug::enable(&Serial);

  // driver config
  // power supply voltage [V]
  bldcdriver.voltage_power_supply = 12;
  // limit the maximal dc voltage the driver can set
  // as a protection measure for the low-resistance motors
  // this value is fixed on startup
  bldcdriver.voltage_limit = 6;
  if (!bldcdriver.init())
  {
    log_error("Driver init failed!");
    return;
  }
  // link the motor and the driver
  bldcmotor.linkDriver(&bldcdriver);

  // limiting motor movements
  // limit the voltage to be set to the motor
  // start very low for high resistance motors
  // current = voltage / resistance, so try to be well under 1Amp
  bldcmotor.voltage_limit = 3; // [V]

  // open loop control config
  bldcmotor.controller = MotionControlType::velocity_openloop;

  // init motor hardware
  if (!bldcmotor.init())
  {
    log_error("Motor init failed!");
    return;
  }
  log_info("Motor ready!");

  log_info("Init completed - eternal loop starts");
}

void loop_servotest()
{
  int pos;
  for (pos = 0; pos <= 180; pos += 1)
  { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    SetServoAngle(0, pos); // tell servo to go to position in variable 'pos'
    SetServoAngle(1, pos);
    SetServoAngle(2, pos);
    delay(15); // waits 15ms for the servo to reach the position
  }
  for (pos = 180; pos >= 0; pos -= 1)
  {                        // goes from 180 degrees to 0 degrees
    SetServoAngle(0, pos); // tell servo to go to position in variable 'pos'
    SetServoAngle(1, pos);
    SetServoAngle(2, pos);
    delay(15); // waits 15ms for the servo to reach the position
  }
}

void loop_default()
{
  PowerSink.poll();
  bldcmotor.move(brushless_motor_velocity);
  static uint32_t last20ms = 0;
  static uint32_t last1000ms = 0;
  uint32_t now = millis();
  app_loop_superfast(now);
  uint32_t timePassed20 = now - last20ms;
  if (timePassed20 >= 20)
  {
    last20ms = now;
    app_loop20ms(now);
  }
  uint32_t timePassed1000 = now - last1000ms;
  if (timePassed1000 >= 1000)
  {
    last1000ms = now;
    app_loop1000ms(now);
  }
}
#if defined(ARDUINO_LABATHOME_15_3)
bool shaft = false;

void loop_steppertest()
{
  
  if (stepperdriver.isSetupAndCommunicating())
  {
    Serial.println("Stepper driver is setup and communicating!");
    Serial.println("Try turning driver power off to see what happens.");
  }
  else if (stepperdriver.isCommunicatingButNotSetup())
  {
    Serial.println("Stepper driver is communicating but not setup!");
    Serial.println("Running setup again...");
    stepperdriver.setup(stepper_serial_stream);
  }
  else
  {
    Serial.println("Stepper driver is not communicating!");
    Serial.println("Try turning driver power on to see what happens.");
  }
  Serial.println();
  delay(3000);
  /*
  // Run 5000 steps and switch direction in software
  for (uint16_t i = 5000; i>0; i--) {
    digitalWrite(PIN::LED_WHITE_P_SERVO5_STEPPER3_STEP, HIGH);
    delayMicroseconds(160);
    digitalWrite(PIN::LED_WHITE_P_SERVO5_STEPPER3_STEP, LOW);
    delayMicroseconds(160);
  }
  shaft = !shaft;
  stepperdriver.shaft(shaft);
  */
}

void loop_busservotest(){
  servoBus.WritePosEx(5, 4095, 3400, 50);//servo(ID1) speed=3400，acc=50，move to position=4095.
  delay(2000);
  
  servoBus.WritePosEx(5, 2000, 1500, 50);//servo(ID1) speed=3400，acc=50，move to position=2000.
  delay(2000);
}
#endif

void loop()
{
  loop_default();
}