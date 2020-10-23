cmake_minimum_required (VERSION 3.0)

project( dep )

set (CMAKE_CXX_FLAGS "-std=c++14")
add_executable( ${PROJECT_NAME} dep.cpp )


if( DEPLOY )
    execute_process( COMMAND ${CMAKE_COMMAND} -E make_directory ${DEPLOY_DIR} )

    if( APPLE )
        if( NOT DEPLOY_DIR_EXECUTABLE )
            set( DEPLOY_DIR_EXECUTABLE ${DEPLOY_DIR} )
        endif()

        set_target_properties( ${PROJECT_NAME} PROPERTIES XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR  ${DEPLOY_DIR_EXECUTABLE} )

    elseif( WIN32 )

        set( OUTPUT_DIR "${DEPLOY_DIR}" )
        foreach( TARGET ${PROJECT_NAME} ${TARGET_MODULES_LIST} )
            foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
                string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG )
                set_target_properties ( ${TARGET} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${OUTPUT_DIR} )
            endforeach( OUTPUTCONFIG CMAKE_CONFIGURATION_TYPES )
        endforeach()

    endif()

endif()