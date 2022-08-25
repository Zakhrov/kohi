project(KohiEngine)

include_directories(include)
add_library(${PROJECT_NAME} SHARED)
add_subdirectory(src)
target_link_libraries(${PROJECT_NAME} vulkan X11 xcb X11::XCB KohiCore KohiPlatform KohiRenderer KohiMemory KohiMathLibrary)
