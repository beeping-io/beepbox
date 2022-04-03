# Adding clean files
# set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
#   "${CMAKE_CURRENT_SOURCE_DIR}/CMakeFiles"   
#   "${CMAKE_CURRENT_SOURCE_DIR}/cmake_install.cmake"
#   "${CMAKE_CURRENT_SOURCE_DIR}/CMakeCache.txt"
#   "${CMAKE_CURRENT_SOURCE_DIR}/libBeepingCore.a"
#   "${CMAKE_CURRENT_SOURCE_DIR}/Makefile"
#   "${PROJECT_BINARY_DIR}/CMakeFiles"
#   "${PROJECT_BINARY_DIR}/cmake_install.cmake"
#   "${PROJECT_BINARY_DIR}/CMakeCache.txt"
#   "${PROJECT_BINARY_DIR}/CPackConfig.cmake"
#   "${PROJECT_BINARY_DIR}/CPackSourceConfig.cmake"
#   "${PROJECT_BINARY_DIR}/install_manifest.txt"
#   "${PROJECT_BINARY_DIR}/lib"
#   "${PROJECT_BINARY_DIR}/Makefile"
# )

# Set a default C compile flag
set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w -DLINUX -DOSX -O3 -DNDEBUG -ffast-math -funroll-loops" CACHE STRING "Setting C++ Compiler Flags" FORCE)

# Configuration File
configure_file(configure.h.in configure.h)

# Main project executable
add_executable(${PROJECT_NAME} 
    BeepBoxMain.cpp
    Mixer.cpp
    LoudnessStats.cpp
)

# Subdirectory targets to be included as dependencies
include_directories(    
    ${EXTRA_INCLUDE_DIRS}
)

link_directories(
    ${EXTRA_LIB_DIRS}
)

link_libraries(
    ${EXTRA_LINKS}
)
