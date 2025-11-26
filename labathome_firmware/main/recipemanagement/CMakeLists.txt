# Recipe Management Framework als - ESP-IDF Komponente
# Wird als eigenständige Komponente in EXTRA_COMPONENT_DIRS registriert

idf_component_register(
    SRCS 
        # === Core Domain ===
        "core/domain/entities/Recipe.cc"
        "core/domain/value-objects/StepContext.cc"
        
        # === Core Services ===
        "core/services/IoResourceManager.cc"
        
        # === Infrastructure - Engine ===
        "infrastructure/engine/RecipeEngine.cc"
        "infrastructure/engine/StepTypeRegistry.cc"
        
        # === Infrastructure - Serialization ===
        "infrastructure/serialization/JsonSerialization.cc"
        
        # === Infrastructure - Parsers ===
        "infrastructure/parsers/RecipeParser.cc"
        
        # === Application Layer ===
        "application/services/RecipeStorageManager.cc"
        "application/services/RecipeApplicationService.cc"
        
        # === Application - Use Cases (optional, momentan nicht verwendet) ===
        # "application/usecases/CreateRecipeUseCase.cc"
        # "application/usecases/DeleteRecipeUseCase.cc"
        # "application/usecases/ExecuteRecipeUseCase.cc"
        # "application/usecases/GetAvailableStepsUseCase.cc"
        # "application/usecases/GetRecipeListUseCase.cc"
        
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
# - example_implementations/recipemanagement/engine/Steps.cc (als Header-Include)
# - example_implementations/recipemanagement/engine/Io_impl.cc (als Header-Include)
# - example_implementations/recipemanagement/storage/RecipeStorage_impl.cc (optional im Protopyen zumindest) 
# Diese implementieren die init()-Methoden und konkrete Step/IO-Klassen
