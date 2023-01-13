project(KohiSystems)

add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE texture_system.c material_system.c geometry_system.c resource_system.c)
target_link_libraries(${PROJECT_NAME} LINK_PUBLIC KohiResourceLoaders)