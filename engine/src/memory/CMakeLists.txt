project(KohiMemory)
add_library(${PROJECT_NAME} SHARED)
target_sources(${PROJECT_NAME} PRIVATE linear_allocator.c kmemory.c)