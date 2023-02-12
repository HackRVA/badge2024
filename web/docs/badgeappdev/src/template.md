# App Template
1. copy `./source/apps/badge-app-template.c` and `./source/apps/badge-app-template.h`
2. read through both files and follow instructions in the comments
3. add your file to `./source/apps/CMakeLists.txt`
```diff
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
        ${CMAKE_CURRENT_LIST_DIR}/username.c
        ${CMAKE_CURRENT_LIST_DIR}/slot_machine.c
+       ${CMAKE_CURRENT_LIST_DIR}/<app file name>.c
        )
```
4. add an import to your `*.h` file in `./source/core/menu.c`
5. create a menu entry in `./source/core/menu.c`

```diff
const struct menu_t games_m[] = {
   {"Blinkenlights", VERT_ITEM|DEFAULT_ITEM, FUNCTION, {(struct menu_t *)blinkenlights_cb}}, // Set other badges LED
   {"Conductor",     VERT_ITEM, FUNCTION, {(struct menu_t *)conductor_cb}}, // Tell other badges to play notes
   {"Lunar Rescue",  VERT_ITEM, FUNCTION, {(struct menu_t *)lunarlander_cb} },
   {"Badge Monsters",VERT_ITEM, FUNCTION, {(struct menu_t *)badge_monsters_cb} },
   {"Smashout",      VERT_ITEM, FUNCTION, {(struct menu_t *)smashout_cb} },
   {"Hacking Sim",   VERT_ITEM, FUNCTION, {(struct menu_t *)hacking_simulator_cb} },
   {"Spinning Cube", VERT_ITEM, FUNCTION, {(struct menu_t *)cube_cb} },
   {"Game of Life", VERT_ITEM, FUNCTION, {(struct menu_t *)game_of_life_cb} },
   {"Slot Machine", VERT_ITEM, FUNCTION, {(struct menu_t *)slot_machine_cb}},
+  {"<AppName>",     VERT_ITEM, FUNCTION, {(struct menu_t *)<App Callback>}},
   {"Back",         VERT_ITEM|LAST_ITEM, BACK, {NULL}},
};
```
