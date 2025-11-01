// Recipe.hpp
#pragma once

#include <string>
#include <vector>
#include <optional>

class Recipe {
public:
    Recipe() = default;
    Recipe(const std::string& id, const std::string& name);

    // basic accessors
    void setId(const std::string& id);
    std::string id() const;
    void setName(const std::string& name);
    std::string name() const;
    void setDescription(const std::string& d);
    std::string description() const;
    void setVersion(const std::string& v);
    std::string version() const;

    // step manipulation
    void setSteps(const std::vector<StepInstanceDescriptor>& steps);
    const std::vector<StepInstanceDescriptor>& steps() const;
    void addStep(const StepInstanceDescriptor& s);
    void clearSteps();

private:
    std::string id_;
    std::string name_;
    std::string description_;
    std::string version_;
    std::vector<StepInstanceDescriptor> steps_;
};
