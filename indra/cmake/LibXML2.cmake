# -*- cmake -*-
include(Prebuilt)

if (STANDALONE)
  include(FindPkgConfig)
  pkg_check_modules(LIBXML2_LIBRARIES REQUIRED xml2)
else (STANDALONE)
  use_prebuilt_binary(libxml2)
  
  add_definitions(-DLIBXML_STATIC)

  set(LIBXML2_INCLUDES
      ${LIBS_PREBUILT_DIR}/include/libxml2
	  )
  if (DARWIN)
    set(LIBXML2_LIBRARIES
        xml2
        iconv
            )
  elseif(LINUX)
    set(LIBXML2_LIBRARIES
        xml2
	    )
  elseif (WINDOWS)
    set(LIBXML2_LIBRARIES
	    libxml2_a
	    )
  endif()
endif (STANDALONE)
