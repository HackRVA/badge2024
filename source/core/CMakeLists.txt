
target_sources(badge2022_c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/achievements.c
        ${CMAKE_CURRENT_LIST_DIR}/badge.c
        ${CMAKE_CURRENT_LIST_DIR}/dynmenu.c
        ${CMAKE_CURRENT_LIST_DIR}/menu.c
        ${CMAKE_CURRENT_LIST_DIR}/schedule.c
        ${CMAKE_CURRENT_LIST_DIR}/screensavers.c
        ${CMAKE_CURRENT_LIST_DIR}/settings.c
        ${CMAKE_CURRENT_LIST_DIR}/trig.c
        ${CMAKE_CURRENT_LIST_DIR}/xorshift.c
        )

target_include_directories(badge2022_c PUBLIC .)