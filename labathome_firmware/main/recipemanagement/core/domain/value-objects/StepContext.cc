#include "StepContext.hh"

StepContext::StepContext(const StepMetadata& metadata, IoResourceManager& ioManager)
    : m_metadata(metadata), m_ioManager(ioManager), m_isActive(false) {
    for (const auto& param : metadata.params) {
        if (!param.key.empty() && param.value) {
            m_params[param.key] = param.value->clone();
        }
    }
}

std::unique_ptr<IValue>* StepContext::getParam(std::string_view key) {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_params.find(key);
    return it != m_params.end() ? &it->second : nullptr;
}

std::shared_ptr<IInput> StepContext::getInput(std::string_view alias) {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_ioManager.resolveInput(alias);
}

std::shared_ptr<IOutput> StepContext::getOutput(std::string_view alias) {
    std::lock_guard<std::mutex> lk(m_mutex);
    return m_ioManager.resolveOutput(alias);
}

void StepContext::log(std::string_view message) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_eventQueue.push(message);
}

std::queue<std::string_view>& StepContext::getEventQueue() {
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
    m_timers[id] = {std::chrono::steady_clock::now(), duration};
}

void StepContext::stopTimer(std::string_view id) {
    std::lock_guard<std::mutex> lk(m_mutex);
    m_timers.erase(id);
}

bool StepContext::isTimerExpired(std::string_view id) const {
    std::lock_guard<std::mutex> lk(m_mutex);
    auto it = m_timers.find(id);
    if (it == m_timers.end()) return false;
    auto start = it->second.first;
    auto dur = it->second.second;
    return (std::chrono::steady_clock::now() - start) >= dur;
}
