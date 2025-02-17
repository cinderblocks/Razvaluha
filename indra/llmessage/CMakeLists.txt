# -*- cmake -*-

project(llmessage)

include(00-Common)
include(CURL)
include(OpenSSL)
include(NGHTTP2)
include(LLCommon)
include(AIStateMachine)
include(LLMath)
include(LLMessage)
include(LLVFS)
include(LLXML)

include_directories (${CMAKE_CURRENT_SOURCE_DIR})

include_directories(
    ${LLCOMMON_INCLUDE_DIRS}
    ${AISTATEMACHINE_INCLUDE_DIRS}
    ${LLMATH_INCLUDE_DIRS}
    ${LLMESSAGE_INCLUDE_DIRS}
    ${LLVFS_INCLUDE_DIRS}
    ${LLXML_INCLUDE_DIRS}
    )

set(llmessage_SOURCE_FILES
    aiaverage.cpp
    aicurl.cpp
    aicurleasyrequeststatemachine.cpp
    aicurlperservice.cpp
    aicurlthread.cpp
    aicurltimer.cpp
    aihttpheaders.cpp
    aihttptimeout.cpp
    aihttptimeoutpolicy.cpp
    debug_libcurl.cpp
    llares.cpp
    llareslistener.cpp
    llassetstorage.cpp
    llavatarname.cpp
    llavatarnamecache.cpp
    llbuffer.cpp
    llbufferstream.cpp
    llcachename.cpp
    llchainio.cpp
    llcircuit.cpp
    llclassifiedflags.cpp
    lldatapacker.cpp
    lldispatcher.cpp
    llexperiencecache.cpp
    llfiltersd2xmlrpc.cpp
    llhost.cpp
    llhttpclient.cpp
    llhttpnode.cpp
    llhttpsender.cpp
    llinstantmessage.cpp
    lliobuffer.cpp
    lliohttpserver.cpp
    lliopipe.cpp
    lliosocket.cpp
    llioutil.cpp
    llmessagebuilder.cpp
    llmessageconfig.cpp
    llmessagelog.cpp
    llmessagereader.cpp
    llmessagetemplate.cpp
    llmessagetemplateparser.cpp
    llmessagethrottle.cpp
    llnamevalue.cpp
    llnullcipher.cpp
    llpacketack.cpp
    llpacketbuffer.cpp
    llpacketring.cpp
    llpartdata.cpp
    llproxy.cpp
    llpumpio.cpp
    llsdappservices.cpp
    llsdhttpserver.cpp
    llsdmessage.cpp
    llsdmessagebuilder.cpp
    llsdmessagereader.cpp
    llstoredmessage.cpp
    lltemplatemessagebuilder.cpp
    lltemplatemessagedispatcher.cpp
    lltemplatemessagereader.cpp
    llthrottle.cpp
    lltransfermanager.cpp
    lltransfersourceasset.cpp
    lltransfersourcefile.cpp
    lltransfertargetfile.cpp
    lltransfertargetvfile.cpp
    lltrustedmessageservice.cpp
    llurlrequest.cpp
    lluseroperation.cpp
    llxfer.cpp
    llxfer_file.cpp
    llxfer_mem.cpp
    llxfer_vfile.cpp
    llxfermanager.cpp
    llxorcipher.cpp
    machine.cpp
    message.cpp
    message_prehash.cpp
    message_string_table.cpp
    net.cpp
    patch_code.cpp
    patch_dct.cpp
    patch_idct.cpp
    sound_ids.cpp
    )

