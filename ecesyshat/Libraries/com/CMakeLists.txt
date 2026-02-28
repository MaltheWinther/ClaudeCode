cmake_minimum_required(VERSION 3.10)
# Just declare the install for the header
set(SRC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/com_interface.hpp")
set(DST_HEADER "${CMAKE_BINARY_DIR}/copied_headers/com_interface.hpp")

# Copy to build tree for inclusion
add_custom_command(
    OUTPUT "${DST_HEADER}"
    COMMAND ${CMAKE_COMMAND} -E copy "${SRC_HEADER}" "${DST_HEADER}"
    DEPENDS "${SRC_HEADER}"
)

add_custom_target(copy_comm_header ALL DEPENDS "${DST_HEADER}")

# Install permanently to /usr/local/include/sys_hat
install(FILES "${SRC_HEADER}" DESTINATION include)

