# Compile Tiger2

# Add the one and only flag needed during debug compile..
set(CMAKE_C_FLAGS_DEBUG "-O3")

# Make it a static library
ADD_LIBRARY(tiger STATIC sboxes.c tiger.c)
target_link_libraries(tiger)


# Compile Skein
# TODO: do this.. lol