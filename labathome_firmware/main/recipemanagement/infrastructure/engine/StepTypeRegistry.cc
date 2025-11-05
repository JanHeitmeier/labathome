#include "StepTypeRegistry.hh"

StepTypeRegistry& StepTypeRegistry::instance() {
    static StepTypeRegistry registry;
    return registry;
}

void StepTypeRegistry::registerStepType(std::unique_ptr<IStep> prototype) {
    if (!prototype) return;
    
    std::lock_guard<std::mutex> lock(m_mutex);
    uint32_t typeId = prototype->getMetadata().typeId;
    m_prototypes[typeId] = std::move(prototype);
}

std::vector<StepMetadata> StepTypeRegistry::availableTypes() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    std::vector<StepMetadata> result;
    result.reserve(m_prototypes.size());
    
    for (const auto& [id, prototype] : m_prototypes) {
        if (prototype) {
            result.push_back(prototype->getMetadata());
        }
    }
    
    return result;
}

AvailableStepsDto StepTypeRegistry::availableTypesAsDto() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    AvailableStepsDto dto;
    dto.steps.reserve(m_prototypes.size());
    
    for (const auto& [id, prototype] : m_prototypes) {
        if (!prototype) continue;
        
        StepMetadata metadata = prototype->getMetadata();
        StepMetadataDto stepDto;
        
        // Basis-Informationen
        stepDto.typeId = std::to_string(metadata.typeId);
        stepDto.displayName = std::string(metadata.displayName);
        stepDto.description = std::string(metadata.description);
        stepDto.category = "general"; // TODO: Aus Metadata oder Config
        
        // Parameter konvertieren
        for (const auto& param : metadata.params) {
            ParameterMetadataDto paramDto;
            paramDto.name = std::string(param.key);
            
            // Typ aus IValue extrahieren
            if (param.value) {
                switch (param.value->kind()) {
                    case ValueKind::Int:   paramDto.type = "int"; break;
                    case ValueKind::Float: paramDto.type = "float"; break;
                    case ValueKind::Bool:  paramDto.type = "bool"; break;
                    case ValueKind::String: paramDto.type = "string"; break;
                    default: paramDto.type = "unknown"; break;
                }
                
                // Default-Wert als String
                int intVal;
                float floatVal;
                bool boolVal;
                const char* strVal;
                
                if (param.value->get(intVal)) {
                    paramDto.defaultValue = std::to_string(intVal);
                } else if (param.value->get(floatVal)) {
                    paramDto.defaultValue = std::to_string(floatVal);
                } else if (param.value->get(boolVal)) {
                    paramDto.defaultValue = boolVal ? "true" : "false";
                } else if (param.value->get(strVal)) {
                    paramDto.defaultValue = strVal;
                }
            }
            
            // Min-Wert
            if (param.minValue && *param.minValue) {
                int intVal;
                float floatVal;
                if ((*param.minValue)->get(intVal)) {
                    paramDto.minValue = std::to_string(intVal);
                } else if ((*param.minValue)->get(floatVal)) {
                    paramDto.minValue = std::to_string(floatVal);
                }
            }
            
            // Max-Wert
            if (param.maxValue && *param.maxValue) {
                int intVal;
                float floatVal;
                if ((*param.maxValue)->get(intVal)) {
                    paramDto.maxValue = std::to_string(intVal);
                } else if ((*param.maxValue)->get(floatVal)) {
                    paramDto.maxValue = std::to_string(floatVal);
                }
            }
            
            paramDto.description = std::string(param.description);
            paramDto.required = !param.minValue.has_value(); // Heuristik: Wenn kein Min → required
            paramDto.unit = std::string(param.label); // Label oft als Unit genutzt
            
            stepDto.parameters.push_back(std::move(paramDto));
        }
        
        // IoAliases konvertieren
        for (const auto& alias : metadata.ioAliases) {
            IoAliasMetadataDto ioDto;
            ioDto.aliasName = std::string(alias.aliasName);
            
            // ioType bestimmen
            if (alias.isSensor) {
                ioDto.ioType = "sensor";
            } else if (alias.isInput && alias.isOutput) {
                ioDto.ioType = "input-output"; // Bidirektional
            } else if (alias.isInput) {
                ioDto.ioType = "input";
            } else if (alias.isOutput) {
                ioDto.ioType = "output";
            } else {
                ioDto.ioType = "unknown";
            }
            
            // valueType aus exampleValue
            if (alias.exampleValue && *alias.exampleValue) {
                switch ((*alias.exampleValue)->kind()) {
                    case ValueKind::Bool:  ioDto.valueType = "bool"; break;
                    case ValueKind::Int:   ioDto.valueType = "int"; break;
                    case ValueKind::Float: ioDto.valueType = "float"; break;
                    case ValueKind::String: ioDto.valueType = "string"; break;
                    default: ioDto.valueType = "unknown"; break;
                }
            } else {
                ioDto.valueType = "unknown";
            }
            
            ioDto.description = ""; // TODO: Falls in IoAliasDef vorhanden
            
            stepDto.ioAliases.push_back(std::move(ioDto));
        }
        
        dto.steps.push_back(std::move(stepDto));
    }
    
    return dto;
}

std::unique_ptr<IStep> StepTypeRegistry::createInstance(uint32_t typeId) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    
    auto it = m_prototypes.find(typeId);
    if (it == m_prototypes.end() || !it->second) {
        return nullptr;
    }
    
    return it->second->cloneEmpty();
}

// HINWEIS: init() wird ABSICHTLICH NICHT hier implementiert!
// Diese Methode muss vom Entwickler in example_implementations/recipemanagement/init/InitSteps.cc
// implementiert werden. Wenn sie fehlt, gibt der Linker einen Fehler aus.
