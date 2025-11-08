#include "TestRecipeExecution.hh"
#include "../../../recipemanagement/infrastructure/parsers/RecipeParser.hh"
#include "../../../recipemanagement/infrastructure/engine/RecipeEngine.hh"
#include "../../../recipemanagement/infrastructure/engine/StepTypeRegistry.hh"
#include "../../../recipemanagement/infrastructure/engine/IoResourceManager.hh"
#include <esp_log.h>

static const char* TAG = "TestRecipeExecution";

// Hardcodiertes Test-JSON (aus test_recipe.json)
static const char* TEST_RECIPE_JSON = R"({
  "id": "test_recipe_001",
  "name": "LED Button Test Recipe",
  "description": "Test recipe with 3 LED+Button steps",
  "version": "1.0",
  "author": "System",
  "steps": [
    {
      "stepTypeId": "0x0001",
      "systemId": "step_red_led",
      "parameters": {},
      "aliases": {
        "LED": "LED0",
        "RedButton": "RedButton"
      },
      "repeatCount": 1
    },
    {
      "stepTypeId": "0x0002",
      "systemId": "step_yellow_green",
      "parameters": {},
      "aliases": {
        "LED": "LED1",
        "GreenButton": "GreenButton"
      },
      "repeatCount": 1
    },
    {
      "stepTypeId": "0x0003",
      "systemId": "step_two_leds",
      "parameters": {},
      "aliases": {
        "LEDRed": "LED2",
        "LEDGreen": "LED3",
        "RedButton": "RedButton",
        "GreenButton": "GreenButton"
      },
      "repeatCount": 1
    }
  ]
})";

TestRecipeExecution::TestRecipeExecution() 
    : m_engine(nullptr), m_isRunning(false) {
}

TestRecipeExecution::~TestRecipeExecution() {
    stop();
}

bool TestRecipeExecution::init(iHAL* hal) {
    if (!hal) {
        ESP_LOGE(TAG, "HAL is null, cannot initialize");
        return false;
    }
    
    ESP_LOGI(TAG, "Initializing Recipe Management System...");
    
    // 1. Initialize I/O Resource Manager
    ESP_LOGI(TAG, "Initializing IoResourceManager...");
    IoResourceManager::instance().init(hal);
    
    // 2. Initialize Step Type Registry
    ESP_LOGI(TAG, "Initializing StepTypeRegistry...");
    StepTypeRegistry::instance().init();
    
    // 3. Create RecipeEngine
    ESP_LOGI(TAG, "Creating RecipeEngine...");
    m_engine = std::make_unique<RecipeEngine>();
    
    ESP_LOGI(TAG, "Recipe Management System initialized successfully");
    return true;
}

bool TestRecipeExecution::loadAndStart() {
    if (!m_engine) {
        ESP_LOGE(TAG, "RecipeEngine not initialized");
        return false;
    }
    
    ESP_LOGI(TAG, "========================================");
    ESP_LOGI(TAG, "Starting Test Recipe Execution");
    ESP_LOGI(TAG, "========================================");
    
    // 1. Parse JSON to StepInstanceDescriptors
    ESP_LOGI(TAG, "Parsing test recipe JSON...");
    RecipeParser parser;
    std::vector<StepInstanceDescriptor> steps;
    
    if (!parser.parseJsonToStepDescriptors(TEST_RECIPE_JSON, steps)) {
        ESP_LOGE(TAG, "Failed to parse recipe JSON");
        return false;
    }
    
    ESP_LOGI(TAG, "Successfully parsed %zu steps", steps.size());
    
    // 2. Load recipe into engine
    ESP_LOGI(TAG, "Loading recipe into engine...");
    if (!m_engine->loadRecipe(steps, "test_recipe_001")) {
        ESP_LOGE(TAG, "Failed to load recipe into engine");
        return false;
    }
    
    ESP_LOGI(TAG, "Recipe loaded successfully");
    
    // 3. Start execution
    ESP_LOGI(TAG, "Starting recipe execution...");
    if (!m_engine->start()) {
        ESP_LOGE(TAG, "Failed to start recipe execution");
        return false;
    }
    
    m_isRunning = true;
    ESP_LOGI(TAG, "Recipe execution started!");
    ESP_LOGI(TAG, "========================================");
    
    return true;
}

void TestRecipeExecution::tick(uint32_t deltaMs) {
    if (m_engine && m_isRunning) {
        m_engine->tick(deltaMs);
        
        // Check if recipe is complete
        auto state = m_engine->getState();
        if (state == RecipeEngineState::Idle) {
            ESP_LOGI(TAG, "========================================");
            ESP_LOGI(TAG, "Recipe execution completed!");
            ESP_LOGI(TAG, "========================================");
            m_isRunning = false;
        } else if (state == RecipeEngineState::Error) {
            ESP_LOGE(TAG, "========================================");
            ESP_LOGE(TAG, "Recipe execution error!");
            ESP_LOGE(TAG, "========================================");
            m_isRunning = false;
        }
    }
}

void TestRecipeExecution::stop() {
    if (m_engine && m_isRunning) {
        ESP_LOGI(TAG, "Stopping recipe execution...");
        m_engine->stop();
        m_isRunning = false;
    }
}

bool TestRecipeExecution::isRunning() const {
    return m_isRunning;
}

RecipeEngineState TestRecipeExecution::getState() const {
    if (m_engine) {
        return m_engine->getState();
    }
    return RecipeEngineState::Idle;
}
