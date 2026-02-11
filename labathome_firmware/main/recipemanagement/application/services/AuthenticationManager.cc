#include "AuthenticationManager.hh"
#include "StorageManager.hh"
#include <esp_log.h>
#include <esp_random.h>
#include <sstream>
#include <iomanip>

static const char* TAG = "AuthManager";

// Storage keys for 4-digit PINs
static const char* KEY_PIN_ADMIN = "pin_admin";
static const char* KEY_PIN_EDITOR = "pin_editor";
static const char* KEY_PIN_STARTER = "pin_starter";
static const char* KEY_PIN_OBSERVER = "pin_observer";

AuthenticationManager::AuthenticationManager(StorageManager* storageManager)
    : m_storageManager(storageManager)
{
    loadPins();
}

AuthenticationManager::~AuthenticationManager()
{
}

void AuthenticationManager::setDefaultPins()
{
    m_pins[UserRole::Admin] = "0000";
    m_pins[UserRole::RecipeEditor] = "1111";
    m_pins[UserRole::RecipeStarter] = "2222";
    m_pins[UserRole::Observer] = "3333";
    ESP_LOGI(TAG, "Set default PINs");
}

void AuthenticationManager::loadPins()
{
    ESP_LOGI(TAG, "loadPins() called, storageManager=%p", m_storageManager);
    
    if (!m_storageManager)
    {
        ESP_LOGW(TAG, "No storage manager, using defaults");
        setDefaultPins();
        return;
    }
    
    auto adminPin = m_storageManager->getAuthPassword(KEY_PIN_ADMIN);
    auto editorPin = m_storageManager->getAuthPassword(KEY_PIN_EDITOR);
    auto starterPin = m_storageManager->getAuthPassword(KEY_PIN_STARTER);
    auto observerPin = m_storageManager->getAuthPassword(KEY_PIN_OBSERVER);
    
    if (adminPin.has_value() && editorPin.has_value() && 
        starterPin.has_value() && observerPin.has_value())
    {
        m_pins[UserRole::Admin] = adminPin.value();
        m_pins[UserRole::RecipeEditor] = editorPin.value();
        m_pins[UserRole::RecipeStarter] = starterPin.value();
        m_pins[UserRole::Observer] = observerPin.value();
        ESP_LOGI(TAG, "Loaded PINs from storage - Admin:%s Editor:%s Starter:%s Observer:%s", 
                 m_pins[UserRole::Admin].c_str(),
                 m_pins[UserRole::RecipeEditor].c_str(),
                 m_pins[UserRole::RecipeStarter].c_str(),
                 m_pins[UserRole::Observer].c_str());
    }
    else
    {
        setDefaultPins();
        savePins();
        ESP_LOGI(TAG, "No stored PINs, using and saving defaults");
    }
}

void AuthenticationManager::savePins()
{
    if (!m_storageManager)
    {
        ESP_LOGW(TAG, "Cannot save PINs, no storage manager");
        return;
    }
    
    m_storageManager->saveAuthPassword(KEY_PIN_ADMIN, m_pins[UserRole::Admin]);
    m_storageManager->saveAuthPassword(KEY_PIN_EDITOR, m_pins[UserRole::RecipeEditor]);
    m_storageManager->saveAuthPassword(KEY_PIN_STARTER, m_pins[UserRole::RecipeStarter]);
    m_storageManager->saveAuthPassword(KEY_PIN_OBSERVER, m_pins[UserRole::Observer]);
    
    ESP_LOGI(TAG, "Saved PINs to storage");
}

bool AuthenticationManager::isPinCorrectForRole(const std::string& pin, UserRole role)
{
    if (pin.empty())
    {
        ESP_LOGD(TAG, "isPinCorrectForRole: empty pin");
        return false;
    }
    
    bool isCorrect = (m_pins[role] == pin);
    
    const char* roleNames[] = {"Observer", "RecipeStarter", "RecipeEditor", "Admin"};
    ESP_LOGI(TAG, "isPinCorrectForRole: Checking PIN for role '%s': %s",
             roleNames[static_cast<int>(role)],
             isCorrect ? "CORRECT" : "INCORRECT");
    
    return isCorrect;
}

std::string AuthenticationManager::generateToken()
{
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (int i = 0; i < 4; i++) {
        ss << std::setw(8) << esp_random();
    }
    return ss.str();
}

std::string AuthenticationManager::login(const std::string& pin, UserRole role)
{
    const char* roleNames[] = {"Observer", "RecipeStarter", "RecipeEditor", "Admin"};
    
    ESP_LOGI(TAG, "login() called for role='%s' with pin='%s' (length=%d)",
             roleNames[static_cast<int>(role)], pin.c_str(), pin.length());
    
    if (pin.length() != 4)
    {
        ESP_LOGW(TAG, "Login failed: Invalid PIN length: %d (expected 4)", pin.length());
        return "";
    }
    
    if (!isPinCorrectForRole(pin, role))
    {
        ESP_LOGW(TAG, "Login failed: Incorrect PIN for role '%s'", roleNames[static_cast<int>(role)]);
        return "";
    }
    
    std::string token = generateToken();
    m_sessions[token] = role;
    
    ESP_LOGI(TAG, "Login successful for role '%s', token issued (length=%d)",
             roleNames[static_cast<int>(role)], token.length());
    return token;
}

void AuthenticationManager::logout(const std::string& sessionToken)
{
    auto it = m_sessions.find(sessionToken);
    if (it != m_sessions.end())
    {
        ESP_LOGI(TAG, "Logout for role %d", static_cast<int>(it->second));
        m_sessions.erase(it);
    }
}

UserRole AuthenticationManager::validateToken(const std::string& sessionToken)
{
    auto it = m_sessions.find(sessionToken);
    if (it != m_sessions.end())
    {
        return it->second;
    }
    return UserRole::Observer;
}

bool AuthenticationManager::hasPermission(const std::string& sessionToken, UserRole requiredRole)
{
    UserRole userRole = validateToken(sessionToken);
    return static_cast<int>(userRole) >= static_cast<int>(requiredRole);
}

bool AuthenticationManager::changePin(const std::string& adminToken, UserRole role, const std::string& oldPin, const std::string& newPin)
{
    const char* roleNames[] = {"Observer", "RecipeStarter", "RecipeEditor", "Admin"};
    
    ESP_LOGI(TAG, "changePin() called for role='%s'", roleNames[static_cast<int>(role)]);
    
    if (!hasPermission(adminToken, UserRole::Admin))
    {
        ESP_LOGW(TAG, "PIN change denied: Unauthorized attempt (token validation failed)");
        return false;
    }
    
    if (newPin.length() != 4)
    {
        ESP_LOGW(TAG, "PIN change failed: Invalid new PIN length: %d (expected 4)", newPin.length());
        return false;
    }
    
    if (m_pins[role] != oldPin)
    {
        ESP_LOGW(TAG, "PIN change failed: Old PIN incorrect for role '%s'",
                 roleNames[static_cast<int>(role)]);
        return false;
    }
    
    std::string oldPinMasked = std::string(oldPin.length(), '*');
    std::string newPinMasked = std::string(newPin.length(), '*');
    
    m_pins[role] = newPin;
    savePins();
    
    ESP_LOGI(TAG, "PIN successfully changed for role '%s' (old: %s -> new: %s)",
             roleNames[static_cast<int>(role)],
             oldPinMasked.c_str(),
             newPinMasked.c_str());
    
    return true;
}
