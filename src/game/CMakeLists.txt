file(GLOB game_src_files *.cpp *.h)
add_library(lib_reversi ${game_src_files})
target_link_libraries(lib_reversi PUBLIC lib_imgui)
target_include_directories(lib_reversi PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
message("from game dir: ${CMAKE_CURRENT_SOURCE_DIR}")



include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0 # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
)
FetchContent_MakeAvailable(yaml-cpp)

target_link_libraries(lib_reversi PUBLIC yaml-cpp::yaml-cpp) # The library or executable that require yaml-cpp library