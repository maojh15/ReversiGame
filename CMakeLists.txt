cmake_minimum_required(VERSION 3.20)
project(ReversiChess)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_INSTALL_PREFIX ${CMAKE_SOURCE_DIR}/bin)

find_package(SDL2 REQUIRED)
# 计算 SDL2 动态库所在目录并设置 rpath，这样运行 ./main 时 dyld 能找到 libSDL2-2.0.0.dylib
include(findsdl2dir.cmake)

# 在构建和安装时使用这个 rpath
set(CMAKE_INSTALL_RPATH "${SDL2_LIB_DIR}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# add targets
add_subdirectory(${CMAKE_SOURCE_DIR}/src)
target_include_directories(lib_imgui PUBLIC ${SDL2_INCLUDE_DIR})
message("12: ${SDL2_LIBRARIES}")
target_link_libraries(lib_imgui PUBLIC ${SDL2_LIBRARIES} "-framework OpenGL")


