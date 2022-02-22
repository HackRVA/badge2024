
target_sources(badge2022_c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/about_badge.c
        ${CMAKE_CURRENT_LIST_DIR}/badge_monsters.c
        ${CMAKE_CURRENT_LIST_DIR}/blinkenlights.c
        ${CMAKE_CURRENT_LIST_DIR}/conductor.c
        ${CMAKE_CURRENT_LIST_DIR}/cube.c
        ${CMAKE_CURRENT_LIST_DIR}/game_of_life.c
        ${CMAKE_CURRENT_LIST_DIR}/ghost-detector.c
        ${CMAKE_CURRENT_LIST_DIR}/hacking_simulator.c
        ${CMAKE_CURRENT_LIST_DIR}/lunarlander.c
        ${CMAKE_CURRENT_LIST_DIR}/maze.c
        ${CMAKE_CURRENT_LIST_DIR}/qc.c
        ${CMAKE_CURRENT_LIST_DIR}/smashout.c
        ${CMAKE_CURRENT_LIST_DIR}/spacetripper.c
        ${CMAKE_CURRENT_LIST_DIR}/username.c
        )

target_include_directories(badge2022_c PUBLIC .)