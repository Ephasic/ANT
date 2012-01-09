file(GLOB SRC_SRCS_CPP RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.tmpl")
set(SRC_SRCS ${SRC_SRCS_C} ${SRC_SRCS_CPP})

sort_list(SRC_SRCS)

# TODO: complete this!
foreach(TMPL ${SRC_SRCS})
  compile_tmpl(${TMPL} TMPL_CPP)

endforeach(TMPL ${SRC_SRCS})