set(llmessage_HEADER_FILES
    CMakeLists.txt

    aiaverage.h
    aicurl.h
    aicurleasyrequeststatemachine.h
    aicurlperservice.h
    aicurlprivate.h
    aicurlthread.h
    aicurltimer.h
    aihttpheaders.h
    aihttptimeout.h
    aihttptimeoutpolicy.h
    debug_libcurl.h
    llares.h
    llareslistener.h
    llassetstorage.h
    llavatarname.h
    llavatarnamecache.h
    llbuffer.h
    llbufferstream.h
    llcachename.h
    llchainio.h
    llcipher.h
    llcircuit.h
    llclassifiedflags.h
    llcororesponder.h
    llcurl.h
    lldatapacker.h
    lldbstrings.h
    lldispatcher.h
    lleventflags.h
    llexperiencecache.h
    llfiltersd2xmlrpc.h
    llfollowcamparams.h
    llhost.h
    llhttpclient.h
    llhttpnode.h
    llhttpnodeadapter.h
    llhttpsender.h
    llinstantmessage.h
    llinvite.h
    lliobuffer.h
    lliohttpserver.h
    lliopipe.h
    lliosocket.h
    llioutil.h
    llloginflags.h
    llmessagebuilder.h
    llmessageconfig.h
    llmessagelog.h
    llmessagereader.h
    llmessagetemplate.h
    llmessagetemplateparser.h
    llmessagethrottle.h
    llmsgvariabletype.h
    llnamevalue.h
    llnullcipher.h
    llpacketack.h
    llpacketbuffer.h
    llpacketring.h
    llpartdata.h
    llproxy.h
    llpumpio.h
    llqueryflags.h
    llregionflags.h
    llregionhandle.h
    llsdappservices.h
    llsdhttpserver.h
    llsdmessage.h
    llsdmessagebuilder.h
    llsdmessagereader.h
    llstoredmessage.h
    lltaskname.h
    llteleportflags.h
    lltemplatemessagebuilder.h
    lltemplatemessagedispatcher.h
    lltemplatemessagereader.h
    llthrottle.h
    lltransfermanager.h
    lltransfersourceasset.h
    lltransfersourcefile.h
    lltransfertargetfile.h
    lltransfertargetvfile.h
    lltrustedmessageservice.h
    llurlrequest.h
    lluseroperation.h
    llvehicleparams.h
    llxfer.h
    llxfer_file.h
    llxfer_mem.h
    llxfer_vfile.h
    llxfermanager.h
    llxorcipher.h
    machine.h
    mean_collision_data.h
    message.h
    message_prehash.h
    net.h
    patch_code.h
    patch_dct.h
    sound_ids.h
    )

set_source_files_properties(${llmessage_HEADER_FILES}
                            PROPERTIES HEADER_FILE_ONLY TRUE)

list(APPEND llmessage_SOURCE_FILES ${llmessage_HEADER_FILES})

add_library (llmessage ${llmessage_SOURCE_FILES})

target_link_libraries(
  llmessage
  PUBLIC
  llcommon
  ${CURL_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${CRYPTO_LIBRARIES}
  ${NGHTTP2_LIBRARIES}
  ${XMLRPCEPI_LIBRARIES}
  ${PTHREAD_LIBRARY}
)

# tests
if (LL_TESTS)
  include(GoogleMock)
  include(LLAddBuildTest)
  include(Python)
  include(Tut)

  SET(llmessage_TEST_SOURCE_FILES
    llnamevalue.cpp
    lltrustedmessageservice.cpp
    lltemplatemessagedispatcher.cpp
    )
  LL_ADD_PROJECT_UNIT_TESTS(llmessage "${llmessage_TEST_SOURCE_FILES}")

  #    set(TEST_DEBUG on)
  set(test_libs
    ${LLMESSAGE_LIBRARIES}
    ${WINDOWS_LIBRARIES}
    ${LLVFS_LIBRARIES}
    ${LLMATH_LIBRARIES}
    ${LLCOMMON_LIBRARIES}
    ${GOOGLEMOCK_LIBRARIES}
    ${LLXML_LIBRARIES}
    )

  LL_ADD_INTEGRATION_TEST(
    llsdmessage
    "llsdmessage.cpp"
    "${test_libs}"
    ${PYTHON_EXECUTABLE}
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_llsdmessage_peer.py"
    )

  LL_ADD_INTEGRATION_TEST(llavatarnamecache "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llhost "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llpartdata "" "${test_libs}")
  LL_ADD_INTEGRATION_TEST(llxfer_file "" "${test_libs}")
endif (LL_TESTS)

