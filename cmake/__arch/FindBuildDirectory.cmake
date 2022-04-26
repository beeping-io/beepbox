# Require out-of-source builds
file(TO_CMAKE_PATH "${PROJECT_BINARY_DIR}/CMakeLists.txt" LOCAL_PATH)

# Verbose
message(CHECK_START "${BoldWhite}Finding directory compilation${ColourReset}")

# Checking directory
if(EXISTS "${LOCAL_PATH}")
    # Sending Error
    message(CHECK_PASS "${BoldYellow}Build directory not found${ColourReset}")

    # Formatting
    message("\n")

    # Send Error
    message(CHECK_START "${BoldGreen}Please, read the message:${ColourReset}")
    message(CHECK_START "${BoldYellow}You cannot build in a source directory with a CMakeLists.txt file.${ColourReset}")
    message(CHECK_START "${BoldYellow}Please make a build subdirectory.${ColourReset}")
    message(CHECK_START "${BoldYellow}Feel free to remove CMakeCache.txt and CMakeFiles.${ColourReset}")

    # Formatting
    message("\n")

    message(CHECK_START "${BoldGreen}Please, execute the following command:${ColourReset}")    
    message(CHECK_START "${BoldWhite}cmake -S . -B build${ColourReset}")    

    # Formatting
    message("\n")

    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
endif()

# Verbose
message(CHECK_PASS "${BoldWhite}Build directory found${ColourReset}")

