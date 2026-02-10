#include "AuthenticationManager.hh"
#include "StorageManager.hh"
#include <esp_log.h>

static const char* TAG = "AuthManager";

// Storage keys for passwords
static const char* KEY_PW_ADMIN = "pw_admin";
static const char* KEY_PW_EDITOR = "pw_editor";
static const char* KEY_PW_STARTER = "pw_starter";
static const char* KEY_PW_OBSERVER = "pw_observer";

AuthenticationManager::AuthenticationManager(StorageManager* storageManager)
    : m_storageManager(storageManager)
{
    loadPasswords();
}

AuthenticationManager::~AuthenticationManager()
{
}

void AuthenticationManager::setDefaultPasswords()
{
    m_passwords[UserRole::Admin] = "admin";
    m_passwords[UserRole::RecipeEditor] = "editor";
    m_passwords[UserRole::RecipeStarter] = "starter";
    m_passwords[UserRole::Observer] = "observer";
    ESP_LOGI(TAG, "Set default passwords");
}

void AuthenticationManager::loadPasswords()
{
    if (!m_storageManager)
    {
        ESP_LOGW(TAG, "No storage manager, using defaults");
        setDefaultPasswords();
        return;
    }
    
    // Try to load passwords from storage
    auto adminPw = m_storageManager->getAuthPassword(KEY_PW_ADMIN);
    auto editorPw = m_storageManager->getAuthPassword(KEY_PW_EDITOR);
    auto starterPw = m_storageManager->getAuthPassword(KEY_PW_STARTER);
    auto observerPw = m_storageManager->getAuthPassword(KEY_PW_OBSERVER);
    
    if (adminPw.has_value() && editorPw.has_value() && 
        starterPw.has_value() && observerPw.has_value())
    {
        m_passwords[UserRole::Admin] = adminPw.value();
        m_passwords[UserRole::RecipeEditor] = editorPw.value();
        m_passwords[UserRole::RecipeStarter] = starterPw.value();
        m_passwords[UserRole::Observer] = observerPw.value();
        ESP_LOGI(TAG, "Loaded passwords from storage");
    }
    else
    {
        // No stored passwords, use and save defaults
        setDefaultPasswords();
        savePasswords();
    }
}

void AuthenticationManager::savePasswords()
{
    if (!m_storageManager)
    {
        ESP_LOGW(TAG, "Cannot save passwords, no storage manager");
        return;
    }
    
    m_storageManager->saveAuthPassword(KEY_PW_ADMIN, m_passwords[UserRole::Admin]);
    m_storageManager->saveAuthPassword(KEY_PW_EDITOR, m_passwords[UserRole::RecipeEditor]);
    m_storageManager->saveAuthPassword(KEY_PW_STARTER, m_passwords[UserRole::RecipeStarter]);
    m_storageManager->saveAuthPassword(KEY_PW_OBSERVER, m_passwords[UserRole::Observer]);
    
    ESP_LOGI(TAG, "Saved passwords to storage");
}

UserRole AuthenticationManager::getRoleForPassword(const std::string& password)
{
    if (password.empty())
    {
        return UserRole::Observer;
    }
    
    // Check from highest to lowest role
    if (m_passwords[UserRole::Admin] == password)
        return UserRole::Admin;
    if (m_passwords[UserRole::RecipeEditor] == password)
        return UserRole::RecipeEditor;
    if (m_passwords[UserRole::RecipeStarter] == password)
        return UserRole::RecipeStarter;
    if (m_passwords[UserRole::Observer] == password)
        return UserRole::Observer;
    
    return UserRole::Observer; // Default to lowest role on invalid password
}

bool AuthenticationManager::validatePassword(const std::string& password, UserRole requiredRole)
{
    UserRole userRole = getRoleForPassword(password);
    
    // Hierarchical check: higher roles can access lower role functions
    return static_cast<int>(userRole) >= static_cast<int>(requiredRole);
}

bool AuthenticationManager::changePassword(UserRole role, const std::string& oldPassword, const std::string& newPassword)
{
    if (newPassword.empty())
    {
        ESP_LOGW(TAG, "Cannot set empty password");
        return false;
    }
    
    // Verify old password matches
    if (m_passwords[role] != oldPassword)
    {
        ESP_LOGW(TAG, "Old password incorrect for role %d", static_cast<int>(role));
        return false;
    }
    
    m_passwords[role] = newPassword;
    savePasswords();
    
    ESP_LOGI(TAG, "Password changed for role %d", static_cast<int>(role));
    return true;
}

bool AuthenticationManager::resetToDefaults(const std::string& adminPassword)
{
    if (m_passwords[UserRole::Admin] != adminPassword)
    {
        ESP_LOGW(TAG, "Invalid admin password for reset");
        return false;
    }
    
    setDefaultPasswords();
    savePasswords();
    
    ESP_LOGI(TAG, "All passwords reset to defaults");
    return true;
}
