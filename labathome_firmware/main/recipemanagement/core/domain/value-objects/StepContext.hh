#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <queue>
#include <chrono>
#include "StepMetadata.hh"
#include "ParameterValue.hh"
#include "../../interfaces/engine/IInput.hh"
#include "../../interfaces/engine/IOutput.hh"
#include "../../services/IoResourceManager.hh"

class StepContext {
public:
    StepContext(StepMetadata metadata, IoResourceManager& ioManager);
    
    ParameterValue* getParam(std::string_view key);
    
    std::shared_ptr<IInput> getInput(std::string_view alias);
    std::shared_ptr<IOutput> getOutput(std::string_view alias);
    
    void log(std::string_view message);
    std::queue<std::string>& getEventQueue();
    
    void startTimer(std::string_view id, std::chrono::milliseconds duration);
    void stopTimer(std::string_view id);
    bool isTimerExpired(std::string_view id) const;
    
    bool isActive() const;
    void setActive(bool active);
    
    void requestUserAcknowledgment(std::string_view instruction);
    bool isAwaitingAcknowledgment() const;
    bool isAcknowledged() const;
    void acknowledge();
    std::string getUserInstruction() const;
    
    const StepMetadata& getMetadata() const { return m_metadata; }

private:
    StepMetadata m_metadata;
    IoResourceManager& m_ioManager;
    std::unordered_map<std::string, ParameterValue> m_params;
    std::unordered_map<std::string, std::pair<std::chrono::steady_clock::time_point, std::chrono::milliseconds>> m_timers;
    std::queue<std::string> m_eventQueue;
    bool m_isActive;
    
    bool m_awaitingAcknowledgment{false};
    bool m_acknowledged{false};
    std::string m_userInstruction;
    
    friend class RecipeEngine;
    void setAcknowledgedState(bool acked) { 
        m_acknowledged = acked;
        m_awaitingAcknowledgment = false;
    }
};
