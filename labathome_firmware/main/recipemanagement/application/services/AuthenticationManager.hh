#pragma once

#include <string>
#include <map>
#include <cstdint>

enum class UserRole {
    Observer = 0,      // Read-only access
    RecipeStarter = 1, // Can start/stop/pause recipes
    RecipeEditor = 2,  // Can create/edit/delete recipes + start them
    Admin = 3          // Full access including PIN changes
};

class StorageManager;

class AuthenticationManager {
public:
    explicit AuthenticationManager(StorageManager* storageManager);
    ~AuthenticationManager();
    
    // Login: validates 4-digit PIN for specified role, returns session token
    std::string login(const std::string& pin, UserRole role);
    
    // Logout: invalidates session token
    void logout(const std::string& sessionToken);
    
    // Validates session token, returns role or Observer on fail
    UserRole validateToken(const std::string& sessionToken);
    
    // Checks if token has required role (hierarchical)
    bool hasPermission(const std::string& sessionToken, UserRole requiredRole);
    
    // Changes PIN for role (Admin only via token)
    bool changePin(const std::string& adminToken, UserRole role, const std::string& oldPin, const std::string& newPin);
    
    // Loads PINs from storage or sets defaults
    void loadPins();
    
    // Saves PINs to storage
    void savePins();

private:
    StorageManager* m_storageManager;
    std::map<UserRole, std::string> m_pins;  // 4-digit PINs
    std::map<std::string, UserRole> m_sessions;  // token -> role
    
    void setDefaultPins();
    std::string generateToken();
    bool isPinCorrectForRole(const std::string& pin, UserRole role);
    
    AuthenticationManager(const AuthenticationManager&) = delete;
    AuthenticationManager& operator=(const AuthenticationManager&) = delete;
};
