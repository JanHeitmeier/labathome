#include "StepContext.hh"

StepContext::StepContext(const StepMetadata& metadata, IoResourceManager& ioManager)
    : m_metadata(metadata), m_ioManager(ioManager), m_isActive(false) {
    for (const auto& param : metadata.params) {
        if (!param.key.empty()) {
            m_params[param.key] = param.value;
        }
    }
}

ParameterValue* StepContext::getParam(std::string_view key) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_params.find(std::string(key));
    return it != m_params.end() ? &it->second : nullptr;
}

std::shared_ptr<IInput> StepContext::getInput(std::string_view alias) {
    std::lock_guard<std::mutex> lk(m_mutex);
    // Find the physical name for this alias
    for (const auto& ioAlias : m_metadata.ioAliases) {
        if (ioAlias.aliasName == alias && ioAlias.isInput) {
            // Use physicalName if set, otherwise fall back to aliasName
            const std::string& name = ioAlias.physicalName.empty() ? ioAlias.aliasName : ioAlias.physicalName;
            return m_ioManager.resolveInput(name);
        }
    }
    // Fallback: try alias directly
    return m_ioManager.resolveInput(std::string(alias));
}

std::shared_ptr<IOutput> StepContext::getOutput(std::string_view alias) {
    std::lock_guard<std::mutex> lk(m_mutex);
    // Find the physical name for this alias
    for (const auto& ioAlias : m_metadata.ioAliases) {
        if (ioAlias.aliasName == alias && ioAlias.isOutput) {
            // Use physicalName if set, otherwise fall back to aliasName
            const std::string& name = ioAlias.physicalName.empty() ? ioAlias.aliasName : ioAlias.physicalName;
            return m_ioManager.resolveOutput(name);
        }
    }
    // Fallback: try alias directly
    return m_ioManager.resolveOutput(std::string(alias));
}

void StepContext::log(std::string_view message) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_eventQueue.push(std::string(message));
}

std::queue<std::string>& StepContext::getEventQueue() {
    return m_eventQueue;
}

std::mutex& StepContext::getMutex() {
    return m_mutex;
}

bool StepContext::isActive() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_isActive;
}

void StepContext::setActive(bool active) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_isActive = active;
}

void StepContext::startTimer(std::string_view id, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_timers[std::string(id)] = {std::chrono::steady_clock::now(), duration};
}

void StepContext::stopTimer(std::string_view id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_timers.erase(std::string(id));
}

bool StepContext::isTimerExpired(std::string_view id) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_timers.find(std::string(id));
    if (it == m_timers.end()) return false;
    auto start = it->second.first;
    auto dur = it->second.second;
    return (std::chrono::steady_clock::now() - start) >= dur;
}

void StepContext::requestUserAcknowledgment(std::string_view instruction) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_awaitingAcknowledgment = true;
    m_acknowledged = false;
    m_userInstruction = instruction;
}

bool StepContext::isAwaitingAcknowledgment() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_awaitingAcknowledgment;
}

bool StepContext::isAcknowledged() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_acknowledged;
}

void StepContext::acknowledge() {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_awaitingAcknowledgment = false;
    m_acknowledged = true;
}

std::string StepContext::getUserInstruction() const {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_userInstruction;
}
