
target_sources(${PRODUCT} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/framebuffer.c
        ${CMAKE_CURRENT_LIST_DIR}/assetList.c)

target_include_directories(${PRODUCT} PUBLIC . assets/)
