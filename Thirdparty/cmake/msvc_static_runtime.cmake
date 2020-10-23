# Setup static linking with crt for microsoft compiler
# This file should be passed to cmake through command line:
#   cmake -DCMAKE_USER_MAKE_RULES_OVERRIDE=msvc_static_runtime.cmake ..
if(MSVC)
    set(CompilerFlags
            CMAKE_CXX_FLAGS_DEBUG_INIT
            CMAKE_CXX_FLAGS_MINSIZEREL_INIT
            CMAKE_CXX_FLAGS_RELEASE_INIT
            CMAKE_CXX_FLAGS_RELWITHDEBINFO_INIT

            CMAKE_C_FLAGS_DEBUG_INIT
            CMAKE_C_FLAGS_MINSIZEREL_INIT
            CMAKE_C_FLAGS_RELEASE_INIT
            CMAKE_C_FLAGS_RELWITHDEBINFO_INIT
    )
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()
