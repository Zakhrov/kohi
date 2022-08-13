
set(CORE_SOURCES core/logger.c core/application.c core/kmemory.c)
set(PLATFORM_SOURCES platform/platform_linux.c)
set(SOURCES vulkan_test.cpp)

add_library(${PROJECT_NAME} SHARED ${CORE_SOURCES} ${SOURCES} ${PLATFORM_SOURCES})
target_link_libraries(${PROJECT_NAME} vulkan X11 xcb X11::XCB)
