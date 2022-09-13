# -*- cmake -*-

# The copy_win_libs folder contains file lists and a script used to
# copy dlls, exes and such needed to run the SecondLife from within
# VisualStudio.

include(CMakeCopyIfDifferent)
include(Linking)

# When we copy our dependent libraries, we almost always want to copy them to
# both the Release and the RelWithDebInfo staging directories. This has
# resulted in duplicate (or worse, erroneous attempted duplicate)
# copy_if_different commands. Encapsulate that usage.
# Pass FROM_DIR, TARGETS and the files to copy. TO_DIR is implicit.
# to_staging_dirs diverges from copy_if_different in that it appends to TARGETS.
MACRO(to_debug_staging_dirs from_dir targets)
  foreach(staging_dir
          "${SHARED_LIB_STAGING_DIR_DEBUG}")
    copy_if_different("${from_dir}" "${staging_dir}" out_targets ${ARGN})
    list(APPEND "${targets}" "${out_targets}")
  endforeach()
ENDMACRO(to_debug_staging_dirs from_dir to_dir targets)

MACRO(to_relwithdeb_staging_dirs from_dir targets)
  foreach(staging_dir
          "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}")
    copy_if_different("${from_dir}" "${staging_dir}" out_targets ${ARGN})
    list(APPEND "${targets}" "${out_targets}")
  endforeach()
ENDMACRO(to_relwithdeb_staging_dirs from_dir to_dir targets)

MACRO(to_release_staging_dirs from_dir targets)
  foreach(staging_dir
          "${SHARED_LIB_STAGING_DIR_RELEASE}")
    copy_if_different("${from_dir}" "${staging_dir}" out_targets ${ARGN})
    list(APPEND "${targets}" "${out_targets}")
  endforeach()
ENDMACRO(to_release_staging_dirs from_dir to_dir targets)

###################################################################
# set up platform specific lists of files that need to be copied
###################################################################
if(WINDOWS)
    if(GEN_IS_MULTI_CONFIG)
        set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug")
        set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo")
        set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release")
    elseif (UPPERCASE_CMAKE_BUILD_TYPE MATCHES DEBUG)
        set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    elseif (UPPERCASE_CMAKE_BUILD_TYPE MATCHES RELWITHDEBINFO)
        set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    elseif (UPPERCASE_CMAKE_BUILD_TYPE MATCHES RELEASE)
        set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")
    endif()

    #*******************************
    # VIVOX - *NOTE: no debug version
    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        SLVoice.exe
        vivoxplatform.dll
        )
    if (ADDRESS_SIZE EQUAL 64)
        list(APPEND vivox_files
            vivoxsdk_x64.dll
            ortp_x64.dll
            )
    else (ADDRESS_SIZE EQUAL 64)
        list(APPEND vivox_files
            vivoxsdk.dll
            ortp.dll
            )
    endif (ADDRESS_SIZE EQUAL 64)

    #*******************************
    # Misc shared libs 

    set(addrsfx "-x${ADDRESS_SIZE}")

    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        glod.dll
        libexpatd.dll
        libhunspell.dll
        libogg.dll
        libvorbis.dll
        libvorbisfile.dll
        xmlrpc-epid.dll
        )

    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        libapr-1.dll
        libaprutil-1.dll
        libapriconv-1.dll
        glod.dll
        libexpat.dll
        libhunspell.dll
        libogg.dll
        libvorbis.dll
        libvorbisfile.dll
        xmlrpc-epi.dll
        )

    if(NOT DISABLE_TCMALLOC)
      list(APPEND debug_files libtcmalloc_minimal-debug.dll)
      list(APPEND release_files libtcmalloc_minimal.dll)
    endif(NOT DISABLE_TCMALLOC)

    if(OPENAL)
      list(APPEND debug_files alut.dll OpenAL32.dll)
      list(APPEND release_files alut.dll OpenAL32.dll)
    endif(OPENAL)

    if (USE_FMODSTUDIO)
      list(APPEND debug_files fmodL.dll)
      list(APPEND release_files fmod.dll)
    endif (USE_FMODSTUDIO)

