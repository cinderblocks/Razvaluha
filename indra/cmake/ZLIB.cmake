# -*- cmake -*-

set(ZLIB_FIND_QUIETLY ON)
set(ZLIB_FIND_REQUIRED ON)

include(Prebuilt)
include(Linking)

if (STANDALONE)
  include(FindZLIB)
else (STANDALONE)
  use_prebuilt_binary(zlib-ng)
  use_prebuilt_binary(minizip-ng)
  if (WINDOWS)
    set(MINIZIP_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/libminizip.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/libminizip.lib)

    set(ZLIB_LIBRARIES 
      debug ${ARCH_PREBUILT_DIRS_DEBUG}/zlibd.lib
      optimized ${ARCH_PREBUILT_DIRS_RELEASE}/zlib.lib)
  elseif (LINUX)
    set(MINIZIP_LIBRARIES minizip)
    #
    # When we have updated static libraries in competition with older
    # shared libraries and we want the former to win, we need to do some
    # extra work.  The *_PRELOAD_ARCHIVES settings are invoked early
    # and will pull in the entire archive to the binary giving it 
    # priority in symbol resolution.  Beware of cmake moving the
    # achive load itself to another place on the link command line.  If
    # that happens, you can try something like -Wl,-lz here to hide
    # the archive.  Also be aware that the linker will not tolerate a
    # second whole-archive load of the archive.  See viewer's
    # CMakeLists.txt for more information.
    #
    set(ZLIB_PRELOAD_ARCHIVES -Wl,--whole-archive z -Wl,--no-whole-archive)
    set(ZLIB_LIBRARIES z)
  elseif (DARWIN)
    set(MINIZIP_LIBRARIES minizip)
    set(ZLIB_LIBRARIES -Wl,-force_load,${ARCH_PREBUILT_DIRS_RELEASE}/libz.a)
  endif (WINDOWS)
  set(ZLIB_INCLUDE_DIRS ${LIBS_PREBUILT_DIR}/include/zlib)
endif (STANDALONE)
