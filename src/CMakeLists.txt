# Find all the *.cpp files within the current source directory, and sort the list
file(GLOB SRC_SRCS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.cpp")
set(SRC_SRCS ${SRC_SRCS_C} ${SRC_SRCS_CPP})

sort_list(SRC_SRCS)

# Set all the files to use C++ as well as set their compile flags (use the module-specific compile flags, though)
set_source_files_properties(${SRC_SRCS} PROPERTIES LANGUAGE CXX COMPILE_FLAGS "${CXXFLAGS}")

# Create an empty list to store extra include directories
set(EXTRA_INCLUDES)
# Iterate through all the source files
foreach(SRC ${SRC_SRCS})
  # Temporary variable for the current source's include directories
  set(TEMP_INCLUDES)
  # Create unused skip variable
  set(SKIP)
  # Calculate the header file dependencies for the given source file
  calculate_depends(${SRC} SKIP TEMP_INCLUDES)
  # If there were some extra include directories, add them to the list
  if(TEMP_INCLUDES)
    append_to_list(EXTRA_INCLUDES ${TEMP_INCLUDES})
  endif(TEMP_INCLUDES)
endforeach(SRC)
# If there were extra include directories, remove the duplicates and add the directories to the include path
if(EXTRA_INCLUDES)
  remove_list_duplicates(EXTRA_INCLUDES)
  include_directories(${EXTRA_INCLUDES})
endif(EXTRA_INCLUDES)

# Generate the Navn executable and set it's linker flags, also set it to export it's symbols even though it's not a module
add_executable(${PROGRAM_NAME} ${SRC_SRCS})
set_target_properties(${PROGRAM_NAME} PROPERTIES LINKER_LANGUAGE CXX LINK_FLAGS "${LDFLAGS}" ENABLE_EXPORTS ON)

# Add our libs needed for linking our program
target_link_libraries(${PROGRAM_NAME} ${LINK_LIBS})

# Get the filename of the Navn executable as it is in on this system
get_target_property(BOT_BINARY ${PROGRAM_NAME} LOCATION)
get_filename_component(BOT_BINARY ${BOT_BINARY} NAME)

# Generate conf.h from the earlier configuration
configure_file(${ant_SOURCE_DIR}/include/config.h.cmake ${ant_SOURCE_DIR}/include/config.h)

# Set Navn to be installed to the bin directory
install(TARGETS ${PROGRAM_NAME}
  DESTINATION bin
)