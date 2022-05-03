# CMake Mininum Version
cmake_minimum_required(VERSION 3.22)

# Enable clean Files
message(STATUS "Enabling ADDITIONAL_CLEAN_FILES on directory level ...")
option(ENABLE_DIRECTORY_CLEAN ON)

# Project Name
project(BeepBox VERSION "1.0.0")

# Linking Core Depedendency Library as Static
set(BUILD_SHARED_LIBS OFF)

# Adding CMake list dependencies
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Setting text colors
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/__arch/FindColors.cmake")

# Checking build directory
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/__arch/FindBuildDirectory.cmake")

# Configure Operating System
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/__arch/FindOS.cmake")

# Adding old compatibilities
set(CMAKE_PROJECT_INCLUDE_BEFORE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/__deps/EnablePolicies.cmake")

# Adding installation dependencies
set(CMAKE_INSTALL_INCLUDEDIR  "${CMAKE_BINARY_DIR}/include")
set(CMAKE_INSTALL_LIBDIR "${CMAKE_BINARY_DIR}/lib")

# Adding beepingcore dependency
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/__deps/GetDependencies.cmake")

# Setting binary instalation directory
set(BEEPBOX_INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}")

# Specifying the files to be cleaned
list(APPEND BEEPBOX_FILES_TO_CLEAN "${CMAKE_CURRENT_SOURCE_DIR}/lib")
list(APPEND BEEPBOX_FILES_TO_CLEAN "${CMAKE_CURRENT_SOURCE_DIR}/bin")
list(APPEND BEEPBOX_FILES_TO_CLEAN "${CMAKE_CURRENT_SOURCE_DIR}/build")

# Activate make clean function
set_directory_properties(
    PROPERTIES
    ADDITIONAL_CLEAN_FILES 
    "${BEEPBOX_FILES_TO_CLEAN}"
)

# Include LibSndFile
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/FindSndFile.cmake")

# Check whether LibSndFile is installed
if ( NOT LIBSNDFILE_FOUND )
  # LibSndFile is not installed
  # Formatting
  message("\n")

  # Sending Error
  message(CHECK_START "${BoldGreen}Please, read the message:${ColourReset}")
  message(CHECK_START "${BoldGreen}LibSndFile must to be installed as a dependency${ColourReset}")
   
  # Formatting
  message("\n")

  message(CHECK_START "${BoldGreen}Please, execute the following command:${ColourReset}")    
  message(CHECK_START "${BoldWhite}brew install libsndfile")    

  # Formatting
  message("\n")
   
  # Exit
  message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")

endif()

# Compiling Beepbox
add_executable(BeepBox    
    src/BeepBoxMain.cpp
    src/Mixer.cpp
    src/LoudnessStats.cpp
)
 
# Setup include directories
target_include_directories(BeepBox PUBLIC 
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
 	$<INSTALL_INTERFACE:include>
  ${LIBSNDFILE_INCLUDE_DIRS}
  ${LIBEBUR128_INCLUDE_DIRS}
)

# Setup link libraries
target_link_libraries(BeepBox 
  ${LIBSNDFILE_LIBRARIES}
  BeepingCore
  ebur128
)

# Install Targets
install(TARGETS BeepBox
        EXPORT BeepBoxTargets
        RUNTIME DESTINATION ${BEEPBOX_INSTALL_DIR}/bin 
		    ARCHIVE DESTINATION ${BEEPBOX_INSTALL_DIR}/lib
		    LIBRARY DESTINATION ${BEEPBOX_INSTALL_DIR}/lib
)

