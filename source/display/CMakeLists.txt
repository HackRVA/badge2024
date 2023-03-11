
target_sources(badge2023_c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/framebuffer.c
        ${CMAKE_CURRENT_LIST_DIR}/assets.c
        ${CMAKE_CURRENT_LIST_DIR}/assetList.c)

target_include_directories(badge2023_c PUBLIC . assets/)
