function(find_sdl2_libraries_dir)
    # locate a libSDL2*.dylib and set SDL2_LIBRARIES_DIR to its containing directory
    if(NOT DEFINED SDL2_ROOT_DIR)
        set(SDL2_ROOT_DIR "")
    endif()

    find_library(SDL2_LIBRARY NAMES SDL2 libSDL2
        PATHS ${SDL2_ROOT_DIR}
                    /opt/homebrew/lib
                    /usr/local/lib
                    /usr/lib
                    /Library/Frameworks
                    ~/anaconda3/lib
        PATH_SUFFIXES lib
    )

    message("LINE25: SDL2_LIBRARY ${SDL2_LIBRARY}, ${SDL2_LIBRARIES_DIR}")
    if(SDL2_LIBRARY)
        get_filename_component(SDL2_LIBRARIES_DIR ${SDL2_LIBRARY} DIRECTORY)
    else()
        file(GLOB _SDL2_CANDIDATES
            "${SDL2_ROOT_DIR}/lib/libSDL2*.dylib"
            "/opt/homebrew/lib/libSDL2*.dylib"
            "/usr/local/lib/libSDL2*.dylib"
            "/usr/lib/libSDL2*.dylib"
        )
        list(GET _SDL2_CANDIDATES 0 _first_candidate)
        if(_first_candidate)
            get_filename_component(SDL2_LIBRARIES_DIR ${_first_candidate} DIRECTORY)
        endif()
    endif()

    if(SDL2_LIBRARIES_DIR)
        message(STATUS "Found SDL2 library in: ${SDL2_LIBRARIES_DIR}")
        set(SDL2_LIBRARIES_DIR ${SDL2_LIBRARIES_DIR} CACHE PATH "Directory containing libSDL2*.dylib")
    else()
        message(WARNING "Could not locate libSDL2*.dylib. Set SDL2_ROOT_DIR or SDL2_LIBRARIES_DIR to the correct location.")
    endif()
endfunction()