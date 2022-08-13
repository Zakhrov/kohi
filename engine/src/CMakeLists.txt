
set(CORE_SOURCES core/logger.cxx core/application.cxx)
set(PLATFORM_SOURCES platform/platform_linux.cxx)
set(SOURCES vulkan_test.cxx)

add_library(${PROJECT_NAME} SHARED ${CORE_SOURCES} ${SOURCES} ${PLATFORM_SOURCES})
target_link_libraries(${PROJECT_NAME} vulkan X11 xcb X11::XCB)
