
target_sources(${PRODUCT} PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/achievements.c
        ${CMAKE_CURRENT_LIST_DIR}/a_star.c
        ${CMAKE_CURRENT_LIST_DIR}/badge.c
        ${CMAKE_CURRENT_LIST_DIR}/bline.c
        ${CMAKE_CURRENT_LIST_DIR}/dynmenu.c
	${CMAKE_CURRENT_LIST_DIR}/fxp_sqrt.c
        ${CMAKE_CURRENT_LIST_DIR}/key_value_storage.c
        ${CMAKE_CURRENT_LIST_DIR}/menu.c
        ${CMAKE_CURRENT_LIST_DIR}/schedule.c
        ${CMAKE_CURRENT_LIST_DIR}/screensavers.c
        ${CMAKE_CURRENT_LIST_DIR}/settings.c
        ${CMAKE_CURRENT_LIST_DIR}/trig.c
        ${CMAKE_CURRENT_LIST_DIR}/xorshift.c
        )


# Define a test executable for the off-target key-value storage test
# and a_star.

if (${TARGET} STREQUAL "SIMULATOR" OR ${TARGET} STREQUAL "SDL_SIMULATOR")
	add_executable(test_key_value_storage
		${CMAKE_CURRENT_LIST_DIR}/key_value_storage_test.c
		${CMAKE_CURRENT_LIST_DIR}/key_value_storage.c
		${CMAKE_CURRENT_LIST_DIR}/../hal/flash_storage_sim.c
		)
	target_include_directories(test_key_value_storage PUBLIC
		${CMAKE_CURRENT_LIST_DIR}/../hal/
		)

	add_test(NAME KeyValueStorageTest COMMAND test_key_value_storage)

	add_executable(test_a_star
		${CMAKE_CURRENT_LIST_DIR}/a_star.c
		${CMAKE_CURRENT_LIST_DIR}/test_a_star.c
		)
endif()

if (${TARGET} STREQUAL "SIMULATOR")
	add_custom_target(deprecation ALL
		echo "The GTK based simulator is deprecated.  Use the SDL based simulator."
	)
endif()

target_include_directories(${PRODUCT} PUBLIC .)
