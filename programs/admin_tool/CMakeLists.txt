add_executable( admin_tool main.cpp )
if( UNIX AND NOT APPLE )
    set(rt_library rt )
endif()

find_package( Gperftools QUIET )
if( GPERFTOOLS_FOUND )
    message( STATUS "Found gperftools; compiling cli_wallet with TCMalloc")
    list( APPEND PLATFORM_SPECIFIC_LIBS tcmalloc )
endif()

if( STEEM_STATIC_BUILD )
    # I don't know why steemit_app is required twice in the following line, I just know the linker breaks if it isn't.
    target_link_libraries( admin_tool PRIVATE
            "-static-libstdc++ -static-libgcc -lreadline"
            steemit_app graphene_net steemit_chain steemit_protocol graphene_utilities steemit_private_message steemit_app steemit_follow fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
else( STEEM_STATIC_BUILD )
    target_link_libraries( admin_tool PRIVATE
            steemit_app graphene_net steemit_chain steemit_protocol graphene_utilities steemit_private_message steemit_app steemit_follow fc ${CMAKE_DL_LIBS} ${PLATFORM_SPECIFIC_LIBS} )
endif( STEEM_STATIC_BUILD )

if(MSVC)
    set_source_files_properties( main.cpp PROPERTIES COMPILE_FLAGS "/bigobj" )
endif(MSVC)

install( TARGETS
        admin_tool

        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        )
