cmake_minimum_required(VERSION 3.17)
project(Kohi)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})



find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)
find_package(XCB REQUIRED)
find_package(X11_XCB REQUIRED)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib64)


add_subdirectory(engine)
add_subdirectory(testbed)

