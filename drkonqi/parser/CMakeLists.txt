set(BACKTRACEPARSER_SRCS
    backtraceparser.cpp
    backtraceparsergdb.cpp
    backtraceparserkdbgwin.cpp
    backtraceparsernull.cpp
)

add_library(drkonqi_backtrace_parser STATIC ${BACKTRACEPARSER_SRCS})
target_link_libraries(drkonqi_backtrace_parser
    PUBLIC
        Qt5::Core
)
