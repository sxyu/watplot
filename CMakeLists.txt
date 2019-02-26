cmake_minimum_required( VERSION 2.8 )

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

project( watplot )
set( INCLUDE_DIR "${PROJECT_SOURCE_DIR}/include" )

set( CMAKE_CXX_STACK_SIZE "10000000" )
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake_modules" )

if( NOT CMAKE_BUILD_TYPE )
  set( CMAKE_BUILD_TYPE Release )
endif()

set( PROJ_NAME "watplot" )
set( OUTPUT_NAME "watplot" )

include( CheckCXXCompilerFlag )
CHECK_CXX_COMPILER_FLAG( "-std=c++11" COMPILER_SUPPORTS_CXX11 )
CHECK_CXX_COMPILER_FLAG( "-std=c++0x" COMPILER_SUPPORTS_CXX0X )

if( COMPILER_SUPPORTS_CXX11 )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11" )
elseif( COMPILER_SUPPORTS_CXX0X )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x" )
else()
    message( STATUS "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler." )
endif()

if ( CMAKE_COMPILER_IS_GNUCXX )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated -Wno-deprecated-declarations -O3 -g" )
endif ( CMAKE_COMPILER_IS_GNUCXX )

FIND_PACKAGE( Eigen REQUIRED )
FIND_PACKAGE( OpenCV REQUIRED )

include_directories(${EIGEN_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})

set(
  SOURCES
  filterbank.cpp
  util.cpp
  stdafx.cpp
)

set(
  HEADERS
  ${INCLUDE_DIR}/blfile.hpp
  ${INCLUDE_DIR}/filterbank.hpp
  ${INCLUDE_DIR}/waterfall.hpp
  ${INCLUDE_DIR}/util.hpp
  ${INCLUDE_DIR}/fsutil.hpp
  ${INCLUDE_DIR}/tinydir.h
  ${INCLUDE_DIR}/consts.hpp
  ${INCLUDE_DIR}/core.hpp
  stdafx.h
)

if ( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )
  set( _DEBUG_ "//" )
endif ( NOT CMAKE_BUILD_TYPE STREQUAL "Debug" )

configure_file(config.hpp.in ${INCLUDE_DIR}/config.hpp)
list ( APPEND HEADERS ${INCLUDE_DIR}/config.hpp )

# MSVC special options
if ( MSVC )
    # generate debug symbols regardless of build type for easier debugging
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /Zi")
    
    # MSVC precompiled header
    set( PRECOMPILED_BINARY "$(IntDir)/stdafx.pch" )
    set( PRECOMPILED_HEADER "stdafx.h" )
    set( PRECOMPILED_SOURCE "stdafx.cpp" )

    set_source_files_properties( ${PRECOMPILED_SOURCE}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PRECOMPILED_HEADER}\" /Fp\"${PRECOMPILED_BINARY}\""
                                           OBJECT_OUTPUTS "${PRECOMPILED_HEADER}" )
    set_source_files_properties( ${SOURCES}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PRECOMPILED_HEADER}\" /FI\"${PRECOMPILED_HEADER}\" /Fp\"${PRECOMPILED_BINARY}\""
                                           OBJECT_DEPENDS "${PRECOMPILED_HEADER}" )
    add_definitions( -D_CRT_SECURE_NO_WARNINGS )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /EHsc /Ot" )
    set( CMAKE_EXE_LINKER_FLAGS  "${CMAKE_EXE_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT" )
    set( CMAKE_SHARED_LINKER_FLAGS  "${CMAKE_SHARED_LINKER_FLAGS} /NODEFAULTLIB:LIBCMT" )
    set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /MD" )
else ()
    add_compile_options(-g)
endif ( MSVC )

add_executable( ${PROJ_NAME} main.cpp ${SOURCES} ${HEADERS} )
target_include_directories( ${PROJ_NAME} PRIVATE ${INCLUDE_DIR} )
set_target_properties( ${PROJ_NAME} PROPERTIES OUTPUT_NAME ${OUTPUT_NAME} )
target_link_libraries( ${PROJ_NAME} ${OpenCV_LIBRARIES} )

if ( MSVC )
    set_property(TARGET ${PROJ_NAME} APPEND PROPERTY LINK_FLAGS /DEBUG)
else ()
    target_link_libraries( ${PROJ_NAME} -pthread )
endif ( MSVC )

if(WIN32)
    foreach(DLL ${CAIRO_DLLS})
        add_custom_command(TARGET ${PROJ_NAME} POST_BUILD COMMAND
            ${CMAKE_COMMAND} -E copy_if_different ${DLL} $<TARGET_FILE_DIR:${PROJ_NAME}>)
    endforeach()
endif()

# Create source group for headers
source_group( "Header Files" FILES ${HEADERS} )
