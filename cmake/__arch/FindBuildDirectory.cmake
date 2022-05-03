# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOCAL_PATH)

# Verbose
message(STATUS "${BoldWhite}Finding directory compilation${ColourReset}")

# Checking directory
if(EXISTS "${LOCAL_PATH}")
    # Sending Error
    message(STATUS "${BoldYellow}Build directory not found${ColourReset}")

    # Formatting
    message("\n")

    # Send Error
    message(STATUS "${BoldGreen}Please, read the message:${ColourReset}")
    message(STATUS "${BoldYellow}You cannot build in a source directory with a CMakeLists.txt file.${ColourReset}")
    message(STATUS "${BoldYellow}Please make a build subdirectory.${ColourReset}")
    message(STATUS "${BoldYellow}Feel free to remove CMakeCache.txt and CMakeFiles.${ColourReset}")

    # Formatting
    message("\n")

    message(STATUS "${BoldGreen}Please, execute the following command:${ColourReset}")    
    message(STATUS "${BoldWhite}cmake -S . -B build${ColourReset}")    

    # Formatting
    message("\n")

    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
endif()

# Verbose
message(STATUS "${BoldWhite}Build directory found${ColourReset}")

