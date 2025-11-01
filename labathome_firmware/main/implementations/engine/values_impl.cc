#pragma once
#include "../entities/IValueBase.hh"
#include <cstdint>

//implemntierung durch Template nutzung. 

//Definierung einer ID und eines Namens (Unit) für jede Einheitart
struct CelsiusTag {
    static constexpr UnitId id = 100;
    static constexpr const char* name = "Celsius";
};
struct FahrenheitTag {
    static constexpr UnitId id = 101;
    static constexpr const char* name = "Fahrenheit";
};
struct RPMTag {
    static constexpr UnitId id = 200;
    static constexpr const char* name = "RPM";
};
struct MsTag {
    static constexpr UnitId id = 1;
    static constexpr const char* name = "Milliseconds";
};

//deklarierung der konkreten Value klasse. durch Template implementiert es ein interface für einfache nutzung in Collections

class ValueCelsius : public ValueBase<ValueCelsius, double, CelsiusTag> {
public:
    using Base = ValueBase<ValueCelsius, double, CelsiusTag>;
    using Base::Base;
};
class ValueFahrenheit : public ValueBase<ValueFahrenheit, double, FahrenheitTag> {
public:
    using Base = ValueBase<ValueFahrenheit, double, FahrenheitTag>;
    using Base::Base;
};
class ValueRPM : public ValueBase<ValueRPM, int64_t, RPMTag> {
public:
    using Base = ValueBase<ValueRPM, int64_t, RPMTag>;
    using Base::Base;
};
class ValueMilliseconds : public ValueBase<ValueMilliseconds, int64_t, MsTag> {
public:
    using Base = ValueBase<ValueMilliseconds, int64_t, MsTag>;
    using Base::Base;
};


//nachgetragen 
struct BoolTag {
    static constexpr UnitId id = 0;
    static constexpr const char* name = "Boolean";
};

class ValueBool : public ValueBase<ValueBool, bool, BoolTag> {
public:
    using Base = ValueBase<ValueBool, bool, BoolTag>;
    using Base::Base;
};
