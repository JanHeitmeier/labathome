#pragma once

#include <vector>
#include <memory>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <string>
#include <string_view>
#include "../../domain/value-objects/StepMetadata.hh"
#include "../../domain/value-objects/StepContext.hh"
#include "../../domain/value-objects/ParameterValue.hh"

class IStep {
public:
    using ParamPtrList = std::vector<ParamDef*>;
    using AliasPtrList = std::vector<IoAliasDef*>;

    enum class State : uint8_t {
        Inactive = 0,
        Activating,
        Active,
        Deactivating,
        Deactivated
    };

    virtual ~IStep() = default;

    virtual StepMetadata getMetadata() const noexcept = 0;
    virtual ParamPtrList getParamDefPointers() noexcept = 0;
    virtual AliasPtrList getIoAliasPointers() noexcept = 0;

    virtual std::optional<ParameterValue> getParamValue(std::string_view key) const = 0;
    virtual bool setParamValue(std::string_view key, ParameterValue value) = 0;

    virtual void initialize() = 0;
    virtual void onActivating(StepContext& context) = 0;
    virtual void onActive(StepContext& context) = 0;
    virtual void onDeactivating(StepContext& context) = 0;
    virtual void onDeactivated(StepContext& context) = 0;
    virtual bool isTransitionConditionMet(StepContext& context) = 0;

    virtual void onError(StepContext& context, const char* msg) {
        (void)context; (void)msg;
    }

    virtual void triggerEvent(StepContext& context, const char* event) {
        context.getEventQueue().push(event);
    }

    virtual State state() const noexcept = 0;
    virtual void setState(State s) noexcept = 0;

protected:
    IStep() = default;
};

// Einfache Basisklasse für Step-Implementierungen
// Externe Entwickler erben von dieser Klasse und implementieren die fehlenden virtuellen Methoden
class StepBase : public IStep {
public:
    StepBase() = default;
    
    StepBase(std::uint32_t typeId, std::string_view displayName, 
             std::string_view description, std::string_view version)
        : typeId_(typeId), 
          displayName_(displayName),
          description_(description),
          version_(version) {}
    
    StepBase(const StepBase& other)
        : state_(other.state_),
          typeId_(other.typeId_),
          displayName_(other.displayName_),
          description_(other.description_),
          version_(other.version_)
    {
        // WICHTIG: Pointer-Listen und Maps werden NICHT kopiert!
        // Die abgeleitete Klasse muss ihre eigenen ParamDefs/IoAliasDefs
        // neu registrieren im Copy-Konstruktor via registerParamDefs() / registerIoAliases()
        // Dadurch zeigen die Pointer auf die eigenen Member-Variablen, nicht auf fremde Objekte
    }
    
    StepBase(StepBase&&) = default;
    StepBase& operator=(const StepBase&) = delete;
    StepBase& operator=(StepBase&&) = delete;
    ~StepBase() override = default;

    State state() const noexcept override { return state_; }
    void setState(State s) noexcept override { state_ = s; }

    // Diese müssen in der abgeleiteten Klasse implementiert werden
    // (pure virtual bleibt aus IStep)
    
    void initialize() override {
        setState(State::Inactive);
    }

    ParamPtrList getParamDefPointers() noexcept override {
        return m_paramPtrs;
    }

    AliasPtrList getIoAliasPointers() noexcept override {
        return m_aliasPtrs;
    }

    std::optional<ParameterValue> getParamValue(std::string_view key) const override {
        ParamDef* p = findParamDefPtr(key);
        if (!p) return std::nullopt;
        return p->value;
    }

    bool setParamValue(std::string_view key, ParameterValue value) override {
        ParamDef* p = findParamDefPtr(key);
        if (!p) return false;

        // Optional: Type check if the parameter already has a value/type defined
        if (p->value.index() != 0 && p->value.index() != value.index()) {
            // 0 is monostate (empty), so we allow setting it
            return false;
        }

        p->value = std::move(value);
        return true;
    }

