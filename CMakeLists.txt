cmake_minimum_required(VERSION 3.17)
project(Kohi)
# set(CMAKE_C_STANDARD 17)
# set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(ECM REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})



find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)
find_package(XCB REQUIRED)
find_package(X11_XCB REQUIRED)


add_subdirectory(engine)
add_subdirectory(testbed)

