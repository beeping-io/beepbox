# CMake Mininum Version
cmake_minimum_required(VERSION 3.20.0)

# Project Name
project(BeepBox)

# Find Git
message("-- Detecting GIT ...")
find_package(Git REQUIRED)
if(Git_FOUND)
    message("-- Detected GIT ...")
endif()

# Find LibSdnFile
message("-- Detecting LibSdnFile ...")
find_package(SndFile QUIET)
if(SndFile_FOUND)

    message("-- Detected LibSdnFile ...")
    message("-- Detected LibSdnFile: ${SndFile_LIBRARIES}")
    message("-- Detected LibSdnFile: ${SndFile_INCLUDE_DIRS}")

    list(APPEND EXTRA_LINKS SndFile::sndfile)
    list(APPEND EXTRA_INCLUDE_DIRS ${SndFile_INCLUDE_DIRS})
elseif()
    
    # SEND ERROR MESSAGE
    

endif()

# Set MacOs RPath
set(CMAKE_MACOSX_RPATH 1)

# Include External dependencies
include(FetchContent)

FetchContent_Declare(libebur128
  GIT_REPOSITORY https://github.com/jiixyj/libebur128.git
  GIT_SHALLOW TRUE
  GIT_TAG tags/v1.2.6
)

# Including ebur128 Library
FetchContent_MakeAvailable(libebur128)

# Direcory to be compiled
add_subdirectory(src)

# Install
# install(FILES ${PROJECT_BINARY_DIR}/bin/BeepBox DESTINATION "${CMAKE_CURRENT_SOURCE_DIR}/bin")

# Install with CPACK
include(InstallRequiredSystemLibraries)
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGE_VERSION_MAJOR "${BEEPBOX_CORE_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${BEEPBOX_CORE_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${BEEPBOX_CORE_PATCH}")
include(CPack)
