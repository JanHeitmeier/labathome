# Recipe Management Framework - ESP-IDF Komponente
# Wird als eigenständige Komponente in EXTRA_COMPONENT_DIRS registriert

idf_component_register(
    SRCS 
        # === Core Domain ===
        "core/domain/entities/Recipe.cc"
        "core/domain/value-objects/StepContext.cc"
        
        # === Infrastructure - Engine ===
        "infrastructure/engine/IoResourceManager.cc"
        "infrastructure/engine/RecipeEngine.cc"
        "infrastructure/engine/StepTypeRegistry.cc"
        
        # === Infrastructure - Serialization ===
        "infrastructure/serialization/JsonSerialization.cc"
        
        # === Infrastructure - Parsers ===
        "infrastructure/parsers/RecipeParser.cc"
        
        # === Infrastructure - Storage ===
        "infrastructure/storage/LittleFSRecipeStorage.cc"
        
        # === Application Layer ===
        "application/services/RecipeStorageManager.cc"
        "application/services/RecipeApplicationService.cc"
        
        # === Presentation Layer ===
        "presentation/controllers/RecipeController.cc"
        
    INCLUDE_DIRS 
        "."
        
    REQUIRES 
        "spiffs"
        "nvs_flash"
)

# Hinweis für Entwickler:
# Die folgenden Dateien müssen in der Haupt-main/CMakeLists.txt eingebunden werden:
# - example_implementations/recipemanagement/init/InitSteps.cc
# - example_implementations/recipemanagement/init/InitIos.cc
# Diese implementieren die init()-Methoden von StepTypeRegistry und IoResourceManager
