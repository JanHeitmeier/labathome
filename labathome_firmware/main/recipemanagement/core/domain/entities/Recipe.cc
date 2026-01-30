// Recipe.cpp
#include "Recipe.hh"
#include <sstream>
#include <algorithm>
#include <cctype>

// Konstruktor mit ID und Name
Recipe::Recipe(const std::string& id, const std::string& name)
	: id_(id), name_(name) {
}

// basic accessors
void Recipe::setId(const std::string& id){
	id_ = id;
}

std::string Recipe::id() const{
	return id_;
}

void Recipe::setName(const std::string& name){
	name_ = name;
}

std::string Recipe::name() const{
	return name_;
}

void Recipe::setDescription(const std::string& d){
	description_ = d;
}

std::string Recipe::description() const{
	return description_;
}

void Recipe::setVersion(const std::string& v){
	version_ = v;
}

std::string Recipe::version() const{
	return version_;
}

// step manipulation
void Recipe::setSteps(const std::vector<StepInstanceDescriptor>& steps){
	steps_ = steps;
}

const std::vector<StepInstanceDescriptor>& Recipe::steps() const{
	return steps_;
}

void Recipe::addStep(const StepInstanceDescriptor& s){
	steps_.push_back(s);
}

void Recipe::clearSteps(){
	steps_.clear();
}
