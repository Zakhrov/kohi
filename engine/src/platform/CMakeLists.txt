project(KohiPlatform)
add_library(${PROJECT_NAME} SHARED)
if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(${PROJECT_NAME} PRIVATE platform_linux.c filesystem.c)
endif ()

