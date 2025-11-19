#pragma once

#include <string>
#include <string_view>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <queue>
#include <chrono>
#include "StepMetadata.hh"
#include "ParameterValue.hh"
#include "../../interfaces/engine/IInput.hh"
#include "../../interfaces/engine/IOutput.hh"
#include "../../../infrastructure/engine/IoResourceManager.hh"

class StepContext {
public:
    StepContext(StepMetadata metadata, IoResourceManager& ioManager);
    
    // Parameter access
    ParameterValue* getParam(std::string_view key);
    
    // IO access
    std::shared_ptr<IInput> getInput(std::string_view alias);
    std::shared_ptr<IOutput> getOutput(std::string_view alias);
    
    // Event logging
    void log(std::string_view message);
    //Die Eventqueue ermöglicht das asncrone ausgeben von nachrichten und bestätigungen
    std::queue<std::string>& getEventQueue();
    
    // Timer management
    void startTimer(std::string_view id, std::chrono::milliseconds duration);
    void stopTimer(std::string_view id);
    bool isTimerExpired(std::string_view id) const;
    
    // State management
    bool isActive() const;
    void setActive(bool active);
    
    // User acknowledgment - Developer-friendly API
    void requestUserAcknowledgment(std::string_view instruction);
    bool isAwaitingAcknowledgment() const;
    bool isAcknowledged() const;
    void acknowledge();
    std::string getUserInstruction() const;
    
    // Mutex access for thread-safety
    std::mutex& getMutex();

private:
    StepMetadata m_metadata; // Copy instead of reference - persists with context
    IoResourceManager& m_ioManager;
    std::unordered_map<std::string, ParameterValue> m_params;
    std::unordered_map<std::string, std::pair<std::chrono::steady_clock::time_point, std::chrono::milliseconds>> m_timers;
    std::queue<std::string> m_eventQueue;
    mutable std::mutex m_mutex;
    bool m_isActive;
    
    // User acknowledgment state (managed by Engine)
    bool m_awaitingAcknowledgment{false};
    bool m_acknowledged{false};
    std::string m_userInstruction;
    
    // Allow Engine to restore acknowledged state
    friend class RecipeEngine;
    void setAcknowledgedState(bool acked) { m_acknowledged = acked; }
};
