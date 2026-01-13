#pragma once

#include <cstdint>
#include <string>

// DESIGN PRINCIPLES:
// - No heap allocations
// - 5 bytes per object (uint32_t + uint8_t)
// - Inline functions
// - Compact conversion tables
// - SI units for physical quantities with uint16 range - Is uint32 range but can be changed if needed
// - Temperature: Kelvin * 10, Pressure: kPa * 10

// PHYSICAL QUANTITY TYPES
enum class ParameterType : uint8_t {
    NONE = 0,
    TEMPERATURE,        // Kelvin * 10 (2731 = 273.1K = 0°C)
    PRESSURE,           // Kilopascal * 10 (1013 = 101.3 kPa)
    HUMIDITY,           // Percent * 100 (5000 = 50.00%)
    RPM,                // Revolutions per minute
    PERCENTAGE,         // Percent * 100 (10000 = 100.00%)
    TIME_SECONDS,       // Seconds
    TIME_MILLISECONDS,  // Milliseconds (max ~49.7 days)
    FLOW_RATE,          // ml/min
    VOLUME,             // ml * 10 (5000 = 500.0 ml)
    MASS,               // g * 10 (12345 = 1234.5 g)
    LENGTH,             // mm
    VOLTAGE,            // V * 100 (1234 = 12.34 V)
    CURRENT,            // mA
    POWER,              // W * 10 (123 = 12.3 W)
    CONCENTRATION,      // g/L * 100 (5000 = 50.00 g/L)
    PH_VALUE,           // pH * 100 (700 = pH 7.00, range 0-1400)
    VELOCITY,           // mm/s
    ANGLE,              // Degrees * 10 (900 = 90.0°, range 0-3600)
    BOOLEAN,            // bool (0 or 1)
    GENERIC_INT         // Generic integer value
};

// TEMPERATURE UNITS
enum class TemperatureUnit : uint8_t {
    KELVIN,
    CELSIUS,
    FAHRENHEIT
};

// Enum für verschiedene Druck-Einheiten
enum class PressureUnit : uint8_t {
    KILOPASCAL,
    PASCAL,
    BAR,
    PSI,
    MILLIBAR
};

// Enum für Volumen-Einheiten
enum class VolumeUnit : uint8_t {
    MILLILITER,
    LITER,
    CUBIC_METER
};

// Enum für Masse-Einheiten
enum class MassUnit : uint8_t {
    GRAM,
    KILOGRAM,
    MILLIGRAM
};

// Enum für Längen-Einheiten
enum class LengthUnit : uint8_t {
    MILLIMETER,
    CENTIMETER,
    METER
};

// Enum für Spannungs-Einheiten
enum class VoltageUnit : uint8_t {
    VOLT,
    MILLIVOLT
};

// Enum für Strom-Einheiten
enum class CurrentUnit : uint8_t {
    MILLIAMPERE,
    AMPERE
};

// Enum für Leistungs-Einheiten
enum class PowerUnit : uint8_t {
    WATT,
    KILOWATT,
    MILLIWATT
};

// Enum für Konzentrations-Einheiten
enum class ConcentrationUnit : uint8_t {
    GRAM_PER_LITER,
    MILLIGRAM_PER_LITER,
    MOLAR  // mol/L
};

// Enum für Geschwindigkeits-Einheiten
enum class VelocityUnit : uint8_t {
    MM_PER_SECOND,
    CM_PER_SECOND,
    METER_PER_SECOND
};

// UNIT CONVERSION TABLES - Kompakte Konvertierungsfaktoren
namespace UnitConversion {
    // Temperatur: Zu Kelvin-Raw (Kelvin * 10)
    constexpr float TEMP_TO_RAW[] = {
        10.0f,                  // KELVIN
        10.0f,                  // CELSIUS (offset +273.15 separat)
        10.0f / 1.8f           // FAHRENHEIT (berechnung separat)
    };
    
    // Druck: Zu Kilopascal-Raw (kPa * 10)
    constexpr float PRESSURE_TO_RAW[] = {
        10.0f,                  // KILOPASCAL
        0.01f,                  // PASCAL (Pa / 100)
        1000.0f,                // BAR
        68.9476f,               // PSI
        1.0f                    // MILLIBAR
    };
    
