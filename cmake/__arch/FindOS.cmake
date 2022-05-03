# Verbose
message(STATUS "${BoldWhite}Detecting the Operating System${ColourReset}")

# Check operating system
if (WIN32)
    # Beepbox is not compatible with Windows    

    # Sending Error
    message(STATUS "${BoldGreen}BeepBox can not be installed on Windows${ColourReset}")

    # Formatting
    message("\n")

     # Exit
     message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
elseif (APPLE)
    # Set MacOs RPath
    set(CMAKE_MACOSX_RPATH 1)

    message(STATUS "${BoldWhite}MacOs Operating System Detected${ColourReset}")

# TODO Compile BeepBox for OS2
elseif (CMAKE_SYSTEM_NAME MATCHES "OS2")
    # Sending Error
    message(STATUS "${BoldGreen}BeepBox can not be installed on OS2${ColourReset}")

    # Formatting
    message("\n")
 
    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")
elseif (UNIX)
    # Sending Errors
    message(STATUS "${BoldGreen}UNIX Operating System Detected by BeepBox${ColourReset}")       
    
else()
    # Sending Errors
    message(STATUS "${BoldGreen}BeepBox can not be installed on this Operating System${ColourReset}")    

    # Formatting
    message("\n")

    # Exit
    message(FATAL_ERROR "${Blue}Please fix the errors explained above.{ColourReset}")        
endif ()