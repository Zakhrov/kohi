
add_library(${PROJECT_NAME} SHARED)
add_subdirectory(core)
add_subdirectory(containers)
add_subdirectory(platform)
add_subdirectory(renderer)
add_subdirectory(math)




target_link_libraries(${PROJECT_NAME} vulkan X11 xcb X11::XCB KohiCore KohiPlatform KohiRenderer KohiMathLibrary)
