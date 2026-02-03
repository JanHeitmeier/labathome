#pragma once

#include <memory>
#include <unordered_map>
#include <vector>
#include <functional>
#include "../../core/domain/value-objects/StepMetadata.hh"
#include "../../core/interfaces/engine/IStep.hh"

using StepFactory = std::function<std::unique_ptr<IStep>()>;


class StepTypeRegistry {
private:
    struct StepTypeInfo {
        StepFactory factory;
        StepMetadata metadata;
    };
    std::unordered_map<uint32_t, StepTypeInfo> m_types;
    uint32_t m_nextTypeId = 0x0001;
    
    StepTypeRegistry() = default;
    
public:
    static StepTypeRegistry& instance();
    void init();

    template<typename StepType>
    void registerStepType() {
        uint32_t assignedTypeId = m_nextTypeId++;
        auto temp = std::make_unique<StepType>();
        temp->setTypeId(assignedTypeId);
         StepMetadata metadata = temp->getMetadata();
        StepFactory factory = [assignedTypeId]() -> std::unique_ptr<IStep> {
            auto instance = std::make_unique<StepType>();
            instance->setTypeId(assignedTypeId);
            return instance;
        };
        
        m_types[assignedTypeId] = StepTypeInfo{std::move(factory), std::move(metadata)};
    }

    std::vector<StepMetadata> availableTypes() const;
 
    std::unique_ptr<IStep> createInstance(uint32_t typeId) const;
};
