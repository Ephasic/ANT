# Compile Tiger2

# Add the one and only flag needed during debug compile..
set(CMAKE_C_FLAGS_DEBUG "-O3")

# Make it a static library
ADD_LIBRARY(tiger STATIC sboxes.c tiger.c)
#set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/tiger LANGUAGE C)
target_link_libraries(tiger)


# Compile Skein
# TODO: do this.. lol