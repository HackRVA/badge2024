target_sources(${PRODUCT} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/about_badge.c
        ${CMAKE_CURRENT_LIST_DIR}/asteroids.c
        ${CMAKE_CURRENT_LIST_DIR}/battlezone.c
        ${CMAKE_CURRENT_LIST_DIR}/blinkenlights.c
        # ${CMAKE_CURRENT_LIST_DIR}/clue.c
        ${CMAKE_CURRENT_LIST_DIR}/conductor.c
        ${CMAKE_CURRENT_LIST_DIR}/cube.c
        ${CMAKE_CURRENT_LIST_DIR}/etch-a-sketch.c
        ${CMAKE_CURRENT_LIST_DIR}/game_of_life.c
        ${CMAKE_CURRENT_LIST_DIR}/ghost-detector.c
        ${CMAKE_CURRENT_LIST_DIR}/hacking_simulator.c
        ${CMAKE_CURRENT_LIST_DIR}/lunarlander.c
        ${CMAKE_CURRENT_LIST_DIR}/magic-8-ball.c
        ${CMAKE_CURRENT_LIST_DIR}/maze.c
        ${CMAKE_CURRENT_LIST_DIR}/pong.c
        ${CMAKE_CURRENT_LIST_DIR}/qc.c
        ${CMAKE_CURRENT_LIST_DIR}/smashout.c
        ${CMAKE_CURRENT_LIST_DIR}/tank-vs-tank.c
        ${CMAKE_CURRENT_LIST_DIR}/username.c
        ${CMAKE_CURRENT_LIST_DIR}/gulag.c
        ${CMAKE_CURRENT_LIST_DIR}/rvasec_splash.c
        ${CMAKE_CURRENT_LIST_DIR}/slot_machine.c
        ${CMAKE_CURRENT_LIST_DIR}/test-screensavers.c
        )
add_subdirectory(clue_assets)
add_subdirectory(new_badge_monsters_assets)
add_subdirectory(new_badge_monsters)
target_include_directories(${PRODUCT} PUBLIC .)
