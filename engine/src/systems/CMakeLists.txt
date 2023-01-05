project(KohiSystems)

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE texture_system.c material_system.c)
