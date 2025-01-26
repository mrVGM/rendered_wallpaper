set(BIN_NAME wp)

add_executable(
    ${BIN_NAME}

    private/d3dx12.h
    private/dim.h
    private/renderWindow.h
    private/renderWindow.cpp
    private/render.h
    private/render.cpp
    private/wp.cpp
)

target_include_directories(${BIN_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/public)
target_include_directories(${BIN_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/private)

target_link_libraries(${BIN_NAME} PRIVATE D3D12.lib DXGI.lib D3DCompiler.lib)
target_link_libraries(${BIN_NAME} PUBLIC tasks ipc)

add_executable(
    wp_manager 

    private/wp_manager.cpp
)

target_link_libraries(wp_manager PUBLIC ipc)

