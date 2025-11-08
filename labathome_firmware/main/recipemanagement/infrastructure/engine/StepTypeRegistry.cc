#include "StepTypeRegistry.hh"

StepTypeRegistry& StepTypeRegistry::instance() {
    static StepTypeRegistry registry;
    return registry;
}

std::vector<StepMetadata> StepTypeRegistry::availableTypes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<StepMetadata> result;
    result.reserve(m_types.size());
    
    for (const auto& [id, info] : m_types) {
        result.push_back(info.metadata);
    }
    
    return result;
}

AvailableStepsDto StepTypeRegistry::availableTypesAsDto() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AvailableStepsDto dto;
    dto.steps.reserve(m_types.size());
    
    for (const auto& [id, info] : m_types) {
        const StepMetadata& metadata = info.metadata;
        StepMetadataDto stepDto;
        
        // Basis-Informationen
        stepDto.typeId = std::to_string(metadata.typeId);
        stepDto.displayName = std::string(metadata.displayName);
        stepDto.description = std::string(metadata.description);
        stepDto.category = "general";
        
        // Parameter konvertieren
        for (const auto& param : metadata.params) {
            ParameterMetadataDto paramDto;
            paramDto.name = std::string(param.key);
            
            // Typ aus ParameterValue bestimmen
            std::visit([&paramDto](auto&& arg) {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, bool>) {
                    paramDto.type = "bool";
                } else if constexpr (std::is_same_v<T, int32_t>) {
                    paramDto.type = "int32";
                } else if constexpr (std::is_same_v<T, uint32_t>) {
                    paramDto.type = "uint32";
                } else if constexpr (std::is_same_v<T, int64_t>) {
                    paramDto.type = "int64";
                } else if constexpr (std::is_same_v<T, uint64_t>) {
                    paramDto.type = "uint64";
                } else if constexpr (std::is_same_v<T, float>) {
                    paramDto.type = "float";
                } else if constexpr (std::is_same_v<T, double>) {
                    paramDto.type = "double";
                } else if constexpr (std::is_same_v<T, std::string>) {
                    paramDto.type = "string";
                } else {
                    paramDto.type = "unknown";
                }
            }, param.value);
            
            paramDto.defaultValue = to_string(param.value);
            
            if (param.minValue.has_value()) {
                paramDto.minValue = to_string(*param.minValue);
            }
            
            if (param.maxValue.has_value()) {
                paramDto.maxValue = to_string(*param.maxValue);
            }
            
            paramDto.description = std::string(param.description);
            paramDto.required = true;
            paramDto.unit = param.unit;
            
            stepDto.parameters.push_back(std::move(paramDto));
        }
        
        // IoAliases konvertieren
        for (const auto& alias : metadata.ioAliases) {
            IoAliasMetadataDto ioDto;
            ioDto.aliasName = std::string(alias.aliasName);
            
            if (alias.isSensor) {
                ioDto.ioType = "sensor";
            } else if (alias.isInput && alias.isOutput) {
                ioDto.ioType = "input-output";
            } else if (alias.isInput) {
                ioDto.ioType = "input";
            } else if (alias.isOutput) {
                ioDto.ioType = "output";
            } else {
                ioDto.ioType = "unknown";
            }
            
            ioDto.valueType = alias.valueType;
            ioDto.description = "";
            
            stepDto.ioAliases.push_back(std::move(ioDto));
        }
        
        dto.steps.push_back(std::move(stepDto));
    }
    
    return dto;
}

std::unique_ptr<IStep> StepTypeRegistry::createInstance(uint32_t typeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_types.find(typeId);
    if (it == m_types.end()) {
        return nullptr;
    }
    
    // Factory-Funktion aufrufen um neue Instanz zu erzeugen
    return it->second.factory();
}

// HINWEIS: init() wird ABSICHTLICH NICHT hier implementiert!
// Diese Methode muss vom Entwickler in example_implementations/recipemanagement/init/InitSteps.cc
// implementiert werden. Wenn sie fehlt, gibt der Linker einen Fehler aus.
