# -*- cmake -*-

include(FreeType)

if(LINUX OR STANDALONE)
set(OPENGL_FIND_QUIETLY OFF)
set(OPENGL_FIND_REQUIRED ON)
set(OpenGL_GL_PREFERENCE LEGACY)
include(FindOpenGL)
endif(LINUX OR STANDALONE)

set(LLRENDER_INCLUDE_DIRS
    ${LIBS_OPEN_DIR}/llrender
    ${OPENGL_INCLUDE_DIR}
    )

set(LLRENDER_LIBRARIES
    llrender
    ${OPENGL_LIBRARIES}
    )

