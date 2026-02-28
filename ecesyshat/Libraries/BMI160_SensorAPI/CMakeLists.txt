cmake_minimum_required(VERSION 3.10)

project(BMI160Wrapper LANGUAGES C CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# Install path prefix (default: /usr)
set(CMAKE_INSTALL_PREFIX "/usr" CACHE PATH "Install path prefix")

# Let GNUInstallDirs define LIBDIR and INCLUDEDIR (lib, include)
include(GNUInstallDirs)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Position Independent Code
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include paths
include_directories(include)

# Gather sources
file(GLOB_RECURSE BMI160_SOURCES
    "src/*.c"
    "src/*.cpp"
)

# Shared library target
add_library(BMI160Wrapper SHARED ${BMI160_SOURCES})

# Link external libraries
target_link_libraries(BMI160Wrapper PRIVATE rt)

# Versioning
set_target_properties(BMI160Wrapper PROPERTIES
    VERSION 1.0
    SOVERSION 1
)

# Install shared library into /usr/lib
install(TARGETS BMI160Wrapper
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers into /usr/include
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

