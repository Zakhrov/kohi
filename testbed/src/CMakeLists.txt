
set(SOURCES main.cxx)
include_directories(${CMAKE_SOURCE_DIR}/engine/src)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} KohiEngine)