    // Volumen: Zu ml-Raw (ml * 10)
    constexpr float VOLUME_TO_RAW[] = {
        10.0f,                  // MILLILITER
        10000.0f,               // LITER
        10000000.0f             // CUBIC_METER
    };
    
    // Masse: Zu g-Raw (g * 10)
    constexpr float MASS_TO_RAW[] = {
        10.0f,                  // GRAM
        10000.0f,               // KILOGRAM
        0.01f                   // MILLIGRAM
    };
    
    // Länge: Zu mm-Raw (mm)
    constexpr float LENGTH_TO_RAW[] = {
        1.0f,                   // MILLIMETER
        10.0f,                  // CENTIMETER
        1000.0f                 // METER
    };
    
    // Spannung: Zu V-Raw (V * 100)
    constexpr float VOLTAGE_TO_RAW[] = {
        100.0f,                 // VOLT
        0.1f                    // MILLIVOLT
    };
    
    // Strom: Zu mA-Raw (mA)
    constexpr float CURRENT_TO_RAW[] = {
        1.0f,                   // MILLIAMPERE
        1000.0f                 // AMPERE
    };
    
    // Leistung: Zu W-Raw (W * 10)
    constexpr float POWER_TO_RAW[] = {
        10.0f,                  // WATT
        10000.0f,               // KILOWATT
        0.01f                   // MILLIWATT
    };
    
    // Konzentration: Zu g/L-Raw (g/L * 100)
    constexpr float CONCENTRATION_TO_RAW[] = {
        100.0f,                 // GRAM_PER_LITER
        0.1f,                   // MILLIGRAM_PER_LITER
        100.0f                  // MOLAR (vereinfacht, benötigt Molekulargewicht)
    };
    
    // Geschwindigkeit: Zu mm/s-Raw (mm/s)
    constexpr float VELOCITY_TO_RAW[] = {
        1.0f,                   // MM_PER_SECOND
        10.0f,                  // CM_PER_SECOND
        1000.0f                 // METER_PER_SECOND
    };
    
    // Konvertierung von Temperatur-Einheit zu Raw
    inline uint32_t tempToRaw(float value, TemperatureUnit unit) {
        switch (unit) {
            case TemperatureUnit::KELVIN:
                return static_cast<uint32_t>(value * 10.0f);
            case TemperatureUnit::CELSIUS:
                return static_cast<uint32_t>((value + 273.15f) * 10.0f);
            case TemperatureUnit::FAHRENHEIT:
                return static_cast<uint32_t>(((value - 32.0f) * 5.0f / 9.0f + 273.15f) * 10.0f);
            default:
                return 0;
        }
    }
    
    // Konvertierung von Raw zu Temperatur-Einheit
    inline float rawToTemp(uint32_t raw, TemperatureUnit unit) {
        float kelvin = raw / 10.0f;
        switch (unit) {
            case TemperatureUnit::KELVIN:
                return kelvin;
            case TemperatureUnit::CELSIUS:
                return kelvin - 273.15f;
            case TemperatureUnit::FAHRENHEIT:
                return (kelvin - 273.15f) * 9.0f / 5.0f + 32.0f;
            default:
                return 0.0f;
        }
    }
    
