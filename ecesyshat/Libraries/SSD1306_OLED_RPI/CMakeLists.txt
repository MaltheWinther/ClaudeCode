cmake_minimum_required(VERSION 3.10)

project(SSD1306_OLED_RPI LANGUAGES C CXX)

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
file(GLOB_RECURSE SSD1306_SOURCES
    "src/*.c"
    "src/*.cpp"
)

# Shared library target
add_library(SSD1306_OLED_RPI SHARED ${SSD1306_SOURCES})

add_dependencies(SSD1306_OLED_RPI copy_comm_header)
# Link external libraries
target_link_libraries(SSD1306_OLED_RPI PRIVATE rt)

# Versioning
set_target_properties(SSD1306_OLED_RPI PROPERTIES
    VERSION 1.0
    SOVERSION 1
)

# Install shared library into /usr/lib
install(TARGETS SSD1306_OLED_RPI
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install headers into /usr/include
install(DIRECTORY include/
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

