set( MODULE_NAME_STUB  Sound )

set( CPP_FILES         Private/SoundSystem.cpp ) 
set( CPP_FILES_STUB    Private/*Stub.cpp )#only stub mode
set( CPP_FILES_IMPL    Private/*Impl.cpp )#only impl mode

set( IMPL_MODULE       ${DAVA_ROOT_DIR}/Modules/Sound )

setup_main_module()