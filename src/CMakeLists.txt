add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/imgui)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/game)

file(GLOB main_src_files *.cpp *.h)
add_executable(main ${main_src_files})
target_link_libraries(main PUBLIC lib_imgui)
target_link_libraries(main PUBLIC lib_reversi)

install(TARGETS main
DESTINATION ${CMAKE_INSTALL_PREFIX}
)

add_custom_command(TARGET main
    POST_BUILD COMMAND
    ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "${SDL2_LIBRARIES_DIR}/"
    $<TARGET_FILE:main>)