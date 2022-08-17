include_directories(${CMAKE_SOURCE_DIR}/engine/include)
add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE entry.c game.c)
target_link_libraries(${PROJECT_NAME} KohiEngine)