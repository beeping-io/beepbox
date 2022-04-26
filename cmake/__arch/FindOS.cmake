# Verbose
message(CHECK_START "${BoldWhite}Detecting the Operating System${ColourReset}")

# Check operating system
if (WIN32)
    # Beepbox is not compatible with Windows    

    # Formatting
    message("\n")

    # Sending Error
    message(CHECK_PASS "${BoldGreen}BeepBox can not be installed on Windows${ColourReset}")

    # Formatting
    message("\n")

     # Exit
     message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
elseif (APPLE)
    # Set MacOs RPath
    set(CMAKE_MACOSX_RPATH 1)

    message(CHECK_PASS "${BoldWhite}MacOs Operating System Detected${ColourReset}")
    
    # Checks whether brew is installed in MacOS
    find_program(BREW brew)

    if(NOT BREW)
        # Formatting
        message("\n")

        # Sending Error
        message(CHECK_START "${BoldGreen}Please, read the message:${ColourReset}")
        message(CHECK_START "${BoldGreen}Brew is not installed and is MANDATORY${ColourReset}")
        
        # Formatting
        message("\n")

        message(CHECK_START "${BoldGreen}Please, execute the following command:${ColourReset}")    
        message(CHECK_START "${BoldWhite}/bin/bash -c \"$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)\"${ColourReset}")    

        # Formatting
        message("\n")
        
        # Exit
        message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
    else()        
        message(CHECK_START "${BoldWhite}Brew binary found at: ${BREW}${ColourReset}")
    endif()
# TODO Compile Beepbox for OS2
elseif (CMAKE_SYSTEM_NAME MATCHES "OS2")
    # Formatting
    message("\n")

    # Sending Errors
    message(CHECK_START "${BoldGreen}OS2 Operating System Detected by BeepBox${ColourReset}")    

    # Formatting
    message("\n")
 
    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
# TODO Compile Beepbox for Unix
elseif (UNIX)
    # Formatting
    message("\n")

    # Sending Errors
    message(CHECK_START "${BoldGreen}UNIX Operating System Detected by BeepBox${ColourReset}")    

    # Formatting
    message("\n")

    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")    
else()
    # Can not find the operating system
    # Formatting
    message("\n")

    # Sending Errors
    message(CHECK_START "${BoldGreen}BeepBox can not be installed on this Operating System${ColourReset}")    

    # Formatting
    message("\n")

    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")        
endif ()