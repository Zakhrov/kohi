
set(CORE_SOURCES core/logger.c core/application.c core/kmemory.c core/event.c core/input.c core/clock.c)
set(PLATFORM_SOURCES platform/platform_linux.cpp)
set(CONTAINER_SOURCES containers/darray.c)
set(RENDERER_SOURCES renderer/renderer_backend.c renderer/renderer_frontend.c)
set(RENDERER_BACKEND_SOURCES renderer/vulkan_backend/vulkan_backend.cpp renderer/vulkan_backend/vulkan_device.cpp)
# set(SOURCES )



add_library(${PROJECT_NAME} SHARED ${CORE_SOURCES} ${PLATFORM_SOURCES} ${CONTAINER_SOURCES} ${RENDERER_SOURCES} ${RENDERER_BACKEND_SOURCES})
target_link_libraries(${PROJECT_NAME} vulkan X11 xcb X11::XCB)
