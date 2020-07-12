cmake_minimum_required(VERSION 3.8 FATAL_ERROR)

find_program(CLANG_FORMAT_EXE NAMES "clang-format" DOC "Path to clang-format executable")

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

set(FFT_OCEAN_WAVES_SOURCES ${PROJECT_SOURCE_DIR}/src/main.cpp)
file(GLOB_RECURSE SHADER_SOURCES ${PROJECT_SOURCE_DIR}/src/*.glsl)

if (APPLE)
    add_executable(FFTOceanWaves MACOSX_BUNDLE ${FFT_OCEAN_WAVES_SOURCES} ${SHADER_SOURCES} ${ASSET_SOURCES})
    set(MACOSX_BUNDLE_BUNDLE_NAME "FFTOceanWaves") 
    set_source_files_properties(${SHADER_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/shader)
    set_source_files_properties(${ASSET_SOURCES} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
else()
    add_executable(FFTOceanWaves ${FFT_OCEAN_WAVES_SOURCES}) 
endif()

target_link_libraries(FFTOceanWaves dwSampleFramework)

if (NOT APPLE)
    add_custom_command(TARGET FFTOceanWaves POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/src/shader $<TARGET_FILE_DIR:FFTOceanWaves>/shader)
endif()

if(CLANG_FORMAT_EXE)
    add_custom_target(FFTOceanWaves-clang-format COMMAND ${CLANG_FORMAT_EXE} -i -style=file ${FFT_OCEAN_WAVES_SOURCES} ${SHADER_SOURCES})
endif()

set_property(TARGET FFTOceanWaves PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/$(Configuration)")