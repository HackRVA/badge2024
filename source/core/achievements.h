#ifndef ACHIEVEMENTS_H__
#define ACHIEVEMENTS_H__

/* This is meant to be used by badge apps to record achievements. */

enum achievement {
	ACHIEVEMENT_MAZE_DRAGONS_SLAIN = 0,
	ACHIEVEMENT_MAZE_CHALICE_FOUND = 1,
	ACHIEVEMENT_MAZE_CHALICE_RECOVERED = 2,
	ACHIEVEMENT_COUNT = 3 /* <-- must be last, & the enum must sequentially increase from 0 */
};

/* maybe_load_achievements_from_flash() will load achievements from flash the first time
 * it is called. Subsequent calls won't do anything until the badge is rebooted.
 */
extern void maybe_load_achievements_from_flash(void);

/* Increment the count for the specified achievement by the specified amount.
 * Achievements will be saved to flash (once that is implemented).
 */
extern void add_achievement(enum achievement achievement, unsigned short achievement_increment);

/* Returns the achievement count for the specified acheivement
 * (or -1 if you pass it an achievement that is out of range.)
 * This also calls maybe_load_achievements_from_flash().
 */
extern int get_achievement_count(enum achievement achievement);

#endif
