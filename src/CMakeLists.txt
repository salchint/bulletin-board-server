
# Add CPP Check
include(CppcheckTargets)
add_cppcheck_sources(test UNUSED_FUNCTIONS STYLE POSSIBLE_ERRORS FORCE)

file(
    GLOB
    headers
    *.h
    #${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.h
)

file(
    GLOB
    sources
    *.c
    *.cpp
    #${CMAKE_CURRENT_SOURCE_DIR}/../inc/*.c
)

add_executable(
    bbserv
    ${sources}
    ${headers}
)

install(
    TARGETS bbserv
    DESTINATION lib
)

install(
    FILES ${headers}
    DESTINATION include/${CMAKE_PROJECT_NAME}
)
