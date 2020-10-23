set( MODULE_TYPE INLINE )

set( HPP_FILES *.h   Private/*.h )
set( CPP_FILES *.cpp Private/*.cpp )

set( INCLUDES                        ${DAVA_THIRD_PARTY_ROOT_PATH}/lua/include 
                                     ${DAVA_ENGINE_DIR} 
                                     ${DAVA_THIRD_PARTY_ROOT_PATH}/include )

set( STATIC_LIBRARIES_IOS            ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/liblua_ios.a )
set( STATIC_LIBRARIES_MACOS          ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/liblua_macos.a )
set( STATIC_LIBRARIES_ANDROID        ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/liblua.a )
set( STATIC_LIBRARIES_WIN32_RELEASE  ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Release/liblua.lib )
set( STATIC_LIBRARIES_WIN32_DEBUG    ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Debug/liblua.lib )
set( STATIC_LIBRARIES_WIN64_RELEASE  ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Release/liblua.lib )
set( STATIC_LIBRARIES_WIN64_DEBUG    ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Debug/liblua.lib )
set( STATIC_LIBRARIES_WINUAP_RELEASE ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Release/lua_win.lib )
set( STATIC_LIBRARIES_WINUAP_DEBUG   ${DAVA_THIRD_PARTY_LIBRARIES_PATH}/Debug/lua_wind.lib )

setup_main_module()