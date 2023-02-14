
target_sources(badge2022_c PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/achievements.c
        ${CMAKE_CURRENT_LIST_DIR}/badge.c
        ${CMAKE_CURRENT_LIST_DIR}/dynmenu.c
        ${CMAKE_CURRENT_LIST_DIR}/key_value_storage.c
        ${CMAKE_CURRENT_LIST_DIR}/menu.c
        ${CMAKE_CURRENT_LIST_DIR}/schedule.c
        ${CMAKE_CURRENT_LIST_DIR}/screensavers.c
        ${CMAKE_CURRENT_LIST_DIR}/settings.c
        ${CMAKE_CURRENT_LIST_DIR}/trig.c
        ${CMAKE_CURRENT_LIST_DIR}/xorshift.c
        )


# Define a test executable for the off-target key-value storage test.

# FIXME: this stuff breaks the pico build.
# I don't know cmake well enough to know what to do
# to make this conditionally compile only for the simulator
# build. -- steve
#

# add_executable(test_key_value_storage
#         ${CMAKE_CURRENT_LIST_DIR}/key_value_storage_test.c
#         ${CMAKE_CURRENT_LIST_DIR}/key_value_storage.c
#         ${CMAKE_CURRENT_LIST_DIR}/../hal/flash_storage_sim.c
#         )
# target_include_directories(test_key_value_storage PUBLIC
#         ${CMAKE_CURRENT_LIST_DIR}/../hal/
#         )
#
# add_test(NAME KeyValueStorageTest COMMAND test_key_value_storage)

target_include_directories(badge2022_c PUBLIC .)
