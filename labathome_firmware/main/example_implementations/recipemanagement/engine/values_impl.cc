#pragma once
#include "../../../recipemanagement/core/interfaces/engine/IValueBase.hh"
#include <cstdint>

//implemntierung durch Template nutzung. 

//Definierung einer ID und eines Namens (Unit-Art) für jede Einheitart
struct CelsiusTag {
    static constexpr UnitId id = 100;
    static constexpr const char* name = "Celsius";
};
struct FahrenheitTag {
    static constexpr UnitId id = 101;
    static constexpr const char* name = "Fahrenheit";
};

//nachgetragen 
struct BoolTag {
    static constexpr UnitId id = 103;
    static constexpr const char* name = "Boolean";
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
class ValueRPM : public ValueBase<ValueRPM, float, RPMTag> {
public:
    using Base = ValueBase<ValueRPM, float, RPMTag>;
    using Base::Base;
};
class ValueMilliseconds : public ValueBase<ValueMilliseconds, int64_t, MsTag> {
public:
    using Base = ValueBase<ValueMilliseconds, int64_t, MsTag>;
    using Base::Base;
};

//nachgetragen
class ValueBool : public ValueBase<ValueBool, bool, BoolTag> {
public:
    using Base = ValueBase<ValueBool, bool, BoolTag>;
    using Base::Base;
};
