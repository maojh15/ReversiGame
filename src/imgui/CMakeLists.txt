file(GLOB imgui_src_files *.cpp *.h)
add_library(lib_imgui ${imgui_src_files})
target_include_directories(lib_imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})