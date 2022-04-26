# Include External dependencies
include(FetchContent)

# Getting libebur128
FetchContent_Declare(ebur128
  GIT_REPOSITORY https://github.com/jiixyj/libebur128.git
  GIT_TAG tags/v1.2.6
)

# Getting BeepingCore
FetchContent_Declare(BeepingCore
  GIT_REPOSITORY https://github.com/alfredrc/beeping-core.git
  GIT_SHALLOW TRUE
)

# Making available
FetchContent_MakeAvailable(ebur128 BeepingCore)

# Setting include directories
set(LIBEBUR128_INCLUDE_DIRS ${ebur128_SOURCE_DIR}/ebur128)