    StepMetadata getMetadata() const noexcept override {
        StepMetadata md;
        md.typeId = typeId_;
        md.displayName = displayName_;
        md.description = description_;
        md.version = version_;

        md.params.clear();
        for (ParamDef* p : m_paramPtrs) {
            if (p) {
                md.params.push_back(*p);
            }
        }

        md.ioAliases.clear();
        for (IoAliasDef* a : m_aliasPtrs) {
            if (a) {
                md.ioAliases.push_back(*a);
            }
        }

        return md;
    }

    void setParamPtrList(const ParamPtrList& list) {
        m_paramPtrs = list;
        rebuildParamMap();
    }

    void setAliasPtrList(const AliasPtrList& list) {
        m_aliasPtrs = list;
        rebuildAliasMap();
    }

    void rebuildMapsFromPtrs() {
        rebuildParamMap();
        rebuildAliasMap();
    }

    void registerParamDef(ParamDef* p) {
        if (!p) return;
        m_paramMap[p->key] = p;
        m_paramPtrs.push_back(p);
    }

    void registerParamDefs(const std::initializer_list<ParamDef*>& list) {
        for (ParamDef* p : list) registerParamDef(p);
    }

    void registerIoAlias(IoAliasDef* a) {
        if (!a) return;
        m_aliasMap[a->aliasName] = a;
        m_aliasPtrs.push_back(a);
    }

    void registerIoAliases(const std::initializer_list<IoAliasDef*>& list) {
        for (IoAliasDef* a : list) registerIoAlias(a);
    }

    ParamDef* findParamDefPtr(std::string_view key) const noexcept {
        auto it = m_paramMap.find(std::string(key));
        return it == m_paramMap.end() ? nullptr : it->second;
    }

    IoAliasDef* findIoAliasPtr(std::string_view alias) const noexcept {
        auto it = m_aliasMap.find(std::string(alias));
        return it == m_aliasMap.end() ? nullptr : it->second;
    }

protected:
    void copyParamValuesFrom(const StepBase& src) {
        for (size_t i = 0; i < m_paramPtrs.size() && i < src.m_paramPtrs.size(); ++i) {
            if (m_paramPtrs[i] && src.m_paramPtrs[i]) {
                m_paramPtrs[i]->value = src.m_paramPtrs[i]->value;
                m_paramPtrs[i]->minValue = src.m_paramPtrs[i]->minValue;
                m_paramPtrs[i]->maxValue = src.m_paramPtrs[i]->maxValue;
            }
        }
    }

    void clearAllParamValues() {
        for (ParamDef* p : m_paramPtrs) {
            if (p) {
                p->value = std::monostate{};
                p->minValue = std::nullopt;
                p->maxValue = std::nullopt;
            }
        }
    }

    template<typename T>
    static T readParamOrDefault(const ParamDef& p, T defaultValue) {
        if (const T* val = std::get_if<T>(&p.value)) {
            return *val;
        }
        return defaultValue;
    }

    bool isTimerExpired(StepContext& ctx, const char* name) const {
        return ctx.isTimerExpired(name);
    }

    std::uint32_t getTypeIdForMetadata() const noexcept { return typeId_; }
    const std::string& getDisplayNameForMetadata() const noexcept { return displayName_; }
    const std::string& getDescriptionForMetadata() const noexcept { return description_; }
    const std::string& getVersionForMetadata() const noexcept { return version_; }

    // Diese sind protected, damit abgeleitete Klassen darauf zugreifen können
    ParamPtrList m_paramPtrs;
    AliasPtrList m_aliasPtrs;

private:
    void rebuildParamMap() {
        m_paramMap.clear();
        for (ParamDef* p : m_paramPtrs) {
            if (p) m_paramMap[p->key] = p;
        }
    }

    void rebuildAliasMap() {
        m_aliasMap.clear();
        for (IoAliasDef* a : m_aliasPtrs) {
            if (a) m_aliasMap[a->aliasName] = a;
        }
    }

    State state_{State::Inactive};
    std::unordered_map<std::string, ParamDef*> m_paramMap;
    std::unordered_map<std::string, IoAliasDef*> m_aliasMap;

    std::uint32_t typeId_{0};
    std::string displayName_{};
    std::string description_{};
    std::string version_{};
};