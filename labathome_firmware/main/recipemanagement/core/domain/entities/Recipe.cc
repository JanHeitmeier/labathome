// Recipe.cpp
#include "Recipe.hh"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <esp_log.h>

static const char* TAG = "RecipeEntity";

// Konstruktor mit ID und Name
Recipe::Recipe(const std::string& id, const std::string& name)
	: id_(id), name_(name) {
	// TODO: implement - ggf. Validierungen oder Default-Werte setzen
}

// basic accessors
void Recipe::setId(const std::string& id){
	// TODO: implement - ggf. überprüfen auf gültige ID-Formate
	id_ = id;
}

std::string Recipe::id() const{
	// TODO: implement - evtl. kopieren oder referenzieren
	return id_;
}

void Recipe::setName(const std::string& name){
	// TODO: implement - evtl. trim/validate
	name_ = name;
}

std::string Recipe::name() const{
	// TODO: implement - evtl. lokalisierung
	return name_;
}

void Recipe::setDescription(const std::string& d){
	// TODO: implement - evtl. Sanitizing
	description_ = d;
}

std::string Recipe::description() const{
	// TODO: implement - ggf. shorten
	return description_;
}

void Recipe::setVersion(const std::string& v){
	// TODO: implement - Validierung des Version-Strings
	version_ = v;
}

std::string Recipe::version() const{
	// TODO: implement - evtl. SemVer-Prüfung
	return version_;
}

// step manipulation
void Recipe::setSteps(const std::vector<StepInstanceDescriptor>& steps){
	// TODO: implement - prüfen, kopieren, ggf. deep-copy
	steps_ = steps;
}

const std::vector<StepInstanceDescriptor>& Recipe::steps() const{
	// TODO: implement - ggf. defensive Kopie erwägen
	return steps_;
}

void Recipe::addStep(const StepInstanceDescriptor& s){
	// TODO: implement - Validierung oder Insert-Position
	steps_.push_back(s);
}

void Recipe::clearSteps(){
	// TODO: implement - evtl. notify/cleanup
	steps_.clear();
}
