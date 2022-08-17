
set(SOURCES entry.c game.c)


include_directories(${CMAKE_SOURCE_DIR}/engine/include)
add_executable(${PROJECT_NAME} ${SOURCES})
target_link_libraries(${PROJECT_NAME} KohiEngine)