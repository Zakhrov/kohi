project(KohiResourceLoaders)
add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE image_loader.c material_loader.c binary_loader.c)
