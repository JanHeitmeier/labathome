#pragma once

#include <string>
#include <map>

enum class UserRole {
    Observer = 0,      // Read-only access (no password needed for get_* commands)
    RecipeStarter = 1, // Can start/stop/pause recipes
    RecipeEditor = 2,  // Can create/edit/delete recipes + start them
    Admin = 3          // Full access including password changes
};

class StorageManager; // Forward declaration

class AuthenticationManager {
public:
    explicit AuthenticationManager(StorageManager* storageManager);
    ~AuthenticationManager();
    
    /**
     * Validates password and checks if it meets required role level.
     * Admin role can access everything (hierarchical).
     * @param password The password to validate
     * @param requiredRole Minimum role required
     * @return true if password is valid and role sufficient
     */
    bool validatePassword(const std::string& password, UserRole requiredRole);
    
    /**
     * Gets the role for a given password.
     * @param password The password to check
     * @return UserRole or Observer if password invalid
     */
    UserRole getRoleForPassword(const std::string& password);
    
    /**
     * Changes password for a specific role.
     * Requires the old password to be correct.
     * @param role The role to change password for
     * @param oldPassword Current password
     * @param newPassword New password
     * @return true if change successful
     */
    bool changePassword(UserRole role, const std::string& oldPassword, const std::string& newPassword);
    
    /**
     * Loads passwords from storage or sets defaults.
     */
    void loadPasswords();
    
    /**
     * Saves passwords to storage.
     */
    void savePasswords();
    
    /**
     * Resets all passwords to defaults.
     * Requires admin password.
     * @param adminPassword Current admin password
     * @return true if reset successful
     */
    bool resetToDefaults(const std::string& adminPassword);

private:
    StorageManager* m_storageManager;
    std::map<UserRole, std::string> m_passwords;
    
    void setDefaultPasswords();
    
    AuthenticationManager(const AuthenticationManager&) = delete;
    AuthenticationManager& operator=(const AuthenticationManager&) = delete;
};
