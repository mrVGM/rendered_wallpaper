set(BIN_NAME ipc)

add_library(
    ${BIN_NAME}

    public/ipc.h
    private/ipc.cpp
)

target_include_directories(${BIN_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
target_include_directories(${BIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private)