elseif(DARWIN)
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}/Debug/Resources")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}/RelWithDebInfo/Resources")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}/Release/Resources")

    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        SLVoice
        ca-bundle.crt
        libsndfile.dylib
        libvivoxoal.dylib
        libortp.dylib
        libvivoxplatform.dylib
        libvivoxsdk.dylib
       )
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(release_files
        libapr-1.0.dylib
        libapr-1.dylib
        libaprutil-1.0.dylib
        libaprutil-1.dylib
        libexception_handler.dylib
        libexpat.1.5.2.dylib
        libexpat.dylib
        libGLOD.dylib
        libhunspell-1.7.0.dylib
        libndofdev.dylib
       )

    if (USE_FMODSTUDIO)
      list(APPEND debug_files libfmodL.dylib)
      list(APPEND release_files libfmod.dylib)
    endif (USE_FMODSTUDIO)

elseif(LINUX)
    # linux is weird, multiple side by side configurations aren't supported
    # and we don't seem to have any debug shared libs built yet anyways...
    set(SHARED_LIB_STAGING_DIR_DEBUG            "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELWITHDEBINFO   "${SHARED_LIB_STAGING_DIR}")
    set(SHARED_LIB_STAGING_DIR_RELEASE          "${SHARED_LIB_STAGING_DIR}")

    set(vivox_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    set(vivox_files
        libsndfile.so.1
        libortp.so
        libvivoxoal.so.1
        libvivoxplatform.so
        libvivoxsdk.so
        SLVoice
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${ARCH_PREBUILT_DIRS_DEBUG}")
    set(debug_files
       )
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${ARCH_PREBUILT_DIRS_RELEASE}")
    # *FIX - figure out what to do with duplicate libalut.so here -brad
    set(release_files
        libapr-1.so.0
        libaprutil-1.so.0
        libexpat.so
        libexpat.so.1
        libGLOD.so
       )

    if (USE_TCMALLOC)
      list(APPEND release_files "libtcmalloc_minimal.so")
    endif (USE_TCMALLOC)

    if (USE_FMODSTUDIO)
      list(APPEND debug_files "libfmodL.so")
      list(APPEND release_files "libfmod.so")
    endif (USE_FMODSTUDIO)

    if(OPENAL)
      list(APPEND release_files libopenal.so)
    endif(OPENAL)

else(WINDOWS)
    message(STATUS "WARNING: unrecognized platform for staging 3rd party libs, skipping...")
    set(vivox_src_dir "${CMAKE_SOURCE_DIR}/newview/vivox-runtime/i686-linux")
    set(vivox_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(debug_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib/debug")
    set(debug_files "")
    # *TODO - update this to use LIBS_PREBUILT_DIR and LL_ARCH_DIR variables
    # or ARCH_PREBUILT_DIRS
    set(release_src_dir "${CMAKE_SOURCE_DIR}/../libraries/i686-linux/lib/release")
    set(release_files "")

    set(debug_llkdu_src "")
    set(debug_llkdu_dst "")
    set(release_llkdu_src "")
    set(release_llkdu_dst "")
    set(relwithdebinfo_llkdu_dst "")
endif(WINDOWS)


################################################################
# Done building the file lists, now set up the copy commands.
################################################################

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_DEBUG}"
    out_targets 
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${vivox_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${vivox_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})



#copy_if_different(
#    ${debug_src_dir}
#    "${SHARED_LIB_STAGING_DIR_DEBUG}"
#    out_targets
#    ${debug_files}
#    )
#set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELEASE}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

copy_if_different(
    ${release_src_dir}
    "${SHARED_LIB_STAGING_DIR_RELWITHDEBINFO}"
    out_targets
    ${release_files}
    )
set(third_party_targets ${third_party_targets} ${out_targets})

if(NOT STANDALONE)
  add_custom_target(
      stage_third_party_libs ALL
      DEPENDS ${third_party_targets}
      )
endif(NOT STANDALONE)