    // Konvertierung von Druck-Einheit zu Raw
    inline uint32_t pressureToRaw(float value, PressureUnit unit) {
        return static_cast<uint32_t>(value * PRESSURE_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    // Konvertierung von Raw zu Druck-Einheit
    inline float rawToPressure(uint32_t raw, PressureUnit unit) {
        return raw / PRESSURE_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Volumen-Konvertierungen
    inline uint32_t volumeToRaw(float value, VolumeUnit unit) {
        return static_cast<uint32_t>(value * VOLUME_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToVolume(uint32_t raw, VolumeUnit unit) {
        return raw / VOLUME_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Masse-Konvertierungen
    inline uint32_t massToRaw(float value, MassUnit unit) {
        return static_cast<uint32_t>(value * MASS_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToMass(uint32_t raw, MassUnit unit) {
        return raw / MASS_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Längen-Konvertierungen
    inline uint32_t lengthToRaw(float value, LengthUnit unit) {
        return static_cast<uint32_t>(value * LENGTH_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToLength(uint32_t raw, LengthUnit unit) {
        return raw / LENGTH_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Spannungs-Konvertierungen
    inline uint32_t voltageToRaw(float value, VoltageUnit unit) {
        return static_cast<uint32_t>(value * VOLTAGE_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToVoltage(uint32_t raw, VoltageUnit unit) {
        return raw / VOLTAGE_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Strom-Konvertierungen
    inline uint32_t currentToRaw(float value, CurrentUnit unit) {
        return static_cast<uint32_t>(value * CURRENT_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToCurrent(uint32_t raw, CurrentUnit unit) {
        return raw / CURRENT_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Leistungs-Konvertierungen
    inline uint32_t powerToRaw(float value, PowerUnit unit) {
        return static_cast<uint32_t>(value * POWER_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToPower(uint32_t raw, PowerUnit unit) {
        return raw / POWER_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Konzentrations-Konvertierungen
    inline uint32_t concentrationToRaw(float value, ConcentrationUnit unit) {
        return static_cast<uint32_t>(value * CONCENTRATION_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToConcentration(uint32_t raw, ConcentrationUnit unit) {
        return raw / CONCENTRATION_TO_RAW[static_cast<uint8_t>(unit)];
    }
    
    // Geschwindigkeits-Konvertierungen
    inline uint32_t velocityToRaw(float value, VelocityUnit unit) {
        return static_cast<uint32_t>(value * VELOCITY_TO_RAW[static_cast<uint8_t>(unit)]);
    }
    
    inline float rawToVelocity(uint32_t raw, VelocityUnit unit) {
        return raw / VELOCITY_TO_RAW[static_cast<uint8_t>(unit)];
    }
}

// Hauptklasse für Parameter-Werte mit Unit-Awareness und Type-Safety
class ParameterValue {
private:
    uint32_t m_rawValue;
    ParameterType m_type;
    
public:
    // Standard-Konstruktor (leerer/ungültiger Wert)
    ParameterValue() : m_rawValue(0), m_type(ParameterType::NONE) {}
    
    // ========================================================================
    // FACTORY-METHODEN (Input-Seite)
    // ========================================================================
    
    static ParameterValue fromTemperature(float value, TemperatureUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::TEMPERATURE;
        pv.m_rawValue = UnitConversion::tempToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromPressure(float value, PressureUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::PRESSURE;
        pv.m_rawValue = UnitConversion::pressureToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromHumidity(float percentValue) {
        ParameterValue pv;
        pv.m_type = ParameterType::HUMIDITY;
        pv.m_rawValue = static_cast<uint32_t>(percentValue * 100.0f);
        return pv;
    }
    
    static ParameterValue fromRPM(uint32_t rpm) {
        ParameterValue pv;
        pv.m_type = ParameterType::RPM;
        pv.m_rawValue = rpm;
        return pv;
    }
    
    static ParameterValue fromPercentage(float percentValue) {
        ParameterValue pv;
        pv.m_type = ParameterType::PERCENTAGE;
        pv.m_rawValue = static_cast<uint32_t>(percentValue * 100.0f);
        return pv;
    }
    
    static ParameterValue fromBoolean(bool value) {
        ParameterValue pv;
        pv.m_type = ParameterType::BOOLEAN;
        pv.m_rawValue = value ? 1 : 0;
        return pv;
    }
    
    static ParameterValue fromTimeSeconds(uint32_t seconds) {
        ParameterValue pv;
        pv.m_type = ParameterType::TIME_SECONDS;
        pv.m_rawValue = seconds;
        return pv;
    }
    
    static ParameterValue fromFlowRate(uint32_t mlPerMinute) {
        ParameterValue pv;
        pv.m_type = ParameterType::FLOW_RATE;
        pv.m_rawValue = mlPerMinute;
        return pv;
    }
    
    static ParameterValue fromGenericInt(int32_t value) {
        ParameterValue pv;
        pv.m_type = ParameterType::GENERIC_INT;
        pv.m_rawValue = static_cast<uint32_t>(value);
        return pv;
    }
    
    static ParameterValue fromTimeMilliseconds(uint32_t milliseconds) {
        ParameterValue pv;
        pv.m_type = ParameterType::TIME_MILLISECONDS;
        pv.m_rawValue = milliseconds;
        return pv;
    }
    
    static ParameterValue fromVolume(float value, VolumeUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::VOLUME;
        pv.m_rawValue = UnitConversion::volumeToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromMass(float value, MassUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::MASS;
        pv.m_rawValue = UnitConversion::massToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromLength(float value, LengthUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::LENGTH;
        pv.m_rawValue = UnitConversion::lengthToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromVoltage(float value, VoltageUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::VOLTAGE;
        pv.m_rawValue = UnitConversion::voltageToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromCurrent(float value, CurrentUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::CURRENT;
        pv.m_rawValue = UnitConversion::currentToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromPower(float value, PowerUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::POWER;
        pv.m_rawValue = UnitConversion::powerToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromConcentration(float value, ConcentrationUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::CONCENTRATION;
        pv.m_rawValue = UnitConversion::concentrationToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromPH(float phValue) {
        ParameterValue pv;
        pv.m_type = ParameterType::PH_VALUE;
        pv.m_rawValue = static_cast<uint32_t>(phValue * 100.0f);
        return pv;
    }
    
    static ParameterValue fromVelocity(float value, VelocityUnit unit) {
        ParameterValue pv;
        pv.m_type = ParameterType::VELOCITY;
        pv.m_rawValue = UnitConversion::velocityToRaw(value, unit);
        return pv;
    }
    
    static ParameterValue fromAngle(float degrees) {
        ParameterValue pv;
        pv.m_type = ParameterType::ANGLE;
        pv.m_rawValue = static_cast<uint32_t>(degrees * 10.0f);
        return pv;
    }
    
    // ========================================================================
    // GETTER-METHODEN (Output-Seite)
    // ========================================================================
    
    float getTemperature(TemperatureUnit unit = TemperatureUnit::CELSIUS) const {
        return (m_type == ParameterType::TEMPERATURE) ? 
               UnitConversion::rawToTemp(m_rawValue, unit) : 0.0f;
    }
    
    float getPressure(PressureUnit unit = PressureUnit::KILOPASCAL) const {
        return (m_type == ParameterType::PRESSURE) ? 
               UnitConversion::rawToPressure(m_rawValue, unit) : 0.0f;
    }
    
    float getHumidity() const {
        return (m_type == ParameterType::HUMIDITY) ? m_rawValue / 100.0f : 0.0f;
    }
    
    uint32_t getRPM() const {
        return (m_type == ParameterType::RPM) ? m_rawValue : 0;
    }
    
    float getPercentage() const {
        return (m_type == ParameterType::PERCENTAGE) ? m_rawValue / 100.0f : 0.0f;
    }
    
    bool getBoolean() const {
        return (m_type == ParameterType::BOOLEAN) ? (m_rawValue != 0) : false;
    }
    
    uint32_t getTimeSeconds() const {
        return (m_type == ParameterType::TIME_SECONDS) ? m_rawValue : 0;
    }
    
    uint32_t getFlowRate() const {
        return (m_type == ParameterType::FLOW_RATE) ? m_rawValue : 0;
    }
    
    int32_t getGenericInt() const {
        return (m_type == ParameterType::GENERIC_INT) ? static_cast<int32_t>(m_rawValue) : 0;
    }
    
    uint32_t getTimeMilliseconds() const {
        return (m_type == ParameterType::TIME_MILLISECONDS) ? m_rawValue : 0;
    }
    
    float getVolume(VolumeUnit unit = VolumeUnit::MILLILITER) const {
        return (m_type == ParameterType::VOLUME) ?
               UnitConversion::rawToVolume(m_rawValue, unit) : 0.0f;
    }
    
    float getMass(MassUnit unit = MassUnit::GRAM) const {
        return (m_type == ParameterType::MASS) ?
               UnitConversion::rawToMass(m_rawValue, unit) : 0.0f;
    }
    
    float getLength(LengthUnit unit = LengthUnit::MILLIMETER) const {
        return (m_type == ParameterType::LENGTH) ?
               UnitConversion::rawToLength(m_rawValue, unit) : 0.0f;
    }
    
    float getVoltage(VoltageUnit unit = VoltageUnit::VOLT) const {
        return (m_type == ParameterType::VOLTAGE) ?
               UnitConversion::rawToVoltage(m_rawValue, unit) : 0.0f;
    }
    
    float getCurrent(CurrentUnit unit = CurrentUnit::MILLIAMPERE) const {
        return (m_type == ParameterType::CURRENT) ?
               UnitConversion::rawToCurrent(m_rawValue, unit) : 0.0f;
    }
    
    float getPower(PowerUnit unit = PowerUnit::WATT) const {
        return (m_type == ParameterType::POWER) ?
               UnitConversion::rawToPower(m_rawValue, unit) : 0.0f;
    }
    
    float getConcentration(ConcentrationUnit unit = ConcentrationUnit::GRAM_PER_LITER) const {
        return (m_type == ParameterType::CONCENTRATION) ?
               UnitConversion::rawToConcentration(m_rawValue, unit) : 0.0f;
    }
    
    float getPH() const {
        return (m_type == ParameterType::PH_VALUE) ? m_rawValue / 100.0f : 0.0f;
    }
    
    float getVelocity(VelocityUnit unit = VelocityUnit::MM_PER_SECOND) const {
        return (m_type == ParameterType::VELOCITY) ?
               UnitConversion::rawToVelocity(m_rawValue, unit) : 0.0f;
    }
    
    float getAngle() const {
        return (m_type == ParameterType::ANGLE) ? m_rawValue / 10.0f : 0.0f;
    }
    
    // ========================================================================
    // STRING PARSING (für JSON/Storage → ParameterValue Konvertierung)
    
    /**
     * @brief Parse String zu ParameterValue basierend auf erwartetem Type
     * 
     * Diese Methode wird von RecipeEngine verwendet um String-Parameter
     * aus JSON/Storage in typisierte ParameterValue Objekte zu konvertieren.
     * 
     * @param valueStr String-Darstellung des Werts (z.B. "2500", "true", "25.5")
     * @param expectedType Der erwartete ParameterType
     * @return Geparster ParameterValue oder ungültiger Wert bei Fehler
     */
    static ParameterValue parseFromString(const std::string& valueStr, ParameterType expectedType) {
        // Leere Strings returnen ungültigen Wert
        if (valueStr.empty()) {
            return ParameterValue();
        }
        
        switch (expectedType) {
            case ParameterType::BOOLEAN:
                return fromBoolean(valueStr == "true" || valueStr == "1");
            
            case ParameterType::TIME_MILLISECONDS: {
                char* end;
                unsigned long val = std::strtoul(valueStr.c_str(), &end, 10);
                if (end == valueStr.c_str()) return ParameterValue(); // Parse-Fehler
                return fromTimeMilliseconds(static_cast<uint32_t>(val));
            }
            
            case ParameterType::TIME_SECONDS: {
                char* end;
                unsigned long val = std::strtoul(valueStr.c_str(), &end, 10);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromTimeSeconds(static_cast<uint32_t>(val));
            }
            
            case ParameterType::PERCENTAGE: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromPercentage(val);
            }
            
            case ParameterType::RPM: {
                char* end;
                unsigned long val = std::strtoul(valueStr.c_str(), &end, 10);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromRPM(static_cast<uint32_t>(val));
            }
            
            case ParameterType::GENERIC_INT: {
                char* end;
                long val = std::strtol(valueStr.c_str(), &end, 10);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromGenericInt(static_cast<int32_t>(val));
            }
            
            case ParameterType::TEMPERATURE: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromTemperature(val, TemperatureUnit::CELSIUS);
            }
            
            case ParameterType::PRESSURE: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromPressure(val, PressureUnit::KILOPASCAL);
            }
            
            case ParameterType::HUMIDITY: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromHumidity(val);
            }
            
            case ParameterType::FLOW_RATE: {
                char* end;
                unsigned long val = std::strtoul(valueStr.c_str(), &end, 10);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromFlowRate(static_cast<uint32_t>(val));
            }
            
            case ParameterType::VOLUME: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromVolume(val, VolumeUnit::MILLILITER);
            }
            
            case ParameterType::MASS: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromMass(val, MassUnit::GRAM);
            }
            
            case ParameterType::LENGTH: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromLength(val, LengthUnit::MILLIMETER);
            }
            
            case ParameterType::VOLTAGE: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromVoltage(val, VoltageUnit::VOLT);
            }
            
            case ParameterType::CURRENT: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromCurrent(val, CurrentUnit::MILLIAMPERE);
            }
            
            case ParameterType::POWER: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromPower(val, PowerUnit::WATT);
            }
            
            case ParameterType::CONCENTRATION: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromConcentration(val, ConcentrationUnit::GRAM_PER_LITER);
            }
            
            case ParameterType::PH_VALUE: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromPH(val);
            }
            
            case ParameterType::VELOCITY: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromVelocity(val, VelocityUnit::MM_PER_SECOND);
            }
            
            case ParameterType::ANGLE: {
                char* end;
                float val = std::strtof(valueStr.c_str(), &end);
                if (end == valueStr.c_str()) return ParameterValue();
                return fromAngle(val);
            }
            
            case ParameterType::NONE:
            default:
                return ParameterValue();
        }
    }
    
    // ========================================================================
    // TYPE-CHECKING UND METADATEN
    // ========================================================================
    
    ParameterType getType() const { return m_type; }
    bool isType(ParameterType type) const { return m_type == type; }
    bool isValid() const { return m_type != ParameterType::NONE; }
    uint32_t getRawValue() const { return m_rawValue; }
    
    const char* getTypeName() const {
        static const char* names[] = {
            "None", "Temperature", "Pressure", "Humidity", "RPM",
            "Percentage", "Time", "TimeMs", "FlowRate", "Volume",
            "Mass", "Length", "Voltage", "Current", "Power",
            "Concentration", "pH", "Velocity", "Angle", "Boolean", "Integer"
        };
        return names[static_cast<uint8_t>(m_type)];
    }
    
    // ========================================================================
    // OPERATOREN
    // ========================================================================
    
    bool isCompatibleWith(const ParameterValue& other) const {
        return m_type != ParameterType::NONE && 
               other.m_type != ParameterType::NONE && 
               m_type == other.m_type;
    }
    
    bool operator==(const ParameterValue& other) const {
        return isCompatibleWith(other) && m_rawValue == other.m_rawValue;
    }
    
    bool operator!=(const ParameterValue& other) const {
        return !(*this == other);
    }
    
    bool operator<(const ParameterValue& other) const {
        return isCompatibleWith(other) && m_rawValue < other.m_rawValue;
    }
    
    bool operator>(const ParameterValue& other) const {
        return isCompatibleWith(other) && m_rawValue > other.m_rawValue;
    }
    
    bool operator<=(const ParameterValue& other) const {
        return isCompatibleWith(other) && m_rawValue <= other.m_rawValue;
    }
    
    bool operator>=(const ParameterValue& other) const {
        return isCompatibleWith(other) && m_rawValue >= other.m_rawValue;
    }
    
    ParameterValue operator+(const ParameterValue& other) const {
        if (!isCompatibleWith(other)) return ParameterValue();
        ParameterValue result;
        result.m_type = m_type;
        result.m_rawValue = m_rawValue + other.m_rawValue;
        return result;
    }
    
    ParameterValue operator-(const ParameterValue& other) const {
        if (!isCompatibleWith(other)) return ParameterValue();
        ParameterValue result;
        result.m_type = m_type;
        result.m_rawValue = (m_rawValue > other.m_rawValue) ? 
                            (m_rawValue - other.m_rawValue) : 0;
        return result;
    }
    
    // ========================================================================
    // STRING-KONVERTIERUNG
    // ========================================================================
    
    // Gibt nur den numerischen Wert als String zurück (ohne Unit)
    std::string toNumericString(TemperatureUnit tempUnit = TemperatureUnit::CELSIUS,
                               PressureUnit pressUnit = PressureUnit::KILOPASCAL) const {
        if (m_type == ParameterType::NONE) return "";
        
        switch (m_type) {
            case ParameterType::TEMPERATURE:
                return std::to_string(getTemperature(tempUnit));
            case ParameterType::PRESSURE:
                return std::to_string(getPressure(pressUnit));
            case ParameterType::HUMIDITY:
                return std::to_string(getHumidity());
            case ParameterType::RPM:
                return std::to_string(getRPM());
            case ParameterType::PERCENTAGE:
                return std::to_string(getPercentage());
            case ParameterType::BOOLEAN:
                return getBoolean() ? "true" : "false";
            case ParameterType::TIME_SECONDS:
                return std::to_string(getTimeSeconds());
            case ParameterType::FLOW_RATE:
                return std::to_string(getFlowRate());
            case ParameterType::GENERIC_INT:
                return std::to_string(getGenericInt());
            case ParameterType::TIME_MILLISECONDS:
                return std::to_string(getTimeMilliseconds());
            case ParameterType::VOLUME:
                return std::to_string(getVolume());
            case ParameterType::MASS:
                return std::to_string(getMass());
            case ParameterType::LENGTH:
                return std::to_string(getLength());
            case ParameterType::VOLTAGE:
                return std::to_string(getVoltage());
            case ParameterType::CURRENT:
                return std::to_string(getCurrent());
            case ParameterType::POWER:
                return std::to_string(getPower());
            case ParameterType::CONCENTRATION:
                return std::to_string(getConcentration());
            case ParameterType::PH_VALUE:
                return std::to_string(getPH());
            case ParameterType::VELOCITY:
                return std::to_string(getVelocity());
            case ParameterType::ANGLE:
                return std::to_string(getAngle());
            default:
                return "";
        }
    }
    
    std::string toString(TemperatureUnit tempUnit = TemperatureUnit::CELSIUS,
                        PressureUnit pressUnit = PressureUnit::KILOPASCAL) const {
        if (m_type == ParameterType::NONE) return "";
        
        switch (m_type) {
            case ParameterType::TEMPERATURE: {
                static const char* units[] = {"K", "°C", "°F"};
                return std::to_string(getTemperature(tempUnit)) + units[static_cast<uint8_t>(tempUnit)];
            }
            case ParameterType::PRESSURE: {
                static const char* units[] = {"kPa", "Pa", "bar", "psi", "mbar"};
                return std::to_string(getPressure(pressUnit)) + units[static_cast<uint8_t>(pressUnit)];
            }
            case ParameterType::HUMIDITY:
                return std::to_string(getHumidity()) + "%";
            case ParameterType::RPM:
                return std::to_string(getRPM()) + " RPM";
            case ParameterType::PERCENTAGE:
                return std::to_string(getPercentage()) + "%";
            case ParameterType::BOOLEAN:
                return getBoolean() ? "true" : "false";
            case ParameterType::TIME_SECONDS:
                return std::to_string(getTimeSeconds()) + "s";
            case ParameterType::FLOW_RATE:
                return std::to_string(getFlowRate()) + " ml/min";
            case ParameterType::GENERIC_INT:
                return std::to_string(getGenericInt());
            case ParameterType::TIME_MILLISECONDS:
                return std::to_string(getTimeMilliseconds()) + "ms";
            case ParameterType::VOLUME:
                return std::to_string(getVolume()) + "ml";
            case ParameterType::MASS:
                return std::to_string(getMass()) + "g";
            case ParameterType::LENGTH:
                return std::to_string(getLength()) + "mm";
            case ParameterType::VOLTAGE:
                return std::to_string(getVoltage()) + "V";
            case ParameterType::CURRENT:
                return std::to_string(getCurrent()) + "mA";
            case ParameterType::POWER:
                return std::to_string(getPower()) + "W";
            case ParameterType::CONCENTRATION:
                return std::to_string(getConcentration()) + "g/L";
            case ParameterType::PH_VALUE:
                return "pH " + std::to_string(getPH());
            case ParameterType::VELOCITY:
                return std::to_string(getVelocity()) + "mm/s";
            case ParameterType::ANGLE:
                return std::to_string(getAngle()) + "°";
            default:
                return "";
        }
    }
};

// Legacy-Kompatibilität
inline std::string to_string(const ParameterValue& pv) {
    return pv.toString();
}
    