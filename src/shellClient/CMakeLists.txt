aux_source_directory(. SRC_LIST)

# find *.h
include_directories(${PROJECT_SOURCE_DIR}/include)

# find *.lib
link_directories(${PROJECT_SOURCE_DIR}/lib)

# generate exe
add_executable(shellTools ${SRC_LIST})

# link *.lib
# target_link_libraries(shellTools pthread)

# set exe directory
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/build)