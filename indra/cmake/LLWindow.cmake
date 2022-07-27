# -*- cmake -*-

include(OpenGL)
include(GLEXT)
include(SDL2)

set(LLWINDOW_INCLUDE_DIRS
    ${GLEXT_INCLUDE_DIR}
    ${SDL_INCLUDE_DIRS}
    ${LIBS_OPEN_DIR}/llwindow
    )

set(LLWINDOW_LIBRARIES
    llwindow
    )

if (WINDOWS)
    list(APPEND LLWINDOW_LIBRARIES
        comdlg32
        )
endif (WINDOWS)
