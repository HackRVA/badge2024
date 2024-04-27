#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __linux__
#include <signal.h> /* so we can raise(SIGTRAP) if we detect a bug. */
#endif

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "bline.h"
#include "dynmenu.h"
#include "utils.h"
#include "assetList.h"
#include "xorshift.h"
#include "rtc.h"
#include "music.h"

struct dynmenu planet_menu;
struct dynmenu_item planet_menu_item[10];
struct dynmenu cave_menu;
struct dynmenu_item cave_menu_item[10];
struct dynmenu town_menu;
struct dynmenu_item town_menu_item[10];

/* x and y offsets indexed by direction, 4 and 8 direction variants */
static const int xo4[] = { 0, 1, 0, -1 };
static const int yo4[] = { -1, 0, 1, 0 };
static const int xo8[] = { 0, 1, 1, 1, 0, -1, -1, -1 };
static const int yo8[] = { -1, -1, 0, 1, 1, 1, 0, -1 };

/* Return world index (0 - 4095) from (x, y) coords */
static inline int windex(int x, int y)
{
	return (y * 64) + x;
}

enum badgey_world_type { WORLD_TYPE_SPACE, WORLD_TYPE_PLANET, WORLD_TYPE_TOWN, WORLD_TYPE_CAVE };

struct badgey_world {
	char *name;
	enum badgey_world_type type;
	char const *wm; /* world map */
	struct badgey_world const *subworld[10];
	int landingx, landingy;
	unsigned int initial_seed;
};

static const char space_map[4096] = {
	"                                                                " /* 0 */
	"                                                      *         "
	"      *        1                                                "
	"                                                                "
	"                             *                        0         "
	"                                                                "
	"                 *                           *                * "
	"                                                                "
	"  *                                                             "
	"                                             *                  "
	"                                                                " /* 10 */
	"              *                         *                       "
	"                                                    2           "
	"                                                                "
	"                                                                "
	"                                                                "
	"        *           *    *                                      "
	"                                                                "
	"                                         *               *      "
	"                                                                "
	"                                                                " /* 20 */
	"                                                                "
	"                                                                "
	"                  *                                3            "
	"                                                                "
	"                                                                "
	"                                                                "
	"                                *                               "
	"                                                                "
	"                                                                "
	"                                                                " /* 30 */
	"        *                                     *                 "
	"                                     *                          "
	"                                                                "
	"                                                          *    *"
	"                  *                                             "
	"                                *                               "
	"*                                                               "
	"                                                                "
	"                                                                "
	"         *                                         *            " /* 40 */
	"                                                                "
	"                                                                "
	"                                                                "
	"                                   *                            "
	"                *                                               "
	"                                                                "
	"                                                                "
	"                                                                "
	"         *                    *                                 "
	"                                                                " /* 50 */
	"                                                                "
	"                                             *                  "
	"   *                                                            "
	"                                                                "
	"                           *                                    "
	"                                                                "
	"                                 4                              "
	"*               *                                  *            "
	"                                                                "
	"                                                   *            " /* 60 */
	"                        *                 *                     "
	"       *                                                        "
	"                                                                ", /* 63 */
};

static const char ossaria_map[4096] = {
	"....wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.."
	".........wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"ww.........wwwwwwwwwwwwwwwwwwwwwwww..wwww..wwwwwwwwwww.wwwwwwwww"
	"ww..........ffmmmmmf.....wwwwwwwwwwwwww....wwwwwwwwwww....wwwwww"
	"wwwww.......ffm..fmff.......wwwwwwwwwwwwwwwwwwwwwwww..........ww"
	"wwwwww....ffffmmffmmf.......wwwwwwwwwwwwwwwwwww..........wwwwwww"
	"wwwwww.....ff.5mmff.ff.....wwww...................wwwwwwwwwwwwww"
	"wwwwww......f..fmm..ff.....w0..................wwwwwwwwwwwwwwwww"
	"wwwww..........ff.................................wwwwwwwwwwwwww"
	"wwwww...........f.............................wwwwwwwwwwwwwwwwww"
	"wwwwww..w................................wwwwwwwwwwwwwwwwwwwwwww"
	"wwwww...ww...................wwwwwwm..................wwwwwwwwww"
	"wwwwwwwww.................f.wwwwwmmm................wwwwww.wwwww"
	"wwwww..ww................fffwwwmmm................wwwwwww..wwwww"
	"wwwwww..www.............ffffffm6.....................w.....wwwww"
	"wwwww.....................ff1ffmmfff....................wwwwwwww"
	"wwwww......................fffffmffffff................wwwwwwwww"
	"www....fmf...................ffffffff..................wwwwwwwww"
	"www.....mmf.............fff....fffff......................wwwwww"
	"wwwww...fmmf....................fffff.................www.wwwwww"
	"wwwww...ffmfff....................fffff...............wwwwwwwwww"
	"wwwww...f...f7ff........................................wwwwwwww"
	"wwwww.w......f.ff.......................................wwwwwwww"
	"wwwwwww........fff....................................w.wwww..ww"
	"wwwwwwww.......ffff...................................wwwww...ww"
	"wwwwww...........f..f................................2wwww....ww"
	"wwwww..............ff......................f...........ww.....ww"
	"wwwwww...........................mm..m....ff................wwww"
	"wwwww...............f........m..mm...mm...ff................wwww"
	"wwwww........................mmmm.....mm8ff.............wwwwwwww"
	"wwwwww......................mmm........mmfffff........wwwwwwwwww"
	"wwwwww......................m..........3mf..ff........wwwwwwwwww"
	"wwwwwww.....................m..........mmf...f..........wwwwwwww"
	"wwwwwwwwww..................m................ff.......wwwwwwwwww"
	"wwwwwwwww...........................mmmmmm....f.......wwwwwwwwww"
	"wwww..w.......................mmmmmmm...m...f.f............wwwww"
	"wwww.........................mmm.......ff..ff..............wwwww"
	"wwwwww......................ffffff...fff...ff...........w.wwwwww"
	"wwwwww....................fffff.....fff....f....ww....wwwwwwwwww"
	"wwwww....................fffffff....fff..........www.....wwwwwww"
	"wwwww..................fffffff.....................wwwwwwwwwwwww"
	"wwwww...................fffffff....................ww..wwwwwwwww"
	"wwwww...............fffffffffffffffffffff........www.....wwwwwww"
	"wwwwww.................fffffffffffffffffffff...........wwwwwwwww"
	"wwwwww........................fffffffffffffffffffff...wwwwwwwwww"
	"wwwwww.......................ffffffffffff..............wwwwwwwww"
	"wwwwwww.................fffffff.....fffffff................wwwww"
	"wwwwwwww...........fffffff................fffffff........wwwwwww"
	"wwwwwwwww..www..........................................wwwwwwww"
	"wwwwwwwwww.w..........................................wwwwwwwwww"
	"wwwwww..wwwwwww.............................................wwww"
	"wwwww......w................mmm..........f.............w.....www"
	"wwwwwww....w.............wwwwwmm.........f.f......w....www...www"
	"wwwwwwww...ww.......ffff....wwwwww......f.f.......w.w..wwwwwwwww"
	"wwwwwwww..........ffff........wwwww.....m9f.....w.www...wwwwwwww"
	"wwwww...........ffffffff........www.....mmf.....www......wwwwwww"
	"wwwww.........fffffffffffffff..wwww......mmm......w......wwwwwww"
	"wwwwww....w..fffff..........wwwwwwwwww.....m.....wwwwwwwwwwwwwww"
	"wwwwwww..ww..........wwww...wwwwwwwwwwwwwwwww............wwwwwww"
	"wwwwwww.wwww..4..wwwwwwwwww.wwwwwwwwwwwwwwwwwwwww.........wwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.........www"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.....wwwwwwwwwwwwww......www"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww.....w"
	"..wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww......"
};

static const char NW42_map[4096] = {
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwww.wwwww..wwwwwwwwwwwwwwwwwwwwwwwwwwwwww..wwwwwwwwwwwwwww"
	"wwwwwww...........wwwwwwwwwwwwwwwwwwwwwwwwwww....wwwwwwwwwwwwwww"
	"wwwwww..mmm.......wwwwwwwwwwww..w.wwwwwwwwwww..wwwwwwwwwwwwwwwww"
	"wwwwww..m.........wwwwwww..w.......wwwwwwwwwwwwwwwwwwwwwwww..www"
	"wwwwww............wwwww...............wwwwwwwwwwwwwwwww......www"
	"wwwwwww.0........wwwwww.................wwwwwwwwwwwwwww......www"
	"wwwwwww........ffwwwwwww................wwww...wwwwwww......wwww"
	"wwwwww........ff.wwwwwww...............www.....wwwwww.....w.wwww"
	"wwwww..mmm....ff....wwww..............www......wwwww..ww..wwwwww"
	"wwwwww....m...ff.....w................ww.......wwwwwwww...wwwwww"
	"wwwwwwwww.....fff....www.......f.........ff...wwwwwwwwwwwwwwwwww"
	"wwwwwwwww.....f..f...w.w......fff.......fff...wwwwww.wwwwwwwwwww"
	"wwwwwwww........f....w.........ff......ffff..wwwww....wwwwwwwwww"
	"wwwwwww........................fff.....fff.....1......wwwwwwwwww"
	"wwwww............f..............ffff....f.............wwwwwwwwww"
	"www..............................f5ff...f............wwwwwwwwwww"
	"www..............................ffff.............wwwwwwwwwwwwww"
	"www....ww.........................f.ff............wwwwwwwwwwwwww"
	"www..wwwww..........................ffff...........wwwwwww...www"
	"wwwwwwwwww...........................fff...........wwwwww.....ww"
	"wwwwwwww.............................................ww.......ww"
	"wwwwww.....................................................ww.ww"
	"wwwww...................................fffff.............wwwwww"
	"wwwwww............mm...................fm7ff............wwwwwwww"
	"wwwwww.f...fffff...mm................fffmf.........ww..wwwwwwwww"
	"wwwwww.ff....fffff..mm.................fmfff......wwwwwwwwwwwwww"
	"wwwww..fff......fffffm................fmmff.......wwwwwwwwwwwwww"
	"wwwww..fffff.....2ff6m..............fffmf..........wwwwwwwwwwwww"
	"wwwww..ff.fff....ffmm................ffmff.........wwwwwwwwwwwww"
	"wwwww...f..ffffffffmm...................ff.........w.wwwwww..www"
	"wwwww........ff.fmmm....................ff.............wwww..www"
	"wwwwwwww..m..............................f......w........www..ww"
	"wwwwwwwww.mmm.................................w.w.......3.w...ww"
	"wwwwwwwwwww....w............ffffff............wwwww..........www"
	"wwwwww.wwwwwwwww..........fffffffmfff.............w..ww.....wwww"
	"wwwwww.......w.........ffffffffffmmmfff...........wwwwwwwwwwwwww"
	"wwwwww.......w.............fffffffm8mmfffff.......wwwwwwwwwwwwww"
	"wwwwww.......................fffffffmmmffffff.....wwwww.wwwwwwww"
	"wwwwww.............................ffffffffffffffffwww.....wwwww"
	"wwwwww....fffff......................fffff...w.............wwwww"
	"wwwwwww.....fffff............................w..............wwww"
	"wwwwwww........fffff.......................wwwwww...........wwww"
	"wwwwwwwww.......fff..f.................wwwww....wwwwww.....wwwww"
	"wwwwww...........fff..............................wwwwwww...wwww"
	"wwwwwww........f..ff..............................wwwwwwww...www"
	"wwww.........w....f...............ffff..............wwwwww...www"
	"wwwww........ww..................ff..ff..............wwww...wwww"
	"wwwwww......ww............mmmmmmmmm...f...............www..wwwww"
	"wwwwww......w...........mmm.......mmm.f...............wwwwwwwwww"
	"wwwwww.....www........mmmfffff.....9mmmf................wwwwwwww"
	"wwwwww..wwwwww......mmm..............mmmff..............wwwwwwww"
	"wwwwwww.wwwwwwf......ffff..............fff.................wwwww"
	"wwwwwwwwwwwwwwff......fffff......4.wwfff..w................wwwww"
	"wwwwwwwwwwwwwwff.........mmm....mmmmw..wwww..............wwwwwww"
	"wwww..wwwwwwwfff...........mmmmmm...w.ww................wwwwwwww"
	"wwww.ffwwwfffff.....................www................wwwwwwwww"
	"wwww.ffffffff........................ww................wwwwwwwww"
	"wwww.wwwww................ww..........w...............wwwwwwwwww"
	"wwwwwwwwww..w.www..........w..........ww...........wwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwww.........ww....ww....ww.........wwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwww....w....wwww..www..wwwww.....wwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
};

static const char borton_map[4096] = {
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwww..wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww......wwww"
	"wwwwwwwwwwwwwwwwww...www.wwwwwwwwwwwwwwwwwwwwwwww......wwwwwwwww"
	"wwwwwwwwwwwwwwwww.......wwwwwwwwww...wwwwwwwwww......wwwwwwwwwww"
	"wwwww..wwwwwwww........wwwwwwwwwww.....www.............wwwwwwwww"
	"wwwwww..www................wwww..........w...........wwwwwwwwwww"
	"wwwwwww.......ffffffffff....ww.......................wwwwwwwwwww"
	"wwwwwww...w......ffffffffff.ww.......................wwww...wwww"
	"wwwwwwwwwww.....ffffffffff...w..........mmmffff......www...wwwww"
	"wwwwwwwww.........ffffffffff.ww............mffff...........wwwww"
	"wwwwwwwww.............f.ff....w.............mmffff.........wwwww"
	"www.....................ff....w..............m5mff........wwwwww"
	"ww...................f...f....www............wwmfff...wwwwwwwwww"
	"www....w......................w............mmffmf.........wwwwww"
	"wwwwwwwww....................0w..........mffff...........wwwwwww"
	"wwwwwwwwww..............................ffff1.f............wwwww"
	"wwwwwwwwww.......ff........................ff.f............wwwww"
	"wwwwwwwwwww.....fff........................ff............wwwwwww"
	"wwwwwwwwwww......f..........................f..........wwwwwwwww"
	"wwwwwww..............................................wwwwwwwwwww"
	"www.................................................wwwwwwwwwwww"
	"wwwwwwwww...........................................wwwww...wwww"
	"wwwwwwwwwww............fff....................mmm....wwwww....ww"
	"wwww...wwww.............fff....................6m....ww.....wwww"
	"wwwww..www......fff.........................m...m..........wwwww"
	"wwww..............fff.......fff............mmm...........wwwwwww"
	"wwww...............fffff..fff...................w....wwwwwwwwwww"
	"wwww................f.f7f..fffff................w...wwwwwwwwwwww"
	"wwwww...............f..fffff....................wwwwwwwwwwwwwwww"
	"wwwwwwwww...........ff....fff..................ww...wwwwwwwwwwww"
	"wwwwwwwwwww..........f......fff......................wwwwwwwwwww"
	"wwwwwwwwwww...................fff....................wwwwwwwwwww"
	"wwwwwwwwwww.....ww......................................wwwwwwww"
	"wwwwwwwwwwww....w.....................f..................wwwwwww"
	"wwwwwwwwwwwwwwwwww.................fff.............w.wwwwwwwwwww"
	"wwwwwwwwwwww.....w...................fff...........wwwwwwwwwwwww"
	"wwwwwwwwwww......w...................f.fff.fff......wwwwwwwwwwww"
	"wwww...www...............................fff.fff.....wwwwwwwwwww"
	"wwwww...................mmmmmmmmmm........f....fff.......wwwwwww"
	"wwwww...www......mmmmmmmmmmmww...mmmm............fff.....wwwwwww"
	"wwww......ww.......ffffm8mm.2.....m............r........wwwwwwww"
	"www.....wwww.........fffffmm......m....................wwwwwwwww"
	"wwww...wwwww...............mm.fff....................wwwwwwwwwww"
	"wwwwwwwwwww.................fff......................wwwwwwwwwww"
	"wwwwwwwww..........f.................................wwwwwwwwwww"
	"wwwwww......fff....................................wwwwwwwwwwwww"
	"wwwwwwwww..fff..fff.................www.............wwwww...wwww"
	"wwwwwwww..........fff.fff.f.........www......mm.....3ww.....wwww"
	"wwwwwwwwww......w...fff..............wwww.....mm..........wwwwww"
	"wwwwwwwwwwww...www....fffffffff.........w.....9mm.......wwwwwwww"
	"wwwwwwwwwwww.4.ww.......fff.............wwww.........wwwwwwwwwww"
	"wwwwwwwwwww...ff..........fff....fff......ww...ff....wwwwwwwwwww"
	"wwwwww.........ffffff......fff.fff.........w....ff...wwwwwwwwwww"
	"www.............ffffff.......fff...........w....ff...wwwwwwwwwww"
	"wwwwwwww.............ff....................w..........wwwwwwwwww"
	"wwwwwwwwwww......w....ffff.............wwwww............wwwwwwww"
	"wwwwwwwwwwww....ww.................wwwwwww....wwww......wwwwwwww"
	"wwwwwwwwwwwwwwwwwww.....wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwww.....wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
};

static const char skang_map[4096] = {
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwww..wwwwwwwww.wwwwwwwwwwwwwww...wwwwwwwwwwwww"
	"wwwwwwwwww....wwww........wwww.........wwww.......wwwwwwwwwwwwww"
	"wwwwww..........w.........ww.............ww..............wwwwwww"
	"wwwwwwwww...............................www...............wwwwww"
	"wwwwwwwwww..............................w............wwwwwwwwwww"
	"wwwwwwwwwww.............................w0...........wwwwwwwwwww"
	"wwwwwwwwwwwww..........................................wwwwwwwww"
	"wwwwwwwwww.......fffff.........mmmff.....................wwwwwww"
	"wwwww....ww..m......fffff..fffm5mwfffff..............wwwwwwwwwww"
	"wwwwwwww..w.mm..........fffffm........fffff......wwwwwwwwwwwwwww"
	"wwwwwww......m..f........ffffffffff...............w..wwwwww.wwww"
	"wwwwwwww........ff..ffff....fffffffff............ww..wwww...wwww"
	"wwwwwwwwwww......fffff.........fffffffffffffffff.....ww.w...wwww"
	"wwwwwwwwwww...fffff..f..............fffff1fff..............wwwww"
	"wwwwwwwww..fffff....f..................fffff..............wwwwww"
	"www.......................................................wwwwww"
	"wwwww................................f...................wwwwwww"
	"wwwwwwww...ww................fffm.fffff..................wwwwwww"
	"wwwwwwwwwwww................mmmfffff.................w..wwwwwwww"
	"wwwwwwwww...............fffm6ffffwfffff..............w.wwwwwwwww"
	"wwwwww.................fffffff.....fffff......www....wwwwwwwwwww"
	"wwwwww...mm.....fff..fffffww2.........f.........wwwwwwwwwwwwwwww"
	"wwwwwww..m.......ffffff.fffff..fffff...........ww...wwwwwwwwwwww"
	"wwwwwww........fffff.......fffff...fffff......ww.....wwwwwwwwwww"
	"wwwwwwwww...fffff................f.ff................ww.wwwwwwww"
	"wwwwwwwwww................................................wwwwww"
	"wwwwwwwwww............................................w...wwwwww"
	"wwwwwwww.............................................wwwwwwwwwww"
	"wwwwwwww...................mmmm......................wwwwwwwwwww"
	"wwwwwww.................mmmm7f.......................wwwwwwwwwww"
	"wwwwww...............mmmmm.fff.........................wwwwwwwww"
	"wwww...............................fffff...................wwwww"
	"wwwwwww.....ww..................fffff..........m.............www"
	"wwwwwwwwwwwww................fffff............mm........ww...www"
	"wwwwwwwww...wwww.............................mm.........www.wwww"
	"wwwwwwwwww...................................m.........wwwwwwwww"
	"wwwwwwwwww...................................m.........wwwwwwwww"
	"wwwwwwwwww..............mfffff...........................ww.wwww"
	"wwwwww................mmm.fffff.............................wwww"
	"wwww..................m8ffff................................wwww"
	"wwwwww...............mmfffff.f.......................w......wwww"
	"wwww...............fffff...............m.............wwww.wwwwww"
	"wwwwwwww............fffff..f.........m..........fff..wwwwwwwwwww"
	"wwwwww...........fffff..............mm...........fff.wwwwwwwwwww"
	"wwwwww..................f..........mm..........fff...wwwwwwwwwww"
	"wwwwwww.............f..............mm...........fff...wwwwwwwwww"
	"wwwwwww...........................mm3........fff......wwwwwwwwww"
	"wwwwwwwwww........................m...........fff......wwwwwwwww"
	"wwwwwwwww........ffffff..........mm........ffff.........wwwwwwww"
	"wwwwwwwwww...........ffffff......m.....fff.............wwwwwwwww"
	"wwwwwwwww........ffffff..................fff.......m.......wwwww"
	"wwwwwwww......ffffff....................ffff.....mm.........wwww"
	"wwwwwwww..............................fff........m9m.....wwwwwww"
	"wwwwww..............................fff........mm.......wwwwwwww"
	"wwwwwww..........w............ww.....................wwwwwwwwwww"
	"wwwww....m......ww4....mm......w.............w.......wwwwwwwwwww"
	"wwwww.....mm....w.....mm.......w.............ww.........wwwwwwww"
	"wwwwwww....mm..ww.....m.......ww...........wwww........wwwwwwwww"
	"wwwwwwwwww....wwwww...........wwwww...wwwwwwwww.......wwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
};

static const char gnarg_map[4096] = {
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww...wwwwwwwwww"
	"wwwwwwwwww.......wwwwww.......wwwwwwwwwww..mwww......wwwwwwwwwww"
	"wwwwww...........www......ff......wwww.....m.........ww..wwwwwww"
	"wwww...w..........w...mm..fff.......ww.....mm............wwwwwww"
	"wwwwwwww..........w....mm0ff.........w.......mm..........wwwwwww"
	"wwwwwwwwwww.............mmf...................m.........wwwwwwww"
	"wwww.....www........ff...mm........................wwwwwwwwwwwww"
	"wwwww......w.........f................ww........wwwwwwwwwwww.www"
	"wwwwww...............ff...............w.......wwwww.....www..www"
	"wwwwwwww..............f......ffff....ww......wwww..f.....ww..www"
	"wwwwwwwww......................ff.....www...www....fff...1f..www"
	"wwwwwwwww..........ff..................wwwwwwww.....5f...fffwwww"
	"wwwwwwww............fffff...............wwwwww......ff...f...www"
	"wwwwwwww.............fffff.......www..wwwwww........f....f....ww"
	"www....w................fffff......wwwww.............f..ff.w..ww"
	"wwww.....................fffff........www...............fffw..ww"
	"wwwwwww...ff................fffff.......www................ww.ww"
	"wwwwwwwww..ffmm...............fffff......wwwf.............wwwwww"
	"wwwwwwwww..fffmm...............fffff......wfff...........wwwwwww"
	"wwwwwwww....fff6mmmm..........fffff.......w.ff...........wwwwwww"
	"wwwwwwww....fffmm...m..........fffff.....ww.ffm...........wwwwww"
	"wwwwwww......fffm..............fffff........ff7mm.........wwwwww"
	"wwwww........fffm.............fffff..........ff.mmww......wwwwww"
	"ww....ww2.....ffm...............fffff.........f...www.....wwwwww"
	"www....w......f................fffff...............www..wwwwwwww"
	"wwwwwwww.........................fffff..............wwwwwwwwwwww"
	"wwwwwwwww..........................ff.................w..wwwwwww"
	"wwwwwwwwww...www....................ff....................wwwwww"
	"wwwwwwwwwwwwww..............................ffff...........wwwww"
	"wwwwwwww.....................................ffff..........wwwww"
	"wwwwwwww...................................f..ffff..........wwww"
	"wwwwww.........................................ffff...........ww"
	"wwwwww........................................f.f.........w...ww"
	"wwwww...............fffff........................ffff....www..ww"
	"wwww.............ffff...ffff......................ffff..wwww..ww"
	"wwww............ff.........fff....................f.....wwwwwwww"
	"wwww.........ffff............ffff.............f.ffff.....wwwwwww"
	"wwwww.......ff..................ff...............ffff....wwwwwww"
	"wwwwww...........................ff...............ffff...wwwwwww"
	"wwwwwww..........................m8f......................wwwwww"
	"wwwwwwww.........................mmff........fff..........wwwwww"
	"wwwwwwww.........................3m...........fff..........wwwww"
	"wwwwwwww..........................mff.............m........wwwww"
	"wwwwww............................mff..........fffmm.......wwwww"
	"www................................mff..........ff9mm......wwwww"
	"ww.......ff.ff....................................fffm.....wwwww"
	"wwww......ff.......................m.............fffm....wwwwwww"
	"www........ffff...................mm...............fff...wwwwwww"
	"wwww..........ffff..............mm.ff....................www..ww"
	"wwwww...........fff............mmfff...............m.m....w...ww"
	"wwwwww............ff..........mmff...............mmm.m........ww"
	"wwwwwwww.....ff..........ffff..ffff..............m....m.....wwww"
	"wwwwwww.......fff.....ffff..ffff......................m.....wwww"
	"wwwwwww.........ffff....................w..................wwwww"
	"wwwww..............ffff.................ww........w......wwwwwww"
	"wwwww......mfff..........................w........ww.....wwwwwww"
	"ww....ww....mmfff.......ww...............w..fff....ww....wwwwwww"
	"www....w......mmfff.....4w..............ww...fff....ww...wwwwwww"
	"wwwwwwww........mm.......w............www......fff..www.wwwwwwww"
	"wwwwwwwww...............ww...........www...........wwww.wwwwwwww"
	"wwwwwwwwwwwwwwwww......wwwww.....w.wwwwwwwwww...wwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
	"wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww"
};

static const struct badgey_world ossaria = {
	.name = "OSSARIA",
	.type = WORLD_TYPE_PLANET,
	.wm = ossaria_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 37,
	.landingy = 32,
	.initial_seed = 0x5a5aa5a5,
};

static const struct badgey_world NW42 = {
	.name = "NW42",
	.type = WORLD_TYPE_PLANET,
	.wm = NW42_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
	.initial_seed = 0xa5a55a5a,
};

static const struct badgey_world borton = {
	.name = "BORTON",
	.type = WORLD_TYPE_PLANET,
	.wm = borton_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
	.initial_seed = 0xc3c3c3c3,
};


static const struct badgey_world skang = {
	.name = "SKANG",
	.type = WORLD_TYPE_PLANET,
	.wm = skang_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
	.initial_seed = 0x3c3c3c3c,
};


static const struct badgey_world gnarg = {
	.name = "GNARG",
	.type = WORLD_TYPE_PLANET,
	.wm = gnarg_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
	.initial_seed = 0x55555555,
};

static const struct badgey_world space = {
	.name = "SPACE",
	.type = WORLD_TYPE_SPACE,
	.wm = space_map,
	.subworld = { &ossaria, &NW42, &borton, &skang, &gnarg, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
	.initial_seed = 0xaaaaaaaa,
};

static char dynmap[4096]; /* for dynamically generated locations like towns and caves */
static char dynworld_name[255];

struct badgey_world dynworld = {
	.name = dynworld_name,
	.type = WORLD_TYPE_TOWN, /* for now */
	.wm = dynmap,
	.subworld = { NULL, NULL, NULL,  NULL, NULL, NULL, NULL, NULL, NULL, NULL, },
	.landingx = 0,
	.landingy = 0,
	.initial_seed = 0, /* not used */
};

/* Begin code generated by png-to-badge-asset from badgey_assets/planet.png Fri Apr 12 15:33:50 2024. */
static const uint16_t planet_colormap[89] = {
	0x0882, 0x0000, 0x0001, 0x39cb, 0x210a, 0x2111, 0x08d2, 0x298f, 0x2106, 0x18c3, 0x1082, 0x0841,
	0x18c4, 0x294d, 0x190c, 0x08d1, 0x1912, 0x10d2, 0x214e, 0x10c4, 0x0041, 0x1909, 0x08cf, 0x2112,
	0x6b55, 0x6b95, 0x4a53, 0x2152, 0x4a93, 0x1083, 0x1084, 0x088d, 0x08d0, 0x3a12, 0x6354, 0x39d2,
	0x2992, 0x008e, 0x18c5, 0x214d, 0x420e, 0x39d0, 0x6312, 0x10cc, 0x52d0, 0x088a, 0x4a4e, 0x088c,
	0x110f, 0x0049, 0x088b, 0x210f, 0x4211, 0x4a92, 0x31d2, 0x1112, 0x0889, 0x4212, 0x2950, 0x6314,
	0x6b54, 0x190f, 0x190a, 0x3a11, 0x52d3, 0x10c9, 0x0007, 0x084a, 0x4a50, 0x5b12, 0x31d1, 0x1910,
	0x0843, 0x0886, 0x4a51, 0x210b, 0x1088, 0x1089, 0x420f, 0x6b53, 0x08ce, 0x214f, 0x08cd, 0x10cd,
	0x0842, 0x2108, 0x10c7, 0x10ca, 0x08cb,
}; /* 89 values */

static const uint8_t planet_data[256] = {
	0, 1, 1, 2, 3, 4, 5, 6, 6, 5, 7, 8, 9, 10, 11, 1, 1, 1, 12, 13,
	6, 6, 6, 6, 6, 6, 6, 6, 7, 8, 1, 11, 11, 12, 14, 6, 15, 6, 6, 16,
	17, 6, 6, 6, 6, 18, 19, 11, 20, 21, 22, 15, 6, 6, 23, 24, 25, 26, 6, 27,
	28, 27, 21, 29, 30, 22, 31, 32, 6, 6, 33, 26, 6, 34, 26, 6, 35, 36, 37, 38,
	39, 40, 41, 42, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 43, 44, 31, 33,
	32, 6, 6, 6, 6, 6, 6, 6, 6, 6, 35, 5, 45, 46, 47, 31, 48, 23, 36, 6,
	6, 6, 6, 6, 6, 36, 28, 6, 49, 45, 50, 47, 51, 52, 53, 16, 6, 35, 17, 6,
	33, 17, 54, 55, 56, 45, 45, 47, 31, 53, 57, 58, 59, 60, 26, 6, 24, 6, 6, 61,
	62, 45, 45, 50, 31, 31, 51, 63, 36, 60, 64, 24, 24, 54, 6, 65, 38, 66, 67, 45,
	50, 47, 31, 68, 69, 70, 71, 15, 54, 6, 6, 72, 10, 73, 45, 67, 45, 50, 50, 7,
	74, 74, 22, 32, 22, 6, 75, 10, 10, 29, 76, 77, 45, 50, 50, 47, 78, 79, 80, 22,
	22, 81, 12, 10, 20, 11, 29, 75, 56, 56, 50, 47, 31, 82, 83, 22, 39, 29, 10, 10,
	20, 20, 11, 84, 85, 86, 87, 88, 50, 83, 18, 85, 10, 10, 11, 1,
}; /* 256 values */

static const struct asset2 planet = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) planet_colormap,
	.pixel = (const unsigned char *) planet_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/planet.png Fri Apr 12 15:33:50 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/star.png Fri Apr 12 15:37:08 2024. */
static const uint16_t star_colormap[51] = {
	0x1081, 0x0000, 0xa4cd, 0x5286, 0x3143, 0x6b08, 0x2902, 0x0800, 0x39c4, 0x3184, 0x3984, 0x18c1,
	0x4204, 0x39c5, 0x5246, 0x62c7, 0x5ac7, 0x5a86, 0x7348, 0xa4cc, 0xa48c, 0x83c9, 0x2943, 0xc5ce,
	0xe690, 0xeed0, 0x83ca, 0x9c4b, 0xc58e, 0xde90, 0xe6d0, 0xf757, 0x944b, 0xeed1, 0xf711, 0xff57,
	0xad0e, 0x944c, 0x2102, 0x4a46, 0xde4f, 0x1881, 0x940a, 0x7b89, 0x5a87, 0x41c4, 0x6b07, 0x2903,
	0x4a05, 0x2942, 0x0840,
}; /* 51 values */

static const uint8_t star_data[256] = {
	0, 0, 1, 1, 1, 1, 1, 2, 3, 1, 1, 1, 1, 1, 1, 1, 1, 4, 1, 1,
	1, 1, 1, 2, 5, 1, 1, 1, 1, 1, 6, 7, 1, 0, 8, 1, 1, 1, 1, 9,
	2, 1, 1, 1, 1, 10, 11, 1, 1, 1, 12, 13, 1, 1, 1, 3, 2, 1, 1, 1,
	14, 8, 1, 1, 1, 1, 1, 15, 16, 1, 1, 2, 2, 1, 2, 2, 17, 1, 1, 1,
	1, 1, 1, 2, 2, 18, 2, 2, 19, 2, 20, 21, 1, 1, 1, 1, 1, 1, 1, 1,
	22, 23, 2, 24, 25, 23, 23, 1, 1, 1, 1, 1, 26, 2, 2, 27, 28, 29, 30, 31,
	31, 23, 23, 2, 2, 32, 2, 2, 2, 23, 2, 23, 23, 33, 34, 35, 31, 24, 2, 36,
	37, 2, 2, 5, 1, 38, 39, 2, 2, 2, 29, 25, 25, 40, 2, 17, 41, 10, 11, 1,
	1, 1, 1, 3, 3, 23, 2, 23, 40, 23, 23, 2, 1, 1, 1, 1, 1, 1, 1, 9,
	2, 16, 2, 2, 23, 42, 19, 5, 1, 1, 1, 1, 1, 1, 8, 43, 2, 44, 45, 2,
	2, 10, 8, 46, 1, 1, 1, 1, 1, 47, 48, 1, 1, 1, 1, 2, 39, 1, 1, 45,
	8, 1, 1, 1, 49, 4, 1, 1, 1, 1, 1, 2, 16, 1, 1, 1, 1, 1, 1, 1,
	50, 1, 1, 1, 1, 1, 1, 15, 2, 1, 1, 1, 1, 1, 1, 1,
}; /* 256 values */

static const struct asset2 star = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) star_colormap,
	.pixel = (const unsigned char *) star_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/star.png Fri Apr 12 15:37:08 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/badgey.png Fri Apr 12 15:39:57 2024. */
static const uint16_t badgey_colormap[101] = {
	0x634d, 0xa556, 0xbe19, 0xdf1c, 0xc658, 0x8c51, 0x52cb, 0x6b4d, 0xbe59, 0xa596, 0x8c93, 0x8cd3,
	0xbe5a, 0xc65a, 0xbe18, 0x9d14, 0xad55, 0xc618, 0xb554, 0xac8f, 0xb511, 0xacd1, 0xbd95, 0xdf1b,
	0x9cd3, 0xb596, 0x7c10, 0x6b0c, 0x6ac9, 0x6aca, 0x6b0b, 0x7b4b, 0x5acc, 0x9492, 0x7c11, 0x3a09,
	0x4249, 0x6b4e, 0x8c92, 0x4a4a, 0x6b8f, 0x39c7, 0xcedc, 0xc69b, 0xb619, 0xbdd7, 0x39c8, 0x4209,
	0x630d, 0x94d4, 0xce9b, 0xbdd6, 0xa597, 0x8cd4, 0xce59, 0x3187, 0x3a08, 0xad52, 0xb5d4, 0xad97,
	0x9cd4, 0x2986, 0x31c7, 0x5b0c, 0xc659, 0xb5d8, 0xd6dc, 0xd6dd, 0xb5d7, 0x2946, 0x3186, 0x9493,
	0xdf5e, 0xad96, 0x2145, 0x2945, 0x52cc, 0x8451, 0x738f, 0xdf1d, 0x73cf, 0x8452, 0x8410, 0x7bcf,
	0x7bd0, 0xa555, 0xb597, 0xa514, 0x94d3, 0x9d15, 0x4208, 0x8c52, 0x8411, 0x5b0d, 0x738e, 0x5a89,
	0x73d0, 0x6b8e, 0x4a49, 0x4a8a, 0xa515,
}; /* 101 values */

static const uint8_t badgey_data[256] = {
	255, 255, 255, 255, 255, 255, 0, 1, 1, 2, 3, 4, 5, 255, 255, 255, 255, 255, 255, 6,
	7, 8, 9, 10, 11, 12, 13, 14, 15, 255, 255, 255, 255, 255, 255, 16, 17, 18, 19, 20,
	21, 22, 16, 23, 24, 255, 255, 255, 255, 255, 255, 25, 26, 27, 28, 29, 30, 31, 32, 15,
	33, 255, 255, 255, 0, 26, 34, 25, 35, 36, 36, 36, 36, 36, 36, 37, 38, 39, 40, 41,
	42, 43, 44, 45, 46, 35, 36, 36, 36, 47, 36, 48, 5, 49, 50, 51, 52, 53, 44, 54,
	55, 46, 56, 47, 56, 56, 56, 48, 57, 58, 59, 42, 53, 34, 60, 14, 61, 62, 62, 46,
	62, 41, 62, 63, 64, 13, 65, 66, 42, 67, 43, 68, 69, 55, 55, 61, 61, 70, 61, 32,
	71, 2, 34, 43, 2, 66, 72, 73, 74, 61, 61, 69, 74, 75, 75, 76, 77, 65, 78, 79,
	80, 81, 24, 54, 33, 82, 83, 84, 82, 83, 84, 85, 84, 78, 15, 66, 86, 87, 38, 24,
	88, 59, 89, 24, 83, 90, 40, 78, 91, 65, 7, 86, 48, 92, 82, 92, 49, 65, 59, 13,
	59, 77, 84, 81, 38, 59, 59, 66, 49, 49, 5, 89, 49, 40, 93, 93, 94, 83, 95, 1,
	88, 88, 89, 15, 40, 96, 96, 10, 89, 255, 255, 255, 255, 255, 255, 49, 97, 98, 61, 40,
	34, 97, 7, 99, 80, 255, 255, 255, 255, 255, 255, 59, 60, 49, 15, 100,
}; /* 256 values */

static const struct asset2 badgey = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) badgey_colormap,
	.pixel = (const unsigned char *) badgey_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/badgey.png Fri Apr 12 15:39:57 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/grass.png Fri Apr 12 15:41:46 2024. */
static const uint16_t grass_colormap[69] = {
	0x0b40, 0x1c00, 0x5482, 0x3440, 0x2cc0, 0x3d00, 0x2440, 0x2c40, 0x1300, 0x2380, 0x1b00, 0x3b00,
	0x0b00, 0x1c40, 0x0b80, 0x2400, 0x2bc0, 0x4cc2, 0x2480, 0x4500, 0x3441, 0x1bc0, 0x33c1, 0x2b40,
	0x4340, 0x4bc1, 0x3c80, 0x2c00, 0x4440, 0x5404, 0x4402, 0x3cc0, 0x4501, 0x54c2, 0x23c0, 0x1b80,
	0x3bc0, 0x3b80, 0x3c41, 0x3d40, 0x3480, 0x4482, 0x4481, 0x5c42, 0x4480, 0x3c40, 0x4bc2, 0x3340,
	0x13c0, 0x3481, 0x3380, 0x5403, 0x4401, 0x4442, 0x4c00, 0x4c82, 0x6d02, 0x4d01, 0x33c0, 0x2b81,
	0x34c0, 0x4c80, 0x2c80, 0x2b80, 0x1380, 0x22c0, 0x2340, 0x4c41, 0x0380,
}; /* 69 values */

static const uint8_t grass_data[256] = {
	0, 1, 2, 3, 4, 4, 5, 6, 7, 0, 8, 9, 10, 11, 9, 7, 12, 9, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 1, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 7,
	33, 3, 1, 34, 35, 36, 37, 37, 38, 39, 40, 41, 7, 42, 43, 44, 27, 45, 27, 46,
	16, 47, 48, 49, 50, 15, 45, 18, 26, 3, 38, 27, 21, 37, 16, 3, 51, 52, 16, 27,
	16, 1, 21, 27, 16, 31, 53, 54, 36, 55, 56, 57, 45, 58, 22, 59, 21, 60, 20, 36,
	10, 31, 26, 26, 9, 53, 61, 28, 40, 6, 62, 63, 23, 64, 1, 65, 66, 67, 29, 16,
	68, 34, 15, 4, 34, 16, 48, 38, 15, 0, 8, 9, 10, 11, 9, 7, 0, 1, 2, 3,
	4, 4, 5, 6, 19, 20, 21, 1, 22, 23, 24, 25, 12, 9, 13, 14, 15, 16, 17, 18,
	33, 3, 1, 34, 35, 36, 37, 37, 26, 27, 28, 29, 30, 31, 32, 7, 27, 45, 27, 46,
	16, 47, 48, 49, 38, 39, 40, 41, 7, 42, 43, 44, 21, 37, 16, 3, 51, 52, 16, 27,
	50, 15, 45, 18, 26, 3, 38, 27, 36, 55, 56, 57, 45, 58, 22, 59, 16, 1, 21, 27,
	16, 31, 53, 54, 9, 53, 61, 28, 40, 6, 62, 63, 21, 60, 20, 36, 10, 31, 26, 26,
	68, 34, 15, 4, 34, 16, 48, 38, 23, 64, 1, 65, 66, 67, 29, 16,
}; /* 256 values */

static const struct asset2 grass = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) grass_colormap,
	.pixel = (const unsigned char *) grass_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/grass.png Fri Apr 12 15:41:46 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/tree.png Fri Apr 12 15:43:32 2024. */
static const uint16_t tree_colormap[70] = {
	0x0b80, 0x1c00, 0x5441, 0x2c40, 0x2cc0, 0x0a80, 0x2b03, 0x2b81, 0x1b00, 0x3b00, 0x1b80, 0x0b00,
	0x1c40, 0x2c00, 0x00c0, 0x0140, 0x3c01, 0x2b00, 0x4380, 0x4401, 0x3c40, 0x4440, 0x5403, 0x4403,
	0x09c0, 0x01c0, 0x0200, 0x1b81, 0x3c00, 0x3b80, 0x3c41, 0x3d40, 0x4441, 0x2c01, 0x0280, 0x0300,
	0x0c00, 0x2a80, 0x0a00, 0x3b81, 0x3cc0, 0x1a80, 0x2b80, 0x2c03, 0x4540, 0x18c0, 0x44c3, 0x54c1,
	0x1b01, 0x1940, 0x38c0, 0x50c0, 0x5281, 0x4400, 0x44c1, 0x5401, 0x4385, 0x5201, 0x54c5, 0x44c0,
	0x51c1, 0x4383, 0x6543, 0x4405, 0x4443, 0x5303, 0x4280, 0x0380, 0x5285, 0x5205,
}; /* 70 values */

static const uint8_t tree_data[256] = {
	0, 1, 2, 3, 4, 4, 3, 5, 5, 5, 6, 7, 8, 9, 10, 3, 11, 10, 12, 0,
	13, 8, 14, 14, 14, 15, 15, 15, 16, 17, 18, 19, 20, 3, 21, 22, 23, 24, 14, 14,
	25, 26, 26, 15, 27, 28, 29, 29, 30, 31, 20, 32, 33, 14, 14, 15, 34, 35, 26, 26,
	24, 29, 36, 30, 29, 1, 20, 33, 14, 14, 14, 15, 35, 0, 35, 34, 25, 37, 13, 13,
	13, 1, 1, 38, 15, 25, 26, 34, 0, 0, 0, 0, 25, 25, 39, 7, 10, 40, 30, 41,
	26, 26, 26, 34, 0, 0, 34, 34, 34, 25, 3, 42, 42, 0, 1, 5, 26, 34, 26, 35,
	0, 0, 34, 35, 35, 5, 36, 30, 13, 11, 11, 26, 26, 34, 34, 26, 34, 0, 35, 35,
	26, 43, 31, 12, 44, 30, 10, 5, 26, 26, 25, 45, 14, 25, 26, 26, 34, 33, 46, 3,
	47, 20, 1, 1, 48, 26, 49, 50, 51, 52, 53, 22, 23, 40, 54, 3, 13, 20, 13, 55,
	13, 56, 36, 57, 51, 58, 20, 32, 3, 32, 2, 59, 1, 29, 13, 20, 22, 19, 13, 60,
	51, 61, 20, 3, 40, 3, 30, 13, 28, 2, 62, 47, 20, 13, 63, 51, 51, 39, 1, 13,
	42, 40, 32, 21, 10, 64, 59, 21, 20, 12, 65, 51, 51, 66, 30, 28, 8, 40, 20, 20,
	67, 13, 1, 4, 1, 13, 68, 57, 57, 69, 1, 8, 10, 32, 22, 13,
}; /* 256 values */

static const struct asset2 tree = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) tree_colormap,
	.pixel = (const unsigned char *) tree_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/tree.png Fri Apr 12 15:43:32 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/water.png Mon Apr 15 15:24:10 2024. */
static const uint16_t water_colormap[23] = {
	0x8d9b, 0x0a92, 0x12d3, 0x2354, 0x0251, 0x1b13, 0x0ad2, 0x0292, 0x0291, 0x12d2, 0x2314, 0x0a91,
	0x0252, 0x1313, 0x1312, 0x1b14, 0x2b54, 0x1ad3, 0x2355, 0x2b55, 0x1292, 0x0b12, 0x2b95,
}; /* 23 values */

static const uint8_t water_data[256] = {
	0, 1, 2, 3, 1, 1, 4, 4, 1, 5, 6, 6, 1, 7, 8, 0, 9, 0, 0, 0,
	10, 11, 4, 4, 12, 1, 6, 8, 6, 6, 7, 7, 11, 2, 2, 1, 0, 0, 1, 1,
	2, 8, 7, 6, 7, 6, 13, 7, 8, 8, 13, 2, 7, 7, 2, 0, 0, 8, 6, 13,
	6, 6, 6, 14, 5, 2, 15, 1, 7, 7, 7, 1, 2, 0, 0, 16, 3, 13, 8, 6,
	6, 2, 7, 7, 6, 7, 2, 7, 4, 4, 1, 0, 0, 0, 7, 7, 6, 3, 6, 1,
	6, 2, 17, 18, 7, 8, 1, 11, 1, 1, 2, 0, 0, 0, 10, 8, 8, 6, 7, 6,
	7, 4, 9, 19, 20, 6, 2, 6, 21, 5, 0, 0, 1, 7, 8, 8, 5, 1, 2, 3,
	1, 1, 4, 4, 12, 1, 6, 0, 6, 6, 7, 7, 9, 6, 1, 11, 10, 11, 4, 4,
	2, 8, 7, 6, 0, 6, 13, 7, 11, 2, 2, 1, 11, 9, 1, 1, 1, 8, 6, 13,
	6, 6, 0, 14, 8, 8, 13, 2, 7, 7, 7, 2, 2, 8, 9, 16, 3, 13, 8, 6,
	5, 2, 15, 1, 7, 7, 7, 1, 4, 4, 1, 1, 2, 2, 7, 0, 0, 0, 7, 7,
	6, 7, 2, 7, 7, 8, 1, 11, 1, 1, 6, 6, 6, 3, 2, 0, 6, 2, 17, 18,
	7, 4, 9, 19, 20, 6, 2, 6, 22, 5, 10, 8, 2, 0, 0, 6,
}; /* 256 values */

static const struct asset2 water = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) water_colormap,
	.pixel = (const unsigned char *) water_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/water.png Mon Apr 15 15:24:10 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/mountain.png Fri Apr 12 15:44:50 2024. */
static const uint16_t mountain_colormap[144] = {
	0x0b40, 0x1c00, 0x5482, 0x3440, 0x2cc0, 0x3d00, 0x2440, 0x2c40, 0x1300, 0x2380, 0x1b00, 0x3b00,
	0x0b00, 0x1c40, 0x0b80, 0x2400, 0x2bc0, 0x4cc2, 0x2480, 0x4500, 0x3441, 0x1bc0, 0x33c1, 0x2b40,
	0x4340, 0x4bc1, 0x3c80, 0x2c00, 0x4440, 0x5404, 0x4402, 0x3cc0, 0x4501, 0x5b92, 0x6415, 0x23c0,
	0x1b80, 0x3bc0, 0x3b80, 0x3c41, 0x3d40, 0x3480, 0x4482, 0x4481, 0x5c42, 0x3bcc, 0x530f, 0x6391,
	0x5393, 0x4bc2, 0x3340, 0x13c0, 0x3481, 0x3380, 0x3c40, 0x29cc, 0x5310, 0x9d56, 0x4b0f, 0x5403,
	0x4401, 0x2a0d, 0x3a4d, 0x3bce, 0x33c0, 0x2b81, 0x34c0, 0x7456, 0x8cd6, 0x324e, 0x2a0c, 0x324d,
	0x324c, 0x7c55, 0x2c80, 0x2b80, 0x1380, 0x6c56, 0x6390, 0x8494, 0x21cb, 0x3a8e, 0x6bd2, 0x32d0,
	0x5bd4, 0x6bd1, 0x3a4c, 0x7412, 0x94d5, 0x3ad0, 0x4b10, 0x63d4, 0x5351, 0x29c9, 0x3a4b, 0x5b91,
	0x8cd5, 0x42cf, 0x5b51, 0x8d19, 0x5bd3, 0x63d3, 0x6c14, 0x9d57, 0x7c53, 0x428e, 0x9d16, 0x7c54,
	0x7413, 0xa597, 0xb5d9, 0xa558, 0x6b90, 0x9515, 0xbe1a, 0xadd8, 0x8c94, 0xa557, 0x9517, 0x4a8d,
	0x5b0e, 0x6c12, 0xad97, 0xd6dc, 0xbe19, 0xa598, 0x6bd3, 0x8454, 0x84d5, 0x5b50, 0x634f, 0x4acd,
	0x6c13, 0x63d2, 0x7414, 0x5b4f, 0x9516, 0xadd9, 0x8495, 0x4311, 0x7455, 0x7c94, 0x4b11, 0x4b51,
}; /* 144 values */

static const uint8_t mountain_data[256] = {
	0, 1, 2, 3, 4, 4, 5, 6, 7, 0, 8, 9, 10, 11, 9, 7, 12, 9, 13, 14,
	15, 16, 17, 18, 19, 20, 21, 1, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 7,
	33, 34, 1, 35, 36, 37, 38, 38, 39, 40, 41, 42, 7, 43, 44, 45, 46, 47, 48, 49,
	16, 50, 51, 52, 53, 15, 54, 18, 26, 3, 39, 55, 56, 57, 58, 3, 59, 60, 16, 27,
	16, 1, 21, 27, 16, 31, 55, 55, 61, 62, 55, 63, 54, 64, 22, 65, 21, 66, 20, 67,
	68, 69, 61, 70, 71, 72, 55, 73, 41, 6, 74, 75, 23, 76, 77, 47, 78, 79, 56, 55,
	61, 80, 81, 82, 83, 16, 51, 39, 15, 84, 85, 86, 56, 87, 88, 71, 89, 71, 90, 90,
	69, 4, 5, 6, 91, 92, 93, 94, 95, 33, 47, 96, 47, 69, 81, 56, 97, 98, 99, 18,
	100, 98, 82, 92, 33, 101, 102, 96, 103, 104, 105, 106, 107, 108, 109, 110, 92, 107, 111, 107,
	101, 56, 98, 112, 113, 109, 79, 79, 114, 115, 116, 79, 58, 82, 117, 118, 46, 119, 46, 120,
	120, 121, 114, 122, 123, 124, 125, 126, 98, 95, 127, 128, 92, 56, 129, 46, 130, 131, 106, 124,
	117, 106, 101, 33, 126, 126, 87, 46, 98, 132, 133, 134, 112, 78, 135, 136, 137, 138, 139, 92,
	126, 107, 136, 107, 101, 102, 140, 140, 116, 87, 102, 107, 141, 132, 142, 143,
}; /* 256 values */

static const struct asset2 mountain = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) mountain_colormap,
	.pixel = (const unsigned char *) mountain_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/mountain.png Fri Apr 12 15:44:50 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/town.png Sat Apr 13 10:39:45 2024. */
static const uint16_t town_colormap[34] = {
	0xffdf, 0xd69a, 0x5482, 0x3440, 0x2cc0, 0x3d00, 0x2440, 0x2c40, 0x0b40, 0x1300, 0x2380, 0x1b00,
	0x3b00, 0x1c40, 0x0b80, 0x5404, 0x2480, 0x1bc0, 0x1c00, 0x4440, 0x9cd3, 0x4501, 0x23c0, 0x6b4d,
	0x0380, 0x2400, 0x2bc0, 0x13c0, 0x3c41, 0x2b40, 0x1380, 0x22c0, 0x2340, 0x4c41,
}; /* 34 values */

static const uint8_t town_data[256] = {
	0, 1, 2, 3, 4, 4, 5, 6, 7, 8, 9, 10, 11, 12, 0, 1, 0, 1, 13, 14,
	15, 15, 15, 16, 15, 15, 17, 18, 15, 15, 0, 1, 0, 1, 19, 15, 0, 20, 21, 7,
	0, 20, 18, 22, 0, 20, 0, 1, 0, 1, 15, 15, 0, 20, 15, 15, 0, 20, 15, 15,
	0, 20, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	0, 1, 0, 1, 23, 0, 0, 0, 0, 0, 0, 0, 1, 23, 0, 1, 0, 1, 0, 1,
	23, 0, 0, 0, 0, 0, 0, 0, 1, 23, 0, 1, 0, 1, 0, 1, 23, 0, 0, 23,
	23, 23, 0, 0, 1, 23, 0, 1, 0, 1, 0, 0, 0, 0, 23, 23, 23, 23, 23, 0,
	0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 23, 23, 23, 23, 23, 0, 0, 0, 0, 1,
	0, 1, 0, 0, 0, 0, 23, 23, 23, 23, 23, 0, 0, 0, 0, 1, 0, 1, 0, 0,
	0, 0, 23, 23, 23, 23, 23, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 23, 23,
	23, 23, 23, 0, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 23, 23, 23, 23, 23, 0,
	0, 0, 0, 1, 0, 1, 0, 0, 0, 0, 23, 23, 23, 23, 23, 0, 0, 0, 0, 1,
	24, 22, 25, 4, 22, 26, 27, 28, 29, 30, 18, 31, 32, 33, 15, 26,
}; /* 256 values */

static const struct asset2 town = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) town_colormap,
	.pixel = (const unsigned char *) town_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/town.png Sat Apr 13 10:39:45 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/wall.png Sat Apr 13 11:08:48 2024. */
static const uint16_t wall_colormap[9] = {
	0xac8f, 0x7b8d, 0x83cd, 0xa44f, 0xa48f, 0x940e, 0x83ce, 0x9c4f, 0x8bce,
}; /* 9 values */

static const uint8_t wall_data[256] = {
	0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	2, 0, 0, 0, 0, 0, 0, 0, 3, 0, 1, 0, 0, 4, 3, 0, 0, 0, 5, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 6, 7, 5,
	0, 0, 0, 4, 0, 2, 7, 0, 5, 7, 0, 0, 0, 0, 0, 0, 5, 0, 8, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
	0, 6, 2, 0, 0, 0, 0, 1, 0, 8, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 1, 0, 0, 0, 0, 3, 0, 1, 0, 1, 0, 0, 0, 2, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 3, 0, 0, 0, 5, 0, 0, 6, 7, 5,
	0, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0, 8, 0,
	0, 2, 7, 0, 5, 7, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 1, 0, 8, 0, 0, 1, 0, 0, 0, 0, 6, 2, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0,
}; /* 256 values */

static const struct asset2 wall = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) wall_colormap,
	.pixel = (const unsigned char *) wall_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/wall.png Sat Apr 13 11:08:48 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/bricks.png Sat Apr 13 11:09:27 2024. */
static const uint16_t bricks_colormap[2] = {
	0xb410, 0x8082,
}; /* 2 values */

static const uint8_t bricks_data[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
	1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
	1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
	0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
	1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1,
}; /* 256 values */

static const struct asset2 bricks = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) bricks_colormap,
	.pixel = (const unsigned char *) bricks_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/bricks.png Sat Apr 13 11:09:27 2024 */

/* Begin code generated by png-to-badge-asset from ../source/apps/badgey_assets/desert.png Sat Apr 13 14:09:24 2024. */
static const uint16_t desert_colormap[13] = {
	0x9bcb, 0x938b, 0xa40d, 0x938a, 0x9349, 0x934a, 0x9bcc, 0x9c0d, 0xa44e, 0xa40e, 0xac4f, 0x9b8b,
	0x9c0c,
}; /* 13 values */

static const uint8_t desert_data[256] = {
	0, 1, 2, 2, 0, 3, 0, 2, 0, 3, 4, 5, 0, 0, 6, 6, 7, 2, 8, 2,
	0, 3, 1, 1, 3, 1, 4, 4, 1, 3, 6, 2, 9, 2, 0, 1, 3, 5, 3, 3,
	3, 3, 4, 4, 3, 5, 6, 10, 2, 1, 3, 3, 3, 3, 11, 0, 11, 3, 3, 3,
	3, 3, 6, 8, 0, 4, 3, 11, 0, 6, 0, 6, 0, 3, 3, 0, 0, 11, 6, 2,
	3, 4, 4, 6, 2, 6, 0, 6, 0, 3, 3, 1, 6, 6, 2, 7, 11, 1, 3, 12,
	2, 6, 6, 12, 0, 3, 0, 0, 6, 6, 2, 7, 11, 11, 3, 11, 6, 6, 2, 6,
	3, 3, 2, 6, 1, 1, 6, 2, 0, 3, 4, 5, 0, 0, 6, 6, 0, 1, 2, 2,
	0, 3, 0, 2, 3, 1, 4, 4, 1, 3, 6, 2, 7, 2, 8, 2, 0, 3, 1, 1,
	3, 3, 4, 4, 3, 5, 6, 10, 9, 2, 0, 1, 3, 5, 3, 3, 11, 3, 3, 3,
	3, 3, 6, 8, 2, 1, 3, 3, 3, 3, 11, 0, 0, 3, 3, 0, 0, 11, 6, 2,
	0, 4, 3, 11, 0, 6, 0, 6, 0, 3, 3, 1, 6, 6, 2, 7, 3, 4, 4, 6,
	2, 6, 0, 6, 0, 3, 0, 0, 6, 6, 2, 7, 11, 1, 3, 12, 2, 6, 6, 12,
	3, 3, 2, 6, 1, 1, 6, 2, 11, 11, 3, 11, 6, 6, 2, 6,
}; /* 256 values */

static const struct asset2 desert = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) desert_colormap,
	.pixel = (const unsigned char *) desert_data,
};
/* End of code generated by png-to-badge-asset from ../source/apps/badgey_assets/desert.png Sat Apr 13 14:09:24 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/woodfloor.png Sun Apr 14 10:32:36 2024. */
static const uint16_t woodfloor_colormap[2] = {
	0x8205, 0xa2c7,
}; /* 2 values */

static const uint8_t woodfloor_data[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1,
	1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
	1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1,
	1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
}; /* 256 values */

static const struct asset2 woodfloor = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) woodfloor_colormap,
	.pixel = (const unsigned char *) woodfloor_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/woodfloor.png Sun Apr 14 10:32:36 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/cave.png Sun Apr 14 17:10:39 2024. */
static const uint16_t cave_colormap[89] = {
	0x3204, 0x3206, 0x2204, 0x4286, 0x74cc, 0x754c, 0x74ca, 0x754a, 0x4284, 0x4306, 0x5448, 0x64ca,
	0x8d4c, 0x9e51, 0x5306, 0x638a, 0x9d4c, 0x5286, 0x5386, 0x644a, 0x8dcc, 0x744a, 0x5388, 0x4308,
	0x5308, 0x6446, 0x6388, 0x6308, 0x738a, 0x8cca, 0x9dce, 0x8d4e, 0x6448, 0x8c4c, 0x738c, 0x7448,
	0x9cce, 0x3184, 0x9ccc, 0x9c4e, 0x2102, 0x0000, 0x1000, 0x1082, 0x630a, 0xad51, 0xbdd1, 0xadce,
	0x9dd1, 0x8cce, 0x4204, 0xad53, 0x2104, 0xadd1, 0xaed3, 0x9d4e, 0xbdd3, 0xae53, 0xbd55, 0x4206,
	0x8b8c, 0xacce, 0x8ccc, 0x8c4e, 0x2082, 0x3186, 0x5288, 0xacd1, 0xbe51, 0xae4e, 0x9e4e, 0xbdd5,
	0x4186, 0x5208, 0xce53, 0xbe4e, 0x628a, 0x730a, 0x730c, 0x744e, 0x9c51, 0x9cd1, 0xac4e, 0x9d51,
	0xbd53, 0xacd3, 0xbd51, 0x8d51, 0xcd53,
}; /* 89 values */

static const uint8_t cave_data[256] = {
	0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 9, 10, 11, 12, 5, 13, 9, 3, 3, 14,
	15, 6, 16, 12, 17, 18, 18, 18, 19, 20, 21, 22, 19, 23, 24, 25, 26, 27, 28, 29,
	3, 14, 22, 25, 6, 30, 12, 31, 19, 22, 19, 32, 4, 33, 34, 27, 28, 33, 33, 26,
	6, 35, 12, 30, 6, 22, 19, 26, 36, 34, 37, 17, 22, 24, 28, 33, 16, 38, 29, 21,
	31, 19, 32, 21, 39, 40, 41, 42, 43, 43, 40, 44, 45, 46, 47, 48, 27, 32, 33, 49,
	33, 41, 41, 41, 41, 43, 43, 50, 33, 45, 30, 4, 47, 4, 36, 51, 39, 41, 41, 41,
	41, 43, 52, 17, 33, 45, 53, 30, 54, 55, 56, 51, 34, 41, 41, 41, 41, 43, 37, 27,
	36, 45, 56, 16, 57, 31, 58, 51, 40, 43, 43, 43, 43, 43, 59, 60, 61, 56, 36, 55,
	5, 62, 36, 63, 64, 40, 43, 52, 65, 59, 66, 33, 67, 56, 68, 69, 70, 48, 71, 34,
	37, 37, 37, 72, 73, 44, 15, 63, 55, 74, 75, 68, 13, 55, 45, 28, 23, 66, 76, 76,
	77, 78, 33, 36, 53, 53, 47, 55, 6, 62, 12, 49, 22, 79, 80, 81, 67, 82, 67, 83,
	55, 53, 38, 30, 6, 62, 31, 81, 81, 67, 51, 84, 85, 84, 86, 45, 55, 55, 55, 45,
	31, 87, 31, 36, 81, 67, 84, 84, 88, 86, 84, 86, 46, 86, 86, 46,
}; /* 256 values */

static const struct asset2 cave = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) cave_colormap,
	.pixel = (const unsigned char *) cave_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/cave.png Sun Apr 14 17:10:39 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/citizen1.png Tue Apr 16 09:12:22 2024. */
static const uint16_t citizen1_colormap[12] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,

}; /* 12 values */

static const uint8_t citizen1_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 1, 2, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 3,
	3, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 4, 5, 3, 3, 4, 5, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 5, 5, 5, 5, 6, 5, 5, 5, 255, 255, 255, 255,
	255, 255, 255, 5, 5, 255, 5, 5, 5, 5, 255, 5, 5, 255, 255, 255, 255, 255, 255, 6,
	255, 255, 5, 5, 5, 5, 255, 255, 5, 255, 255, 255, 255, 255, 255, 6, 255, 255, 5, 5,
	5, 5, 255, 255, 5, 255, 255, 255, 255, 255, 255, 3, 255, 255, 5, 5, 5, 5, 255, 255,
	3, 255, 255, 255, 255, 255, 255, 255, 255, 255, 7, 7, 7, 7, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 8, 7, 7, 7, 7, 7, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 7, 7, 255, 255, 7, 7, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 7, 7, 255,
	255, 7, 7, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 8, 8, 255, 255, 7, 7, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 9, 0, 255, 255, 9, 9, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 10, 0, 0, 255, 255, 0, 0, 11, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 citizen1 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) citizen1_colormap,
	.pixel = (const unsigned char *) citizen1_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/citizen1.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/citizen2.png Tue Apr 16 09:12:22 2024. */
static const uint16_t citizen2_colormap[20] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141,
}; /* 20 values */

static const uint8_t citizen2_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 12, 12, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 13, 14, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 15,
	15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 16, 16, 13, 13, 16, 16, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 16, 16, 16, 16, 16, 16, 16, 16, 255, 255, 255, 255,
	255, 255, 255, 16, 16, 255, 16, 16, 16, 16, 255, 16, 16, 255, 255, 255, 255, 255, 255, 16,
	255, 255, 16, 16, 16, 16, 255, 255, 16, 255, 255, 255, 255, 255, 255, 16, 255, 255, 16, 16,
	16, 16, 255, 255, 16, 255, 255, 255, 255, 255, 255, 17, 255, 255, 16, 16, 16, 16, 255, 255,
	14, 255, 255, 255, 255, 255, 255, 255, 255, 255, 18, 18, 18, 18, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 18, 18, 18, 18, 18, 18, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 18, 18, 255, 255, 18, 18, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 18, 18, 255,
	255, 18, 18, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 18, 18, 255, 255, 18, 18, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 9, 19, 255, 255, 9, 9, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 10, 0, 0, 255, 255, 0, 0, 11, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 citizen2 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) citizen2_colormap,
	.pixel = (const unsigned char *) citizen2_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/citizen2.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/citizen3.png Tue Apr 16 09:12:22 2024. */
static const uint16_t citizen3_colormap[29] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f,
}; /* 29 values */

static const uint8_t citizen3_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 20, 20, 20, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 20, 21, 21, 20, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 20, 15,
	15, 20, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 20, 15, 15, 20, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 22, 5, 23, 24, 5, 13, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 15, 5, 25, 5, 26, 15, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 15, 5, 5, 5, 5, 13, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 15, 20, 5,
	5, 20, 21, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 15, 255, 27, 0, 255, 13, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 15, 5, 25, 25, 5, 15, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 5, 5, 5, 5, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 5, 5, 5, 5, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 5, 5,
	5, 5, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 28, 255, 255, 28, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 13, 255, 255, 13, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 0, 0, 255, 255, 0, 0, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 citizen3 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) citizen3_colormap,
	.pixel = (const unsigned char *) citizen3_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/citizen3.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/citizen4.png Tue Apr 16 09:12:22 2024. */
static const uint16_t citizen4_colormap[42] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f, 0x2906, 0x2106, 0xcc08, 0xc3c7, 0xc3c8, 0xc386, 0xcc0a,
	0x9801, 0x9903, 0x9842, 0x98c3, 0x98c4, 0x8881,
}; /* 42 values */

static const uint8_t citizen4_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 29, 30, 30, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 30, 31, 32, 30, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 30, 33,
	34, 30, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 30, 33, 34, 30, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 35, 36, 35, 35, 36, 33, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 33, 37, 37, 38, 38, 34, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 33, 37, 36, 36, 38, 34, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 33, 30, 37,
	37, 30, 32, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 33, 255, 27, 0, 255, 33, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 34, 39, 39, 36, 37, 34, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 38, 36, 36, 37, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 38, 36, 36, 39, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 40, 36,
	36, 39, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 33, 255, 255, 33, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 33, 255, 255, 35, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 36, 41, 255, 255, 36, 41, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 citizen4 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) citizen4_colormap,
	.pixel = (const unsigned char *) citizen4_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/citizen4.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/citizen5.png Tue Apr 16 09:12:22 2024. */
static const uint16_t citizen5_colormap[55] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f, 0x2906, 0x2106, 0xcc08, 0xc3c7, 0xc3c8, 0xc386, 0xcc0a,
	0x9801, 0x9903, 0x9842, 0x98c3, 0x98c4, 0x8881, 0xaac0, 0xe616, 0xddd5, 0xa2c1, 0x12c1, 0xa44f,
	0xe615, 0x1a81, 0x1ac3, 0x2140, 0x0940, 0x0101, 0x00c0,
}; /* 55 values */

static const uint8_t citizen5_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 42, 42, 42, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 42, 43, 43, 42, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 42, 44,
	44, 42, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 45, 44, 44, 42, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 44, 46, 47, 47, 46, 44, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 44, 46, 46, 46, 46, 48, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 44, 49, 46, 46, 50, 43, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 44, 45, 46,
	46, 42, 43, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 48, 255, 51, 52, 255, 44, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 44, 46, 46, 46, 46, 44, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 46, 46, 46, 46, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 46, 46, 46, 46, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 50, 46,
	46, 46, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 44, 255, 255, 44, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 44, 255, 255, 43, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 53, 53, 255, 255, 54, 54, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 citizen5 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) citizen5_colormap,
	.pixel = (const unsigned char *) citizen5_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/citizen5.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/citizen6.png Tue Apr 16 09:12:22 2024. */
static const uint16_t citizen6_colormap[65] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f, 0x2906, 0x2106, 0xcc08, 0xc3c7, 0xc3c8, 0xc386, 0xcc0a,
	0x9801, 0x9903, 0x9842, 0x98c3, 0x98c4, 0x8881, 0xaac0, 0xe616, 0xddd5, 0xa2c1, 0x12c1, 0xa44f,
	0xe615, 0x1a81, 0x1ac3, 0x2140, 0x0940, 0x0101, 0x00c0, 0xde9a, 0xcc89, 0x9081, 0x9080, 0x90c2,
	0x9043, 0x9144, 0xdd0a, 0x084b, 0x108b,
}; /* 65 values */

static const uint8_t citizen6_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 5, 55, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 56, 56, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 56,
	56, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 57, 58, 56, 56, 58, 58, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 5, 58, 5, 5, 5, 5, 58, 5, 255, 255, 255, 255,
	255, 255, 255, 58, 59, 255, 59, 58, 58, 58, 255, 58, 58, 255, 255, 255, 255, 255, 255, 5,
	255, 255, 5, 5, 5, 55, 255, 255, 5, 255, 255, 255, 255, 255, 255, 58, 255, 255, 60, 60,
	59, 57, 255, 255, 61, 255, 255, 255, 255, 255, 255, 62, 255, 255, 63, 63, 63, 64, 255, 255,
	56, 255, 255, 255, 255, 255, 255, 255, 255, 255, 63, 63, 63, 63, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 64, 63, 63, 63, 63, 64, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 63, 63, 255, 255, 63, 63, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 63, 63, 255,
	255, 63, 63, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 63, 63, 255, 255, 63, 63, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 9, 0, 255, 255, 9, 9, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 10, 0, 0, 255, 255, 0, 0, 9, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 citizen6 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) citizen6_colormap,
	.pixel = (const unsigned char *) citizen6_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/citizen6.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/goldrobot.png Tue Apr 16 09:12:22 2024. */
static const uint16_t goldrobot_colormap[104] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f, 0x2906, 0x2106, 0xcc08, 0xc3c7, 0xc3c8, 0xc386, 0xcc0a,
	0x9801, 0x9903, 0x9842, 0x98c3, 0x98c4, 0x8881, 0xaac0, 0xe616, 0xddd5, 0xa2c1, 0x12c1, 0xa44f,
	0xe615, 0x1a81, 0x1ac3, 0x2140, 0x0940, 0x0101, 0x00c0, 0xde9a, 0xcc89, 0x9081, 0x9080, 0x90c2,
	0x9043, 0x9144, 0xdd0a, 0x084b, 0x108b, 0xd58e, 0xc488, 0xccc7, 0xcdd2, 0xd5d2, 0xcd0d, 0xcd0c,
	0xe715, 0xde11, 0xcd4c, 0xc4ca, 0xcd4d, 0xd5d0, 0xcdd0, 0xcd0b, 0xcd0e, 0xdd8e, 0xde51, 0xd611,
	0xbc48, 0xd58f, 0xccc9, 0xddce, 0xde53, 0xde52, 0xc489, 0xccca, 0xd54d, 0xde10, 0xe693, 0xcd0a,
	0xddcf, 0xdd8b, 0xddcc, 0xdd8c, 0xde0f, 0xde50, 0xcd8e, 0xde94,
}; /* 104 values */

static const uint8_t goldrobot_data[256] = {
	255, 255, 255, 255, 255, 255, 65, 18, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 66, 67, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 68, 69,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 70, 71, 72, 72, 73, 74, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 75, 75, 76, 77, 78, 18, 71, 79, 255, 255, 255, 255, 255,
	255, 255, 255, 80, 255, 255, 81, 82, 255, 255, 77, 255, 255, 255, 255, 255, 255, 255, 255, 83,
	255, 84, 79, 85, 86, 255, 87, 255, 255, 255, 255, 255, 255, 255, 255, 88, 255, 89, 90, 91,
	88, 255, 79, 92, 255, 255, 255, 255, 255, 255, 255, 77, 255, 93, 255, 255, 77, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 93, 255, 255, 85, 94, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 65, 255, 255, 95, 94, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 96, 255, 255, 255, 65, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 97, 87, 255, 255,
	255, 98, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 98, 255, 255, 255, 255, 99, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 100, 255, 255, 255, 255, 77, 89, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 101, 255, 255, 255, 255, 102, 103, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 goldrobot = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) goldrobot_colormap,
	.pixel = (const unsigned char *) goldrobot_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/goldrobot.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/guard.png Tue Apr 16 09:12:22 2024. */
static const uint16_t guard_colormap[108] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f, 0x2906, 0x2106, 0xcc08, 0xc3c7, 0xc3c8, 0xc386, 0xcc0a,
	0x9801, 0x9903, 0x9842, 0x98c3, 0x98c4, 0x8881, 0xaac0, 0xe616, 0xddd5, 0xa2c1, 0x12c1, 0xa44f,
	0xe615, 0x1a81, 0x1ac3, 0x2140, 0x0940, 0x0101, 0x00c0, 0xde9a, 0xcc89, 0x9081, 0x9080, 0x90c2,
	0x9043, 0x9144, 0xdd0a, 0x084b, 0x108b, 0xd58e, 0xc488, 0xccc7, 0xcdd2, 0xd5d2, 0xcd0d, 0xcd0c,
	0xe715, 0xde11, 0xcd4c, 0xc4ca, 0xcd4d, 0xd5d0, 0xcdd0, 0xcd0b, 0xcd0e, 0xdd8e, 0xde51, 0xd611,
	0xbc48, 0xd58f, 0xccc9, 0xddce, 0xde53, 0xde52, 0xc489, 0xccca, 0xd54d, 0xde10, 0xe693, 0xcd0a,
	0xddcf, 0xdd8b, 0xddcc, 0xdd8c, 0xde0f, 0xde50, 0xcd8e, 0xde94, 0x0000, 0xddcb, 0x7bcd, 0xf5c0,

}; /* 108 values */

static const uint8_t guard_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 6, 6, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 6, 6, 6, 6, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 6, 104,
	104, 6, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 105, 105, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 6, 6, 6, 104, 104, 5, 104, 6, 255, 255, 255, 255,
	255, 255, 255, 6, 6, 6, 6, 6, 5, 104, 106, 6, 6, 255, 255, 255, 255, 255, 6, 6,
	255, 6, 6, 5, 104, 106, 6, 255, 6, 6, 255, 255, 255, 255, 6, 6, 255, 255, 5, 104,
	106, 6, 255, 255, 6, 6, 255, 255, 255, 255, 6, 6, 255, 255, 104, 106, 6, 6, 255, 255,
	6, 6, 255, 255, 255, 255, 255, 104, 255, 255, 104, 107, 107, 104, 255, 255, 104, 255, 255, 255,
	255, 255, 255, 255, 255, 6, 6, 6, 6, 6, 6, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	6, 6, 6, 255, 255, 6, 6, 6, 255, 255, 255, 255, 255, 255, 255, 255, 6, 6, 255, 255,
	255, 255, 6, 6, 255, 255, 255, 255, 255, 255, 255, 255, 6, 6, 255, 255, 255, 255, 6, 6,
	255, 255, 255, 255, 255, 255, 255, 255, 104, 104, 255, 255, 255, 255, 104, 104, 255, 255, 255, 255,
	255, 255, 255, 104, 104, 104, 255, 255, 255, 255, 104, 104, 104, 255, 255, 255,
}; /* 256 values */

static const struct asset2 guard = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) guard_colormap,
	.pixel = (const unsigned char *) guard_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/guard.png Tue Apr 16 09:12:22 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/blackprobe.png Tue Apr 16 09:12:22 2024. */
static const uint16_t blackprobe_colormap[154] = {
	0x2101, 0x7281, 0x6a81, 0x7ac1, 0xf7de, 0xffdf, 0xf79e, 0x62d9, 0x6319, 0x2102, 0x2104, 0x2943,
	0xd641, 0xe50e, 0xe4cc, 0xe50d, 0x104c, 0xe54f, 0xd5cf, 0x2141, 0xdd80, 0xe50b, 0xdccd, 0xcc0b,
	0xcc09, 0xffde, 0xf7df, 0x39c6, 0xe50f, 0x2906, 0x2106, 0xcc08, 0xc3c7, 0xc3c8, 0xc386, 0xcc0a,
	0x9801, 0x9903, 0x9842, 0x98c3, 0x98c4, 0x8881, 0xaac0, 0xe616, 0xddd5, 0xa2c1, 0x12c1, 0xa44f,
	0xe615, 0x1a81, 0x1ac3, 0x2140, 0x0940, 0x0101, 0x00c0, 0xde9a, 0xcc89, 0x9081, 0x9080, 0x90c2,
	0x9043, 0x9144, 0xdd0a, 0x084b, 0x108b, 0xd58e, 0xc488, 0xccc7, 0xcdd2, 0xd5d2, 0xcd0d, 0xcd0c,
	0xe715, 0xde11, 0xcd4c, 0xc4ca, 0xcd4d, 0xd5d0, 0xcdd0, 0xcd0b, 0xcd0e, 0xdd8e, 0xde51, 0xd611,
	0xbc48, 0xd58f, 0xccc9, 0xddce, 0xde53, 0xde52, 0xc489, 0xccca, 0xd54d, 0xde10, 0xe693, 0xcd0a,
	0xddcf, 0xdd8b, 0xddcc, 0xdd8c, 0xde0f, 0xde50, 0xcd8e, 0xde94, 0x0000, 0xddcb, 0x7bcd, 0xf5c0,
	0x3a0b, 0x2988, 0x42cf, 0x3a4b, 0x2105, 0x80c3, 0xe0c3, 0x3a0c, 0x2146, 0x1904, 0x0083, 0x10c3,
	0xfd96, 0x31c9, 0x2145, 0x0001, 0x0882, 0x428d, 0x2945, 0x18c3, 0x424d, 0x18c4, 0x2987, 0x31ca,
	0x3a0a, 0x2986, 0x0883, 0x2946, 0x2947, 0x428e, 0x31c8, 0x31c7, 0x4acc, 0x4acd, 0x4acf, 0x638f,
	0x3a09, 0x3a4c, 0x424c, 0x428c, 0x7c54, 0x4a8d, 0x4b0d, 0x5b50, 0x428b, 0x424b,
}; /* 154 values */

static const uint8_t blackprobe_data[256] = {
	255, 255, 255, 255, 255, 255, 108, 109, 108, 110, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	111, 112, 112, 112, 113, 114, 113, 115, 255, 255, 255, 255, 255, 255, 255, 116, 117, 118, 119, 10,
	114, 120, 114, 121, 255, 255, 255, 255, 255, 255, 255, 122, 10, 123, 104, 124, 113, 114, 113, 125,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 126, 127, 127, 10, 112, 255, 255, 255, 255, 255, 255,
	255, 255, 128, 112, 117, 119, 129, 129, 117, 129, 117, 130, 255, 255, 255, 255, 255, 255, 255, 116,
	112, 117, 117, 10, 117, 10, 112, 131, 255, 255, 255, 255, 255, 255, 255, 132, 133, 112, 119, 134,
	124, 127, 122, 255, 255, 255, 255, 255, 255, 255, 255, 255, 109, 255, 122, 122, 255, 135, 130, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 108, 255, 136, 255, 255, 133, 121, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 137, 255, 138, 116, 255, 139, 111, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 121, 140, 136, 255, 139, 141, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 142, 143, 121,
	255, 144, 138, 145, 255, 255, 255, 255, 255, 255, 255, 255, 146, 255, 140, 146, 255, 255, 147, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 148, 255, 138, 141, 255, 255, 149, 147, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 150, 151, 255, 255, 255, 152, 153, 255, 255, 255,
}; /* 256 values */

static const struct asset2 blackprobe = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) blackprobe_colormap,
	.pixel = (const unsigned char *) blackprobe_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/blackprobe.png Tue Apr 16 09:12:22 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/robot3.png Tue Apr 16 13:42:14 2024. */
static const uint16_t robot3_colormap[14] = {
	0xffdf, 0xb452, 0xef5c, 0x8410, 0x7c0f, 0xc617, 0xb596, 0xc30c, 0xc187, 0xc2cb, 0xff9e, 0xce59,
	0xc104, 0xc000,
}; /* 14 values */

static const uint8_t robot3_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0,
	0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 1, 2, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 3, 3, 3, 4, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 0, 0, 0, 5, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	0, 0, 0, 0, 0, 5, 0, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0, 6, 7, 8,
	0, 9, 5, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 10, 10, 0, 11, 255, 0,
	255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 12, 13, 0, 13, 255, 0, 255, 255, 255, 255,
	255, 255, 255, 255, 0, 255, 0, 0, 0, 5, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255,
	0, 255, 0, 0, 0, 11, 255, 0, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 5, 11,
	11, 5, 255, 0, 255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 3, 3, 255, 255, 0,
	0, 255, 255, 255, 255, 255, 255, 0, 0, 255, 0, 0, 0, 0, 255, 0, 0, 255, 255, 255,
	255, 255, 255, 255, 3, 255, 255, 255, 255, 255, 255, 3, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 robot3 = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) robot3_colormap,
	.pixel = (const unsigned char *) robot3_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/robot3.png Tue Apr 16 13:42:14 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/skavo.png Tue Apr 16 15:36:05 2024. */
static const uint16_t skavo_colormap[14] = {
	0x72c9, 0x6a87, 0x0800, 0x0000, 0x2081, 0x868e, 0x75cc, 0x3102, 0x49c4, 0x6a86, 0x5a05, 0x6ac8,
	0x2104, 0x6246,
}; /* 14 values */

static const uint8_t skavo_data[256] = {
	255, 255, 255, 255, 255, 0, 1, 1, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 1, 2, 3, 2, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 4, 5, 3,
	6, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 1, 7, 3, 2, 2, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 1, 1, 8, 2, 3, 1, 0, 255, 255, 255, 255, 255, 255,
	255, 255, 1, 9, 10, 9, 9, 4, 1, 1, 1, 255, 255, 255, 255, 255, 255, 255, 1, 9,
	10, 9, 1, 1, 1, 10, 1, 1, 11, 2, 12, 255, 255, 255, 255, 1, 1, 9, 7, 1,
	1, 10, 255, 1, 1, 13, 255, 255, 255, 255, 255, 255, 11, 8, 3, 1, 1, 11, 255, 1,
	1, 255, 255, 255, 255, 255, 255, 255, 1, 1, 1, 1, 1, 1, 255, 255, 0, 255, 255, 255,
	255, 255, 255, 11, 1, 10, 1, 1, 1, 9, 1, 255, 255, 255, 255, 255, 255, 255, 255, 1,
	1, 10, 1, 1, 9, 10, 1, 0, 255, 255, 255, 255, 255, 255, 11, 1, 13, 9, 1, 1,
	1, 9, 9, 1, 255, 255, 255, 255, 255, 255, 1, 1, 13, 9, 1, 1, 1, 8, 8, 1,
	11, 255, 255, 255, 255, 255, 1, 13, 13, 1, 1, 1, 1, 1, 8, 1, 1, 255, 255, 255,
	255, 255, 255, 11, 9, 1, 1, 1, 1, 11, 0, 0, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 skavo = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) skavo_colormap,
	.pixel = (const unsigned char *) skavo_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/skavo.png Tue Apr 16 15:36:05 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/zunaro.png Tue Apr 16 15:36:05 2024. */
static const uint16_t zunaro_colormap[28] = {
	0x72c9, 0x6a87, 0x0800, 0x0000, 0x2081, 0x868e, 0x75cc, 0x3102, 0x49c4, 0x6a86, 0x5a05, 0x6ac8,
	0x2104, 0x6246, 0xffdf, 0xf7de, 0xce58, 0x4904, 0xdf1b, 0xffde, 0xef5d, 0xbdd7, 0x9cd3, 0xd659,
	0xe71c, 0xf79d, 0xb041, 0xa986,
}; /* 28 values */

static const uint8_t zunaro_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 14,
	14, 15, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 16, 15, 17, 14, 17, 14, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 18, 14, 14, 14, 19, 14, 14, 14, 20, 255, 255, 255,
	255, 255, 255, 15, 14, 14, 14, 21, 22, 23, 14, 19, 14, 14, 20, 255, 255, 255, 19, 19,
	15, 14, 19, 14, 14, 14, 19, 255, 255, 14, 19, 255, 255, 24, 14, 255, 255, 255, 14, 19,
	14, 14, 25, 255, 255, 19, 15, 255, 255, 14, 25, 255, 255, 255, 255, 24, 20, 18, 255, 255,
	255, 18, 24, 255, 255, 18, 19, 255, 255, 255, 255, 14, 19, 19, 255, 255, 26, 255, 26, 255,
	255, 26, 255, 27, 255, 255, 14, 14, 14, 14, 14, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 19, 14, 19, 24, 18, 14, 25, 255, 255, 255, 255, 255, 255, 255, 255, 255, 14, 14, 18,
	255, 255, 18, 14, 255, 255, 255, 255, 255, 255, 255, 255, 255, 14, 20, 255, 255, 255, 255, 14,
	19, 255, 255, 255, 255, 255, 255, 255, 255, 14, 24, 255, 255, 255, 255, 18, 14, 25, 255, 255,
	255, 255, 255, 255, 14, 14, 18, 255, 255, 255, 255, 255, 24, 25, 21, 255,
}; /* 256 values */

static const struct asset2 zunaro = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) zunaro_colormap,
	.pixel = (const unsigned char *) zunaro_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/zunaro.png Tue Apr 16 15:36:05 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/hargon.png Tue Apr 16 15:36:05 2024. */
static const uint16_t hargon_colormap[42] = {
	0x72c9, 0x6a87, 0x0800, 0x0000, 0x2081, 0x868e, 0x75cc, 0x3102, 0x49c4, 0x6a86, 0x5a05, 0x6ac8,
	0x2104, 0x6246, 0xffdf, 0xf7de, 0xce58, 0x4904, 0xdf1b, 0xffde, 0xef5d, 0xbdd7, 0x9cd3, 0xd659,
	0xe71c, 0xf79d, 0xb041, 0xa986, 0x7285, 0x9b03, 0x82c5, 0x28c1, 0x6245, 0x72c7, 0xbd12, 0x4142,
	0x6a03, 0x61c0, 0x72c8, 0x7b8c, 0x4140, 0x5a07,
}; /* 42 values */

static const uint8_t hargon_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 28, 29, 9, 29, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 30, 31, 32,
	28, 31, 33, 255, 255, 255, 255, 255, 255, 255, 255, 255, 28, 9, 28, 9, 28, 28, 28, 29,
	29, 255, 255, 255, 255, 255, 29, 29, 28, 32, 34, 31, 31, 34, 32, 28, 28, 29, 30, 29,
	29, 28, 9, 9, 9, 31, 31, 31, 31, 31, 35, 32, 255, 9, 28, 28, 28, 28, 255, 255,
	255, 33, 32, 31, 31, 28, 33, 255, 255, 255, 255, 28, 28, 255, 255, 255, 255, 255, 36, 37,
	37, 37, 255, 255, 255, 255, 255, 28, 28, 38, 255, 255, 255, 9, 28, 36, 36, 36, 255, 255,
	255, 255, 28, 28, 32, 37, 255, 255, 28, 28, 28, 36, 28, 28, 9, 255, 255, 255, 37, 36,
	255, 255, 255, 9, 28, 28, 37, 39, 36, 28, 28, 255, 255, 255, 255, 255, 255, 255, 255, 36,
	28, 37, 255, 255, 255, 36, 28, 255, 255, 255, 255, 255, 255, 255, 255, 39, 28, 255, 255, 255,
	255, 37, 33, 255, 255, 255, 255, 255, 255, 255, 255, 255, 28, 255, 255, 255, 255, 36, 28, 33,
	255, 255, 255, 255, 255, 255, 255, 28, 28, 255, 255, 255, 255, 255, 9, 28, 28, 255, 255, 255,
	255, 255, 40, 28, 40, 255, 255, 255, 255, 255, 255, 40, 9, 41, 255, 255,
}; /* 256 values */

static const struct asset2 hargon = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) hargon_colormap,
	.pixel = (const unsigned char *) hargon_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/hargon.png Tue Apr 16 15:36:05 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/tarcon.png Tue Apr 16 15:36:05 2024. */
static const uint16_t tarcon_colormap[57] = {
	0x72c9, 0x6a87, 0x0800, 0x0000, 0x2081, 0x868e, 0x75cc, 0x3102, 0x49c4, 0x6a86, 0x5a05, 0x6ac8,
	0x2104, 0x6246, 0xffdf, 0xf7de, 0xce58, 0x4904, 0xdf1b, 0xffde, 0xef5d, 0xbdd7, 0x9cd3, 0xd659,
	0xe71c, 0xf79d, 0xb041, 0xa986, 0x7285, 0x9b03, 0x82c5, 0x28c1, 0x6245, 0x72c7, 0xbd12, 0x4142,
	0x6a03, 0x61c0, 0x72c8, 0x7b8c, 0x4140, 0x5a07, 0xd283, 0xdb47, 0xd284, 0xdbc9, 0xca85, 0x4901,
	0xaac8, 0xdc0c, 0x91c2, 0x89c2, 0x7982, 0x9a44, 0x8247, 0xb2c7, 0xc2c6,
}; /* 57 values */

static const uint8_t tarcon_data[256] = {
	255, 255, 42, 255, 43, 255, 255, 255, 255, 255, 255, 255, 255, 43, 42, 255, 255, 42, 255, 255,
	44, 255, 255, 255, 255, 255, 255, 42, 255, 255, 43, 42, 42, 255, 255, 43, 42, 255, 255, 255,
	255, 255, 255, 42, 255, 255, 45, 42, 44, 255, 45, 42, 46, 255, 255, 255, 255, 255, 255, 42,
	45, 255, 43, 42, 44, 45, 42, 42, 255, 47, 255, 255, 255, 255, 255, 255, 42, 42, 42, 42,
	42, 42, 42, 42, 255, 42, 255, 255, 255, 47, 255, 255, 255, 44, 44, 255, 255, 42, 255, 255,
	255, 42, 255, 255, 42, 255, 255, 255, 42, 48, 255, 255, 255, 255, 44, 255, 255, 42, 42, 42,
	42, 42, 255, 42, 44, 255, 255, 255, 255, 255, 46, 49, 46, 42, 42, 42, 42, 42, 42, 49,
	48, 255, 255, 255, 255, 255, 255, 50, 51, 44, 52, 53, 50, 52, 54, 51, 52, 53, 54, 255,
	255, 255, 55, 48, 255, 49, 49, 49, 49, 49, 49, 49, 255, 255, 42, 255, 255, 44, 255, 42,
	255, 46, 255, 255, 255, 255, 46, 255, 56, 255, 255, 42, 42, 255, 255, 42, 255, 56, 255, 255,
	255, 255, 48, 255, 42, 255, 255, 42, 42, 255, 255, 44, 255, 255, 255, 255, 255, 255, 255, 255,
	42, 255, 255, 56, 255, 55, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 42, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 tarcon = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) tarcon_colormap,
	.pixel = (const unsigned char *) tarcon_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/tarcon.png Tue Apr 16 15:36:05 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/rovdan.png Tue Apr 16 15:36:05 2024. */
static const uint16_t rovdan_colormap[71] = {
	0x72c9, 0x6a87, 0x0800, 0x0000, 0x2081, 0x868e, 0x75cc, 0x3102, 0x49c4, 0x6a86, 0x5a05, 0x6ac8,
	0x2104, 0x6246, 0xffdf, 0xf7de, 0xce58, 0x4904, 0xdf1b, 0xffde, 0xef5d, 0xbdd7, 0x9cd3, 0xd659,
	0xe71c, 0xf79d, 0xb041, 0xa986, 0x7285, 0x9b03, 0x82c5, 0x28c1, 0x6245, 0x72c7, 0xbd12, 0x4142,
	0x6a03, 0x61c0, 0x72c8, 0x7b8c, 0x4140, 0x5a07, 0xd283, 0xdb47, 0xd284, 0xdbc9, 0xca85, 0x4901,
	0xaac8, 0xdc0c, 0x91c2, 0x89c2, 0x7982, 0x9a44, 0x8247, 0xb2c7, 0xc2c6, 0xca02, 0xa204, 0xa1c2,
	0x9a05, 0xc202, 0x7984, 0x7941, 0xc203, 0x8141, 0x8982, 0x8142, 0x9206, 0x5102, 0x89c3,
}; /* 71 values */

static const uint8_t rovdan_data[256] = {
	255, 255, 255, 57, 58, 255, 255, 255, 255, 255, 255, 58, 57, 255, 255, 255, 255, 255, 255, 57,
	59, 255, 255, 255, 255, 255, 255, 60, 57, 255, 255, 255, 255, 255, 61, 62, 59, 255, 255, 255,
	255, 255, 255, 58, 63, 61, 255, 255, 255, 255, 57, 63, 59, 255, 255, 255, 255, 255, 255, 60,
	63, 57, 255, 255, 255, 61, 63, 63, 59, 255, 255, 255, 255, 255, 255, 60, 63, 63, 64, 255,
	255, 57, 65, 62, 59, 57, 255, 66, 66, 255, 64, 60, 63, 65, 61, 255, 61, 67, 63, 60,
	60, 59, 255, 66, 66, 255, 60, 68, 68, 63, 66, 57, 57, 63, 63, 60, 255, 255, 66, 66,
	66, 59, 255, 255, 68, 63, 67, 57, 57, 63, 59, 255, 255, 66, 255, 67, 67, 255, 66, 255,
	255, 59, 63, 61, 64, 63, 59, 255, 255, 66, 255, 63, 63, 255, 66, 255, 255, 60, 63, 61,
	57, 63, 60, 255, 255, 69, 255, 67, 67, 255, 69, 255, 255, 60, 63, 64, 255, 60, 255, 255,
	255, 255, 66, 66, 66, 66, 255, 255, 255, 255, 59, 255, 255, 68, 255, 255, 255, 255, 70, 255,
	255, 66, 255, 255, 255, 255, 59, 255, 255, 60, 255, 255, 255, 255, 70, 255, 255, 66, 255, 255,
	255, 255, 59, 255, 255, 68, 255, 255, 255, 255, 66, 255, 255, 66, 255, 255, 255, 255, 68, 255,
	255, 255, 255, 255, 255, 255, 69, 255, 255, 69, 255, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 rovdan = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) rovdan_colormap,
	.pixel = (const unsigned char *) rovdan_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/rovdan.png Tue Apr 16 15:36:05 2024 */
/* Begin code generated by png-to-badge-asset from badgey_assets/byrstran.png Tue Apr 16 15:36:05 2024. */
static const uint16_t byrstran_colormap[85] = {
	0x72c9, 0x6a87, 0x0800, 0x0000, 0x2081, 0x868e, 0x75cc, 0x3102, 0x49c4, 0x6a86, 0x5a05, 0x6ac8,
	0x2104, 0x6246, 0xffdf, 0xf7de, 0xce58, 0x4904, 0xdf1b, 0xffde, 0xef5d, 0xbdd7, 0x9cd3, 0xd659,
	0xe71c, 0xf79d, 0xb041, 0xa986, 0x7285, 0x9b03, 0x82c5, 0x28c1, 0x6245, 0x72c7, 0xbd12, 0x4142,
	0x6a03, 0x61c0, 0x72c8, 0x7b8c, 0x4140, 0x5a07, 0xd283, 0xdb47, 0xd284, 0xdbc9, 0xca85, 0x4901,
	0xaac8, 0xdc0c, 0x91c2, 0x89c2, 0x7982, 0x9a44, 0x8247, 0xb2c7, 0xc2c6, 0xca02, 0xa204, 0xa1c2,
	0x9a05, 0xc202, 0x7984, 0x7941, 0xc203, 0x8141, 0x8982, 0x8142, 0x9206, 0x5102, 0x89c3, 0xfc82,
	0xfbc4, 0xec84, 0xebc6, 0xed47, 0xd487, 0xd3c7, 0xf58a, 0xe5c9, 0xf60c, 0xeed0, 0xee4e, 0xd58d,
	0xee8f,
}; /* 85 values */

static const uint8_t byrstran_data[256] = {
	255, 71, 71, 255, 72, 255, 255, 255, 255, 255, 255, 255, 255, 71, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 73, 255, 255, 71, 255, 74, 255, 255, 255, 255, 71, 255, 255, 255, 71, 255, 255,
	255, 255, 255, 255, 255, 255, 71, 255, 255, 255, 255, 255, 255, 71, 75, 255, 76, 255, 77, 255,
	74, 255, 255, 255, 255, 255, 71, 72, 255, 255, 255, 71, 78, 43, 255, 255, 255, 43, 255, 255,
	43, 76, 255, 255, 71, 255, 79, 75, 79, 43, 255, 255, 255, 255, 77, 255, 71, 255, 255, 255,
	71, 255, 75, 80, 78, 255, 255, 71, 255, 71, 255, 255, 76, 73, 255, 76, 75, 255, 81, 71,
	82, 83, 255, 255, 74, 72, 255, 72, 255, 78, 255, 84, 81, 81, 78, 84, 80, 83, 81, 81,
	78, 255, 255, 255, 255, 84, 84, 83, 255, 255, 81, 78, 80, 255, 255, 255, 84, 78, 255, 255,
	84, 84, 83, 255, 71, 255, 81, 84, 81, 81, 255, 71, 255, 81, 78, 72, 255, 255, 255, 255,
	81, 81, 84, 81, 84, 81, 78, 83, 255, 255, 84, 82, 255, 255, 255, 83, 84, 83, 255, 255,
	255, 255, 81, 80, 255, 255, 255, 255, 255, 255, 71, 81, 80, 255, 255, 255, 255, 255, 84, 80,
	255, 255, 255, 255, 255, 255, 82, 82, 255, 255, 255, 255, 255, 255, 84, 78, 71, 255, 255, 255,
	255, 255, 81, 81, 255, 255, 255, 255, 255, 255, 84, 81, 81, 255, 255, 255,
}; /* 256 values */

static const struct asset2 byrstran = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) byrstran_colormap,
	.pixel = (const unsigned char *) byrstran_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/byrstran.png Tue Apr 16 15:36:05 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/ship.png Mon Apr 22 15:50:48 2024. */
static const uint16_t ship_colormap[9] = {
	0xa44e, 0xad9e, 0xffdf, 0xad9c, 0xf79e, 0x60c0, 0x630b, 0x2800, 0x0000,
}; /* 9 values */

static const uint8_t ship_data[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 0, 255, 255, 1, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 0, 255, 1, 2, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 3, 0, 1,
	2, 2, 255, 255, 255, 255, 255, 255, 255, 255, 0, 255, 3, 2, 0, 2, 2, 2, 2, 255,
	255, 255, 255, 255, 255, 1, 0, 1, 2, 2, 0, 2, 2, 2, 2, 255, 255, 255, 255, 255,
	255, 2, 0, 4, 2, 4, 0, 2, 2, 2, 2, 255, 255, 255, 255, 255, 2, 2, 0, 2,
	2, 2, 0, 2, 2, 2, 2, 255, 255, 255, 255, 2, 2, 2, 0, 2, 2, 2, 0, 2,
	2, 2, 2, 255, 255, 255, 1, 2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 255,
	255, 255, 2, 2, 2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 2, 255, 2, 2, 2,
	2, 2, 0, 2, 2, 2, 0, 2, 2, 2, 2, 2, 2, 2, 255, 255, 255, 255, 5, 255,
	255, 255, 5, 255, 255, 255, 255, 255, 6, 6, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8,
	8, 8, 8, 8, 255, 255, 255, 255, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 255,
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255,
}; /* 256 values */

static const struct asset2 ship_icon = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 16,
	.y = 16,
	.colormap = (const uint16_t *) ship_colormap,
	.pixel = (const unsigned char *) ship_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/ship.png Mon Apr 22 15:50:48 2024 */

/* Begin code generated by png-to-badge-asset from badgey_assets/small-fireball.png Sat Apr 27 08:46:04 2024. */
static const uint16_t small_fireball_colormap[4] = {
	0xc981, 0xf2c5, 0xec10, 0xffdf,
}; /* 4 values */

static const uint8_t small_fireball_data[64] = {
	255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 1, 2,
	2, 1, 255, 255, 255, 0, 2, 3, 3, 2, 0, 255, 255, 0, 2, 3, 3, 2, 0, 255,
	255, 255, 1, 2, 2, 1, 255, 255, 255, 255, 255, 0, 0, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255,
}; /* 64 values */

static const struct asset2 small_fireball = {
	.type = PICTURE8BIT,
	.seqNum = 1,
	.x = 8,
	.y = 8,
	.colormap = (const uint16_t *) small_fireball_colormap,
	.pixel = (const unsigned char *) small_fireball_data,
};
/* End of code generated by png-to-badge-asset from badgey_assets/small-fireball.png Sat Apr 27 08:46:04 2024 */

static const struct point upladder[] = {
	{ -51, -41 },
	{ 47, -41 },
	{ 40, -28 },
	{ -41, -29 },
	{ -51, -41 },
	{ -128, -128 },
	{ -25, -39 },
	{ -24, 24 },
	{ -20, 23 },
	{ -20, -40 },
	{ -128, -128 },
	{ 24, -41 },
	{ 24, 23 },
	{ 19, 23 },
	{ 19, -41 },
	{ -128, -128 },
	{ -20, 11 },
	{ 19, 12 },
	{ -128, -128 },
	{ 18, 5 },
	{ -20, 5 },
	{ -128, -128 },
	{ -20, -18 },
	{ 18, -17 },
	{ -128, -128 },
	{ 18, -21 },
	{ -20, -23 },
	{ -128, -128 },
	{ -42, -30 },
	{ -42, -41 },
	{ -128, -128 },
	{ 40, -29 },
	{ 40, -41 },
};

enum iconid {
	ICON_CITIZEN1 = 0,
	ICON_CITIZEN2,
	ICON_CITIZEN3,
	ICON_CITIZEN4,
	ICON_CITIZEN5,
	ICON_CITIZEN6,
	ICON_GUARD,
	ICON_GOLDROBOT,
	ICON_BLACKPROBE,
	ICON_ROBOT3,
	ICON_BYRSTRAN,
	ICON_HARGON,
	ICON_ROVDAN,
	ICON_SKAVO,
	ICON_TARCON,
	ICON_ZUNARO,
};

static const struct asset2 *creature_icon[] = {
	&citizen1,
	&citizen2,
	&citizen3,
	&citizen4,
	&citizen5,
	&citizen6,
	&guard,
	&goldrobot,
	&blackprobe,
	&robot3,
	&byrstran,
	&hargon,
	&rovdan,
	&skavo,
	&tarcon,
	&zunaro,
};

/* creature types */
#define CREATURE_TYPE_CITIZEN 0
#define CREATURE_TYPE_GUARD 1
#define CREATURE_TYPE_ROBOT1 2
#define CREATURE_TYPE_ROBOT2 3
#define CREATURE_TYPE_ROBOT3 4
#define CREATURE_TYPE_BYRSTRAN 5
#define CREATURE_TYPE_HARGON 6
#define CREATURE_TYPE_ROVDAN 7
#define CREATURE_TYPE_SKAVO 8
#define CREATURE_TYPE_TARCON 9
#define CREATURE_TYPE_ZUNARO 10

#define FIRST_MONSTER CREATURE_TYPE_BYRSTRAN
#define LAST_MONSTER CREATURE_TYPE_ZUNARO

/* Shop types */
#define SHOP_NONE 255
#define SHOP_INN 0
#define SHOP_PUB 1
#define SHOP_ARMOURY 2
#define SHOP_WEAPONS 3
#define SHOP_HACKERSPACE 4
#define SHOP_TEMPLE 5
#define SHOP_SPECIALTY 6 /* not a real shop type, used for specialty items */
#define NUMSHOPS 6

#define ITEM_TYPE_INTANGIBLE 0
#define ITEM_TYPE_SUSTENANCE 1
#define ITEM_TYPE_ARMOR 2
#define ITEM_TYPE_RANGED_WEAPON 3
#define ITEM_TYPE_MELEE_WEAPON 4
#define ITEM_TYPE_SOFTWARE 5
#define ITEM_TYPE_USELESS 6

const struct shop_item {
	char *name;
	int price;
	unsigned char item_type;
	unsigned char shop_type;
} shop_item[] = {
	/* INN */
	{ "SAVE GAME", 0, ITEM_TYPE_INTANGIBLE, SHOP_INN },
	{ "RESTORE GAME", 0, ITEM_TYPE_INTANGIBLE, SHOP_INN },

	/* PUB */
	{ "FOOD", 10, ITEM_TYPE_SUSTENANCE, SHOP_PUB },
	{ "DRINK", 5, ITEM_TYPE_SUSTENANCE, SHOP_PUB },

	/* ARMOURY */
	{ "LIGHT ARMOR", 40, ITEM_TYPE_ARMOR, SHOP_ARMOURY },
	{ "HEAVY ARMOR", 80, ITEM_TYPE_ARMOR, SHOP_ARMOURY },

	/* WEAPONS */
	{ "LASER CUTLASS", 10, ITEM_TYPE_MELEE_WEAPON, SHOP_WEAPONS, },
	{ "BLASTER", 20, ITEM_TYPE_RANGED_WEAPON, SHOP_WEAPONS, },

	/* HACKERSPACE */
	{ "PLA DOODAD", 1, ITEM_TYPE_USELESS, SHOP_HACKERSPACE, },
	{ "T-SHIRT", 25, ITEM_TYPE_USELESS, SHOP_HACKERSPACE, },

	/* TEMPLE */
	{ "SACRAMENT", 100, ITEM_TYPE_USELESS, SHOP_TEMPLE, },
	{ "BLESSING", 200, ITEM_TYPE_USELESS, SHOP_TEMPLE, },

	/* specialty items */
	{ "blah blah", 0, ITEM_TYPE_RANGED_WEAPON, SHOP_SPECIALTY },
};

#define MAX_ITEMS_PER_SHOP 8
struct shop {
	int item[MAX_ITEMS_PER_SHOP];
	int nitems;
} shop[NUMSHOPS];

const char *proprietor[] = { /* indexed by shop type */
	"  INNKEEP",
	"  BARKEEP",
	"  ARMOURER",
	"  ENGINEER",
	"  HACKER",
	"  GOOROO",
};

const char *shopname[] = { /* indexed by shop type */
	"INN",
	"PUB",
	"ARMOURER",
	"WEAPONS",
	"HACKERSPACE",
	"TEMPLE",
};

static const char *creature_info[] = {
	"GRUNTS", /* 0 - default monster "info" */
	"HOWDY!", /* 1 - 5 - generic human responses */
	"HELLO!",
	"NICE DAY!",
	"WHAT'S UP!",
	"HI THERE!",
};

struct creature;
static void generic_move(struct creature *self);
static void generic_monster_move(struct creature *self);
static void citizen_move(struct creature *self);

/* Data particular to creature types, but common to all instances of the type
 * but distinct from other types
 */
static const struct creature_generic_data {
	int min_hp, max_hp;
	void (*move)(struct creature *self);
	union {
		struct citizen_generic {
			uint8_t min_icon;
			uint8_t max_icon;
		} citizen;
		struct guard_generic {
			uint8_t icon;
		} guard;
		struct robot1_generic {
			uint8_t icon;
		} robot1;
		struct robot2_generic {
			uint8_t icon;
		} robot2;
		struct robot3_generic {
			uint8_t icon;
		} robot3;
		struct byrstran_generic {
			uint8_t icon;
		} byrstran;
		struct hargon_generic {
			uint8_t icon;
		} hargon;
		struct rovdan_generic {
			uint8_t icon;
		} rovdan;
		struct skavo_generic {
			uint8_t icon;
		} skavo;
		struct tarcon_generic {
			uint8_t icon;
		} tarcon;
		struct zunaro_generic {
			uint8_t icon;
		} zunaro;
	};
} creature_generic_data[] = {
	{
		.citizen = {
			.min_icon = ICON_CITIZEN1,
			.max_icon = ICON_CITIZEN6,
		},
		.min_hp = 50,
		.max_hp = 100,
		.move = citizen_move,
	},
	{
		.guard = {
				.icon = ICON_GUARD,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_move,
	},
	{
		.robot1 = {
			.icon = ICON_GOLDROBOT,
		 },
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_move,
	},
	{
		.robot2 = {
			.icon = ICON_BLACKPROBE,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_move,
	},
	{
		.robot3 = {
			.icon = ICON_ROBOT3,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_move,
	},
	{
		.byrstran = {
			.icon = ICON_BYRSTRAN,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_monster_move,
	},
	{
		.hargon = {
			.icon = ICON_HARGON,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_monster_move,
	},
	{
		.rovdan = {
			.icon = ICON_ROVDAN,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_monster_move,
	},
	{
		.skavo = {
			.icon = ICON_SKAVO,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_monster_move,
	},
	{
		.tarcon = {
			.icon = ICON_TARCON,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_monster_move,
	},
	{
		.zunaro = {
			.icon = ICON_ZUNARO,
		},
		.min_hp = 100,
		.max_hp = 200,
		.move = generic_monster_move,
	},
};

/* Data specific to individual creatures, unioned by type by type */
struct creature_specific_data {
	/* Nothing outside the union, if you feel like you want to put something
	 * here (something not specific to type), put it in struct creature instead.
	 */
	union {
		struct citizen_data {
			uint8_t shopkeep; /* SHOP_NONE or index into which shop */
			uint8_t icon;
		} citizen;
	};
};

struct creature {
	uint8_t type; /* indexes into creature_generic_data[] */
	uint8_t homex, homey, x, y;
	uint8_t onscreen_and_visible;
	uint8_t info;
	char name[8];
	struct creature_specific_data csd;
	uint8_t no_in_party;
	int hit_points;
};

#define MAX_CREATURES 25
#define NUM_MONSTERS 20
#define MAX_COMBAT_CREATURES 5
static struct creature planet_creature[MAX_CREATURES];
static struct creature town_creature[MAX_CREATURES];
static struct creature space_creature[MAX_CREATURES];
static struct creature combat_creature[MAX_COMBAT_CREATURES];
static int overworld_combat_creature = -1;
static int nplanet_creatures = 0;
static int ntown_creatures = 0;
static int nspace_creatures = 0;
static int ncombat_creatures = 0;
static struct creature *creature = space_creature; /* points to one of planet_, town_ or space_ creature[] */
static int *ncreatures = &nspace_creatures; /* points to one of nplanet_, ntown_ or nspace_ creatures */
#define NUM_SHIPS 2
static struct ship {
	int x, y;
	int dir; 
} ship[NUM_SHIPS];
static int nships = 0;

static struct player {
	struct badgey_world const *world; /* current world */
	int x, y; /* coords in current world */
	int world_level;
	struct badgey_world const *old_world[5];
	int wx[5], wy[5]; /* coords at each level */
	int in_town;
	int in_cave;
	int dir;
	int moving;
	unsigned char in_shop;
	int money;
	int hp;
	unsigned char carrying[ARRAY_SIZE(shop_item)];
	unsigned char equipped_weapon, equipped_armor;
	unsigned char cbx, cby;
#define EQUIPPED_NONE 255
} player = {
	.world = &space,
	.x = 32,
	.y = 32,
	.world_level = 0,
	.old_world = { 0 },
	.wx = { 0 },
	.wy = { 0 },
	.in_town = 0,
	.in_cave = 0,
	.dir = 0, /* 0 = N, 1 = E, 2 = S, 3 = W */
	.moving = 0,
	.in_shop = SHOP_NONE,
	.money = 500,
	.carrying = { 0 },
	.hp = 100,
	.equipped_weapon = EQUIPPED_NONE,
	.equipped_armor = EQUIPPED_NONE,
};

/* Program states.  Initial state is BADGEY_INIT */
enum badgey_state_t {
	BADGEY_INIT,
	BADGEY_CONTINUE,
	BADGEY_PLANET_MENU,
	BADGEY_CAVE_MENU,
	BADGEY_TOWN_MENU,
	BADGEY_RUN,
	BADGEY_ENTER_TOWN_OR_CAVE,
	BADGEY_TALK_TO_SHOPKEEPER,
	BADGEY_TALK_TO_CITIZEN,
	BADGEY_STATUS_MESSAGE,
	BADGEY_STATS,
	BADGEY_EQUIP_WEAPON,
	BADGEY_EQUIP_ARMOR,
	BADGEY_EXIT_CONFIRM,
	BADGEY_COMBAT,
	BADGEY_EXIT,
};

static enum badgey_state_t badgey_state = BADGEY_INIT;
static enum badgey_state_t previous_badgey_state = BADGEY_RUN;
static enum badgey_state_t badgey_unconfirm_state = BADGEY_RUN;

static int screen_changed = 0;

#define MAX_MISSILES 50
static struct missile {
	int x, y, vx, vy; /* 24.8 fixed point */
	int alive;
} missile[MAX_MISSILES];
static int nmissiles = 0;

static void add_missile(int x, int y, int vx, int vy, int lifetime)
{
	if (nmissiles >= MAX_MISSILES)
		return;
	missile[nmissiles].x = x;
	missile[nmissiles].y = y;
	missile[nmissiles].vx = vx;
	missile[nmissiles].vy = vy;
	missile[nmissiles].alive = lifetime;
	nmissiles++;
}

static void remove_missile(int i)
{
	if (i < 0 || i >= nmissiles)
		return;
	if (i < nmissiles - 1)
		missile[i] = missile[nmissiles - 1]; /* copy last missile on top of missile[i] */
	nmissiles--;
}

static void move_missile(int i)
{
	missile[i].x += missile[i].vx;
	missile[i].y += missile[i].vy;
	if (missile[i].alive > 0)
		missile[i].alive--;
	int sx = 8 + missile[i].x / 256 - 4;
	int sy = 8 + missile[i].y / 256 - 4;
	if (sx < 0 || sx > LCD_XSIZE || sy < 0 || sy > LCD_YSIZE)
		missile[i].alive = 0;
}

static void missile_collision_detection(int m)
{
	for (int i = 0; i < ncombat_creatures; i++) {
		int cx = 16 * combat_creature[i].x + 8 + 8;
		int cy = 16 * combat_creature[i].x + 8 + 8;
		int mx = 8 + missile[m].x / 256 - 4;
		int my = 8 + missile[m].y / 256 - 4;
		int dist2 = (cx - mx) * (cx - mx) + (cy - my) * (cy - my);
		if (dist2 < 8 * 8) {
			/* TODO: more sophisticated damage */
			combat_creature[i].hit_points = 0;
			missile[m].alive = 0;
			/* TODO: add explosion or something here */
		}
	}
}

static void move_combat_missiles(void)
{
	for (int i = 0; i < nmissiles; /* no increment */ ) {
		move_missile(i);
		missile_collision_detection(i);
		if (!missile[i].alive)
			remove_missile(i);
		else
			i++;
		screen_changed = 1;
	}
}

static void draw_missile(int i)
{
	int sx, sy;

	sx = 8 + missile[i].x / 256 - 4;
	sy = 8 + missile[i].y / 256 - 4;
	if (sx >= 0 && sx < LCD_XSIZE && sy >= 0 && sy < LCD_YSIZE) {
		FbMove(sx, sy);
		FbImage2(&small_fireball, 0);
	}
}

static void draw_combat_missiles(void)
{
	for (int i = 0; i < nmissiles; i++)
		draw_missile(i);
}

static void set_badgey_state(enum badgey_state_t new_state)
{
	previous_badgey_state = badgey_state;
	badgey_state = new_state;
}

static char message_to_display[255];
static char message_displayed = 0;
static int water_scroll = 0;

static void confirm_exit(void)
{
	badgey_unconfirm_state = badgey_state;
	set_badgey_state(BADGEY_EXIT_CONFIRM);
}

static void unconfirm_exit(void)
{
	set_badgey_state(badgey_unconfirm_state);
}

/* hack used for scrolling textures in badgey RPG game */
#define BUFFER( ADDR ) G_Fb.buffer[(ADDR)]
static void FbImage8bit2scrolling(const struct asset2 *asset, unsigned char seqNum, int scroll)
{
    unsigned char y, yEnd, x;
    unsigned char pixbyte, ci;
    unsigned short pixel;
    unsigned char yi;

    scroll = abs(scroll % asset->y);

    /* clip to end of LCD buffer */
    yEnd = G_Fb.pos.y + asset->y;
    if (yEnd > LCD_YSIZE) yEnd = LCD_YSIZE-1;

    for (y = G_Fb.pos.y; y < yEnd; y++) {
	yi = y - G_Fb.pos.y + scroll;
		if (yi >= asset->y)
			yi -= asset->y;
        unsigned char const *pixdata = &asset->pixel[yi * asset->x +
								seqNum * asset->x * asset->y];

        for (x = 0; x < asset->x; x++) {
            if ((x + G_Fb.pos.x) >= LCD_XSIZE) break; /* clip x */

            pixbyte = *pixdata; /* 1 pixel per byte */

            ci = pixbyte;

            if (ci != G_Fb.transIndex) { /* transparent? */
                pixel = asset->colormap[ci];
                if (G_Fb.transMask > 0)
                    BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = (BUFFER(x + G_Fb.pos.x) & (~G_Fb.transMask)) | (pixel & G_Fb.transMask);
                else
                    BUFFER(y * LCD_XSIZE + x + G_Fb.pos.x) = pixel;
            }
            pixdata++;
        }
    }
    G_Fb.changed = 1;
}

static void badgey_status_message(void)
{
	if (!message_displayed) {
		FbMove(0, 0);
		FbColor(WHITE);
		FbBackgroundColor(BLACK);
		FbWriteString(message_to_display);
		FbSwapBuffers();
		message_displayed = 1;
	}

	int down_latches = button_down_latches();

	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		message_displayed = 0;
		screen_changed = 1;
		set_badgey_state(previous_badgey_state);
	}
}

static void status_message(char *message)
{
	strcpy(message_to_display, message);
	message_displayed = 0;
	set_badgey_state(BADGEY_STATUS_MESSAGE); 
}

static void set_creature_name(char *name)
{
	strcpy(name, "JACK"); /* TODO: something better */
}

static void add_shopkeeper(int x, int y, unsigned char shoptype, unsigned int *seed)
{
	int n;

	if (*ncreatures >= MAX_CREATURES) {
#ifdef TARGET_SIMULATOR
		fprintf(stderr, "Too many creatures: %s:%d\n", __FILE__, __LINE__);
#endif
		return;
	}
	n = *ncreatures;
	creature[n].type = CREATURE_TYPE_CITIZEN;
	creature[n].homex = x;
	creature[n].homey = y;
	creature[n].x = x;
	creature[n].y = y;
	creature[n].csd.citizen.shopkeep = shoptype;
	int minicon = creature_generic_data[CREATURE_TYPE_CITIZEN].citizen.min_icon;
	int maxicon = creature_generic_data[CREATURE_TYPE_CITIZEN].citizen.max_icon;
	creature[n].csd.citizen.icon = minicon + (xorshift(seed) % (maxicon - minicon));
	creature[n].info = 0;
	set_creature_name(creature[n].name);
	(*ncreatures)++;
}

static void add_robot1(unsigned char roadchar, unsigned int *seed)
{
	int x, y, n;

	if (*ncreatures >= MAX_CREATURES) {
#ifdef TARGET_SIMULATOR
		fprintf(stderr, "Too many creatures: %s:%d\n", __FILE__, __LINE__);
#endif
		return;
	}
	int count = 0;
	do {
		x = (xorshift(seed) % 50) + 7;
		y = (xorshift(seed) % 50) + 7;
		if (dynmap[windex(x, y)] == roadchar)
			break;
		count++;
		if (count > 50)
			return;
	} while (1);

	n = *ncreatures;
	creature[n].type = CREATURE_TYPE_ROBOT1;
	creature[n].homex = x;
	creature[n].homey = y;
	creature[n].x = x;
	creature[n].y = y;
	creature[n].info = 0;
	set_creature_name(creature[n].name);
	(*ncreatures)++;
}

static void add_robot3(unsigned char roadchar, unsigned int *seed)
{
	int x, y, n;

	if (*ncreatures >= MAX_CREATURES) {
#ifdef TARGET_SIMULATOR
		fprintf(stderr, "Too many creatures: %s:%d\n", __FILE__, __LINE__);
#endif
		return;
	}
	int count = 0;
	do {
		x = (xorshift(seed) % 50) + 7;
		y = (xorshift(seed) % 50) + 7;
		if (dynmap[windex(x, y)] == roadchar)
			break;
		count++;
		if (count > 50)
			return;
	} while (1);

	n = *ncreatures;
	creature[n].type = CREATURE_TYPE_ROBOT3;
	creature[n].homex = x;
	creature[n].homey = y;
	creature[n].x = x;
	creature[n].y = y;
	creature[n].info = 0;
	(*ncreatures)++;
}

static void add_guard(unsigned char roadchar, unsigned int *seed)
{
	int x, y, n;

	if (*ncreatures >= MAX_CREATURES) {
#ifdef TARGET_SIMULATOR
		fprintf(stderr, "Too many creatures: %s:%d\n", __FILE__, __LINE__);
#endif
		return;
	}
	int count = 0;
	do {
		x = (xorshift(seed) % 50) + 7;
		y = (xorshift(seed) % 50) + 7;
		if (dynmap[windex(x, y)] == roadchar)
			break;
		count++;
		if (count > 50)
			return;
	} while (1);

	n = *ncreatures;
	creature[n].type = CREATURE_TYPE_GUARD;
	creature[n].homex = x;
	creature[n].homey = y;
	creature[n].x = x;
	creature[n].y = y;
	creature[n].info = 0;
	set_creature_name(creature[n].name);
	(*ncreatures)++;
}

static void add_citizen(int roadchar, unsigned int *seed)
{
	int x, y, n;

	if (*ncreatures >= MAX_CREATURES) {
#ifdef TARGET_SIMULATOR
		fprintf(stderr, "Too many creatures: %s:%d\n", __FILE__, __LINE__);
#endif
		return;
	}

	int count = 0;
	do {
		x = (xorshift(seed) % 50) + 7;
		y = (xorshift(seed) % 50) + 7;
		if (dynmap[windex(x, y)] == roadchar)
			break;
		count++;
		if (count > 50)
			return;
	} while (1);

	n = *ncreatures;
	creature[n].type = CREATURE_TYPE_CITIZEN;
	creature[n].homex = x;
	creature[n].homey = y;
	creature[n].x = x;
	creature[n].y = y;
	creature[n].csd.citizen.shopkeep = SHOP_NONE;
	creature[n].csd.citizen.icon = ICON_CITIZEN1 + (xorshift(seed) % 6);
	creature[n].info = 0;
	set_creature_name(creature[n].name);
	(*ncreatures)++;
}

static int wrap(int v)
{
	if (v < 0)
		v += 64;
	if (v > 63)
		v -= 64;
	return v;
}

/* What character is on the map at (x + dx, y + dy), making the arithmetic wrap */
static char whats_there(const char *map, int x, int y, int dx, int dy)
{
	x = wrap(x + dx);
	y = wrap(y + dy);
	return map[y * 64 + x];
}

static void spawn_planet_initial_monsters(void);

static void badgey_init(void)
{
	FbInit();
	FbClear();
	player.x = 37;
	player.y = 32;
	player.world = &ossaria;
	player.world_level = 1;
	player.old_world[1] = &space;
	player.old_world[0] = NULL;
	player.wx[0] = 55;
	player.wy[0] = 4;
	player.in_town = 0;
	player.in_cave = 0;
	player.dir = 0;
	player.moving = 0;
	player.in_shop = 0;
	player.money = 500;
	memset(player.carrying, 0, sizeof(player.carrying));
	spawn_planet_initial_monsters();
	set_badgey_state(BADGEY_CONTINUE);
	screen_changed = 1;
}

static void badgey_continue(void)
{
	FbInit();
	FbClear();
	set_badgey_state(BADGEY_RUN);
	screen_changed = 1;
}

static void cave_check_buttons(void)
{
	int newx, newy, newdir;

	newx = player.x;
	newy = player.y;
	newdir = player.dir;

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		newdir--;
		if (newdir < 0)
			newdir += 4;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		newdir++;
		if (newdir > 3)
			newdir -= 4;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		newx += xo4[player.dir];
		newy += yo4[player.dir];
		if (newx < 0)
			newx += 64;
		if (newx > 63)
			newx -= 64;
		if (newy < 0)
			newy += 64;
		if (newy > 63)
			newy -= 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		int backdir = newdir + 2;
		if (backdir > 3)
			backdir -= 4;
		newx += xo4[backdir];
		newy += yo4[backdir];
		if (newx < 0)
			newx += 64;
		if (newx > 63)
			newx -= 64;
		if (newy < 0)
			newy += 64;
		if (newy > 63)
			newy -= 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		if (player.world->type == WORLD_TYPE_CAVE)
			set_badgey_state(BADGEY_CAVE_MENU);
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		confirm_exit();
	}
	if (dynmap[windex(newx, newy)] == '#')
		return;
	if (newx == player.x && newy == player.y && newdir == player.dir)
		return;
	player.x = newx;
	player.y = newy;
	player.dir = newdir;
	screen_changed = 1;
}

static void spawn_monster(unsigned int *seed)
{
	int x, y;

	/* Find a good place to spawn ... */
	int count = 0;
	do {
		x = xorshift(seed) % 64;
		y = xorshift(seed) % 64;
		unsigned char ch = player.world->wm[windex(x, y)];
		if (ch == '.' || ch == 'f' || ch == 'd')
			break;
		count++;
	} while (count < 50);
	if (count >= 50) /* didn't find a good place ... */
		return;

	/* spawn a monster */
	int type = (xorshift(seed) % (LAST_MONSTER - FIRST_MONSTER + 1)) + FIRST_MONSTER;

	int n = *ncreatures;
	if (n >= MAX_CREATURES) {
#if TARGET_SIMULATOR
		printf("Failed to spawn monster, too many creatures %s:%d.\n",
			__FILE__, __LINE__);
#endif
		return;
	}
	creature[n].type = type;
	creature[n].homex = x;
	creature[n].homey = y;
	creature[n].x = x;
	creature[n].y = y;
	creature[n].no_in_party = (unsigned char) ((xorshift(seed) % 4) + 1);
	set_creature_name(creature[n].name);
	(*ncreatures)++;
}

static void respawn_monster(int cr, unsigned int *seed)
{
	/* respawn the overworld creature far away so we don't immediately jump back into combat */
	do {
		int x = xorshift(seed) % 64;
		int y = xorshift(seed) % 65;
		unsigned char ch = player.world->wm[windex(x, y)];
		if (ch != '.' && ch != 'f' && ch != 'd') /* not a good place? */
			continue;
		int dist2 = (player.x - x) * (player.x - x) + (player.y - y) * (player.y - y);
		if (dist2 > 32) {
			creature[cr].x = x;
			creature[cr].y = y;
			creature[cr].homex = x;
			creature[cr].homey = y;
			creature[cr].no_in_party = (unsigned char) ((xorshift(seed) % 4) + 1);
			break;
		}
	} while (1);
}

static void spawn_ship(unsigned int *seed)
{
	int x, y;

	if (nships >= NUM_SHIPS)
		return;
	do {
		x = xorshift(seed) % 64;
		y = xorshift(seed) % 64;
	} while (player.world->wm[windex(x, y)] != 'w');
	ship[nships].x = x;
	ship[nships].y = y;
	ship[nships].dir = 1;
#if TARGET_SIMULATOR
	printf("Ship spawned at %d, %d\n", x, y);
#endif
	nships++;
}

static void spawn_planet_initial_monsters(void)
{
	ncreatures = &nplanet_creatures;
	*ncreatures = 0;
	unsigned int seed = player.world->initial_seed;
	for (int i = 0; i < NUM_MONSTERS; i++) {
		spawn_monster(&seed);
	}
}

static void spawn_planet_initial_ships(void)
{
	unsigned int seed = player.world->initial_seed;
	nships = 0;
	for (int i = 0; i < NUM_SHIPS; i++)
		spawn_ship(&seed);
}

static void enter_combat(int cr);

static void check_buttons(int tick)
{
	int newx, newy, newdir, newmoving;
	int time_for_combat = 0;
	int combat_creature = -1;

	newx = player.x;
	newy = player.y;
	newdir = player.dir;
	newmoving = player.moving;

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		if (player.moving && player.dir == 1) {
			newmoving = 0;
		} else {
			newmoving = 1;
			newdir = 3;
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		if (player.moving && player.dir == 3) {
			newmoving = 0;
		} else {
			newmoving = 1;
			newdir = 1;
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		if (player.moving && player.dir == 2) {
			newmoving = 0;
		} else {
			newmoving = 1;
			newdir = 0;
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		if (player.moving && player.dir == 0) {
			newmoving = 0;
		} else {
			newmoving = 1;
			newdir = 2;
		}
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		if (player.world->type == WORLD_TYPE_PLANET) 
			set_badgey_state(BADGEY_PLANET_MENU);
		else if (player.world->type == WORLD_TYPE_TOWN)
			set_badgey_state(BADGEY_TOWN_MENU);
		newmoving = 0;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		confirm_exit();
		newmoving = 0;
	}

	player.dir = newdir;
	player.moving = newmoving;
	if (player.moving && tick) {
		newx = wrap(player.x + xo4[newdir]);
		newy = wrap(player.y + yo4[newdir]);
	}

	int c = player.world->wm[windex(newx, newy)];
	if (player.world->type == WORLD_TYPE_SPACE) {
		/* Prevent player from driving into a star */
		if (c == '*') {
			player.moving = 0;
			return;
		}
		if (c >= '0' && c <= '9') { /* it's a planet */
			c = c - '0';
			struct badgey_world const *new_world = player.world->subworld[c];
			if (new_world != NULL) {
				player.world_level++;
				player.wx[player.world_level] = player.x;
				player.wy[player.world_level] = player.y;
				player.old_world[player.world_level] = player.world;
				player.x = new_world->landingx;
				player.y = new_world->landingy;
				player.world = new_world;
				screen_changed = 1;
				spawn_planet_initial_monsters();
				spawn_planet_initial_ships();
				player.moving = 0;
				return;
			}
		}
	} else {
		char x = player.world->wm[windex(newx, newy)];
		/* Prevent player from traversing water or mountains or signage or walls */
		if (x == 'w' || x == 'm' || x == '_' || (x >= 'A' && x <= 'Z') || x == '#') {
			player.moving = 0;
			return;
		}
	}
	if (newx != player.x || newy != player.y) {
		player.x = newx;
		player.y = newy;
		player.wx[player.world_level] = newx;
		player.wy[player.world_level] = newy;
		screen_changed = 1;

		/* Check for combat ... */
		for (int i = 0; i < *ncreatures; i++) {
			if (creature[i].x == player.x && creature[i].y == player.y) {
				time_for_combat = 1;
				combat_creature = i;
			}
		}

		if (player.in_town) {
			time_for_combat = 0; /* for now, can't fight in towns */
			/* Is player leaving town? */
			if (player.x < 3 || player.x > 60 || player.y < 3 || player.y > 60) {
				/* leave town */
				struct badgey_world const *old_world = player.old_world[player.world_level];
				if (old_world) {
					player.world = old_world;
					player.world_level--;
					player.x = player.wx[player.world_level];
					player.y = player.wy[player.world_level];
					set_badgey_state(BADGEY_RUN);
					/* global */ screen_changed = 1;
					player.in_town = 0;
					creature = &planet_creature[0];
					ncreatures = &nplanet_creatures;
					spawn_planet_initial_monsters();
					spawn_planet_initial_ships();
					player.moving = 0;
				}
			}
		}
	}

	if (time_for_combat)
		enter_combat(combat_creature);
}

/* Draw a cell of the world at screen coords (x, y) */
static void draw_cell(int x, int y, unsigned char c)
{
	FbMove(x, y);
	switch (c) {
	case '_':
	case 'A' ... 'Z':
		FbColor(WHITE);
		FbBackgroundColor(BLACK);
		FbHorizontalLine(x, y, x + 16, y);
		FbHorizontalLine(x, y + 15, x + 16, y + 15);
		FbMove(x + 4, y + 4);
		if (c != '_')
			FbCharacter(c);
		break;
	case '0' ... '9':
		if (player.world->type == WORLD_TYPE_SPACE) { /* player is in space?  it's a planet */
			FbImage2(&planet, 0);
		} else {
			/* Not in space, so ... */
			if ((c - '0') < 5)
				FbImage2(&town, 0);
			else
				FbImage2(&cave, 0);
		}
		break;
	case '*':
		FbImage2(&star, 0);
		break;
	case ' ': /* empty space, no need to draw anything */
		break;
	case '.':
		FbImage2(&grass, 0);
		break;
	case 'w':
		FbImage8bit2scrolling(&water, 0, water_scroll);
		break;
	case 'd':
		FbImage2(&desert, 0);
		break;
	case 'f':
		FbImage2(&tree, 0);
		break;
	case 'm':
		FbImage2(&mountain, 0);
		break;
	case '#':
		FbImage2(&wall, 0);
		break;
	case 'b':
		FbImage2(&bricks, 0);
		break;
	case '@':
		FbImage2(&badgey, 0);
		break;
	case '=':
		FbImage2(&woodfloor, 0);
		break;
	default:
		FbCharacter((unsigned char) '?'); /* unknown */
		break;
	}
}

struct visibility_check_data {
	int px, py, dx, dy, answer;
};

static unsigned char visibility_cache[9];

static int visibility_evaluator(int x, int y, void *cookie)
{
	struct visibility_check_data *d = cookie;

	if (d->answer == 0) /* visibility already blocked by previous call */
		return 1;
	if (x == d->px && y == d->py)  /* exclude the player and dest from blocking view of dest */
		return 0;
	if (x == d->dx && y == d->dy)
		return 0;
	if (x > 63)
		x -= 64;
	if (x < 0)
		x += 64;
	if (y > 63)
		y -= 64;
	if (y < 0)
		y += 64;
	char c = player.world->wm[windex(x, y)];
	if (c == 'm' || c == 'f' || c == '#' || (c >= '0' && c <= '9')) { /* mountains, forest, towns/caves block visibility */
		d->answer = 0;
		return 1; /* stop bline algorithm */
	}
	return 0; /* continue bline algorithm */
}

static int visibility_check(int px, int py, int x, int y)
{
	struct visibility_check_data d = { px, py, x, y, 1 };
	(void) bline(px, py, x, y, visibility_evaluator, &d);
	return d.answer;
}

static void draw_left_passage(int start, int start_inc)
{
	int x1, y1, x2, y2;	
	x1 = start / 256;
	y1 = (start + start_inc) / 256;
	x2 = (start + start_inc) / 256;
	y2 = LCD_YSIZE - (start + start_inc) / 256;

	FbLine(x1, y1, x2, y1);
	FbLine(x2, y1, x2, y2);
	FbLine(x1, y2, x2, y2);
}

static void draw_left_wall(int start, int start_inc)
{
	int x1, y1, x2, y2, x3, y3, x4, y4;	
	x1 = start / 256; 
	y1 = start / 256;
	x2 = (start + start_inc) / 256;
	y2 = (start + start_inc) / 256;
	x3 = x2;
	y3 = LCD_YSIZE - (start + start_inc) / 256;
	x4 = x1;
	y4 = LCD_YSIZE - start / 256;

	FbLine(x1, y1, x2, y2);
	FbLine(x2, y2, x3, y3);
	FbLine(x3, y3, x4, y4);
}

static void draw_right_passage(int start, int start_inc)
{
	int x1, y1, x2, y2;

	x1 = LCD_XSIZE - start / 256;
	y1 = (start + start_inc) / 256;
	x2 = x1 - start_inc / 256;
	y2 = LCD_YSIZE - (start + start_inc) / 256;

	FbLine(x1, y1, x2, y1);
	FbLine(x2, y1, x2, y2);
	FbLine(x2, y2, x1, y2);
}

static void draw_right_wall(int start, int start_inc)
{
	int x1, y1, x2, y2, x3, y3, x4, y4;

	x1 = LCD_XSIZE - start / 256;
	y1 = start / 256;
	x2 = x1 - start_inc / 256;
	y2 = y1 + start_inc / 256;
	x3 = x2;
	y3 = LCD_YSIZE - (start + start_inc) / 256;
	x4 = x1;
	y4 = LCD_YSIZE - start / 256;

	FbLine(x1, y1, x2, y2);
	FbLine(x2, y2, x3, y3);
	FbLine(x3, y3, x4, y4);
}

static void draw_back_wall(int start, int start_inc)
{
	int x1, y1, x2, y2;

	x1 = (start + start_inc) / 256;
	y1 = (start + start_inc) / 256;
	x2 = LCD_XSIZE - (start + start_inc) / 256;
	y2 = LCD_YSIZE - (start + start_inc) / 256;

	FbLine(x1, y1, x2, y1);
	FbLine(x1, y2, x2, y2);
}

static void draw_left_cave(int x, int y, int start, int start_inc)
{
	int left = player.dir - 1;
	if (left < 0)
		left += 4;
	int lx = x + xo4[left];
	int ly = y + yo4[left];
	if (lx < 0)
		lx += 64;
	if (lx > 63)
		lx -= 64;
	if (ly < 0)
		ly += 64;
	if (ly > 63)
		ly -= 64;
	if (dynmap[windex(lx, ly)] == '#')
		draw_left_wall(start, start_inc);
	else
		draw_left_passage(start, start_inc);
}

static void draw_right_cave(int x, int y, int start, int start_inc)
{
	int right = player.dir + 1;
	if (right > 3)
		right -= 4;
	int rx = x + xo4[right];
	int ry = y + yo4[right];
	if (rx < 0)
		rx += 64;
	if (rx > 63)
		rx -= 64;
	if (ry < 0)
		ry += 64;
	if (ry > 63)
		ry -= 64;
	if (dynmap[windex(rx, ry)] == '#')
		draw_right_wall(start, start_inc);
	else
		draw_right_passage(start, start_inc);
}

static int draw_back_cave(int x, int y, int start, int start_inc)
{
	int bx = x + xo4[player.dir];
	int by = y + yo4[player.dir];
	if (bx < 0)
		bx += 64;
	if (bx > 63)
		bx -= 64;
	if (by < 0)
		by += 64;
	if (by > 63)
		by -= 64;
	if (dynmap[windex(bx, by)] == '#') {
		draw_back_wall(start, start_inc);
		return 1;
	}
	return 0;
}

static void draw_up_ladder(int start, int start_inc, int scale)
{
	int x, y;
	x = LCD_XSIZE / 2;
	y = (start + start_inc / 2) / 256;
	FbDrawObject(upladder, ARRAY_SIZE(upladder), WHITE, x, y, scale);
}

static void draw_cave_screen(void)
{


	int x = player.x;
	int y = player.y;
	int start = 0;
	int ladder_start = 10;
	int start_inc = 18 * 256;
	int scale = 820;
	int hit_back_wall = 0;

	FbColor(WHITE);
	for (int i = 0; i < 4; i++) {
		draw_left_cave(x, y, start, start_inc);
		draw_right_cave(x, y, start, start_inc);
		hit_back_wall = draw_back_cave(x, y, start, start_inc);
		start += start_inc;
		ladder_start += start_inc;
		start_inc = (start_inc * 205) / 256; /* means: * 0.8 */
		scale = (scale * 205) / 256;
		if (x == 32 && y == 62)
			draw_up_ladder(ladder_start, start_inc, scale);
		if (hit_back_wall)
			break;
		x = x + xo4[player.dir];
		y = y + yo4[player.dir];
		if (x < 0 || x > 63 || y < 0 || y > 63)
			break;
	}
}

static int is_onscreen(int x, int y)
{
	int dx, dy;

	dx = abs(player.x - x);
	dy = abs(player.y - y);
	if (dx > 3 && dx < 62)
		return 0;
	if (dy > 4 && dy < 61)
		return 0;
	return 1;
}

static void draw_creature_at_xy(int x, int y, int creature_type, int instance)
{
	int icon;

	switch (creature_type) {
	case CREATURE_TYPE_CITIZEN:
		icon = creature[instance].csd.citizen.icon;
		break;
	case CREATURE_TYPE_GUARD:
		icon = creature_generic_data[creature_type].guard.icon;
		break;
	case CREATURE_TYPE_ROBOT1:
		icon = creature_generic_data[creature_type].robot1.icon;
		break;
	case CREATURE_TYPE_ROBOT2:
		icon = creature_generic_data[creature_type].robot2.icon;
		break;
	case CREATURE_TYPE_ROBOT3:
		icon = creature_generic_data[creature_type].robot2.icon;
		break;
	case CREATURE_TYPE_BYRSTRAN:
		icon = creature_generic_data[creature_type].byrstran.icon;
		break;
	case CREATURE_TYPE_HARGON:
		icon = creature_generic_data[creature_type].hargon.icon;
		break;
	case CREATURE_TYPE_ROVDAN:
		icon = creature_generic_data[creature_type].rovdan.icon;
		break;
	case CREATURE_TYPE_SKAVO:
		icon = creature_generic_data[creature_type].skavo.icon;
		break;
	case CREATURE_TYPE_TARCON:
		icon = creature_generic_data[creature_type].tarcon.icon;
		break;
	case CREATURE_TYPE_ZUNARO:
		icon = creature_generic_data[creature_type].zunaro.icon;
		break;
	default:
#if TARGET_SIMULATOR
		printf("No icon for creature type %hhu\n", creature[instance].type);
#endif
		icon = 0;
		break;
	}
	FbMove(x, y);
	FbImage2(creature_icon[icon], 0);
}

static void draw_creature(int i)
{
	int cx, cy;

	creature[i].onscreen_and_visible = 0;
	if (!is_onscreen(creature[i].x, creature[i].y))
		return;

	cx = creature[i].x - player.x;
	if (cx >= 61)
		cx -= 64;
	if (cx <= -61)
		cx += 64;
	cy = creature[i].y - player.y;
	if (cy >= 60)
		cy -= 64;
	if (cy <= -60)
		cy += 64;
	/* At this point cx, cy is the position relative to the player */

	/* Adjust, since player is at center of screen */
	cx += 3;
	cy += 4;

	/* Check visibility */
	if (!(visibility_cache[cy] & (1 << cx)))
		return;

	/* up to now coordinates have been in cells, convert to pixels */
	cx *= 16;
	cy *= 16;

	cx += 8;
	cy += 8;

	creature[i].onscreen_and_visible = 1;
	if (creature[i].type == CREATURE_TYPE_CITIZEN)
		player.in_shop = creature[i].csd.citizen.shopkeep;
	draw_creature_at_xy(cx, cy, creature[i].type, i);
}

static void draw_creatures(void)
{
	player.in_shop = SHOP_NONE;
	for (int i = 0; i < *ncreatures; i++) {
		draw_creature(i);
	}
}

static void draw_ship(int i)
{
	int cx, cy;

	if (!is_onscreen(ship[i].x, ship[i].y))
		return;

	cx = ship[i].x - player.x;
	if (cx >= 61)
		cx -= 64;
	if (cx <= -61)
		cx += 64;
	cy = ship[i].y - player.y;
	if (cy >= 60)
		cy -= 64;
	if (cy <= -60)
		cy += 64;
	/* At this point cx, cy is the position relative to the player */

	/* Adjust, since player is at center of screen */
	cx += 3;
	cy += 4;

	/* Check visibility */
	if (!(visibility_cache[cy] & (1 << cx)))
		return;

	/* up to now coordinates have been in cells, convert to pixels */
	cx *= 16;
	cy *= 16;

	cx += 8;
	cy += 8;

	FbMove(cx, cy);
	FbImage2(&ship_icon, 0);
}

static void draw_ships(void)
{
	if (player.in_town || player.in_cave)
		return;
	if (player.world->type != WORLD_TYPE_PLANET)
		return;
	for (int i = 0; i < nships; i++)
		draw_ship(i);
}

static void generic_move(struct creature *self)
{
	static unsigned int seed = 0x5a5a5a5a;

	int nx, ny, d;
	unsigned char ch;

	d = (xorshift(&seed) % 4);
	nx = self->x + xo4[d];
	if (nx < 0 || nx > 63)
		return;
	ny = self->y + yo4[d];
	if (ny < 0 || ny > 63)
		return;
	ch = dynmap[windex(nx, ny)];

	if (ch != 'b' && ch != 'd') /* stay on roads */
		return;

	/* Stay close to home base */
	if (abs(nx - self->homex) > 5 || abs(ny - self->homey) > 5)
		return;

	self->x = nx;
	self->y = ny;
}

static void generic_monster_move(struct creature *self)
{
	static unsigned int seed = 0x5a5a5a5a;

	int nx, ny, d;
	unsigned char ch;

	d = (xorshift(&seed) % 4);
	nx = self->x + xo4[d];
	if (nx < 0 || nx > 63)
		return;
	ny = self->y + yo4[d];
	if (ny < 0 || ny > 63)
		return;
	ch = player.world->wm[windex(nx, ny)];

	if (ch != '.' && ch != 'f' && ch != 'd') /* stay on grass, forest and desert */
		return;

	/* Stay with 20 manhattan distance of home base */
	if (abs(nx - self->homex) > 20 || abs(ny - self->homey) > 20)
		return;

	self->x = nx;
	self->y = ny;
}


static void citizen_move(struct creature *self)
{
	if (self->csd.citizen.shopkeep == SHOP_NONE) {
		generic_move(self);
		return;
	}

	int nx, ny, d;
	unsigned char ch;

	/* Move left or right towards player */
	if (player.x > self->x)
		d = 1;
	else if (player.x < self->x)
		d = 3;
	else
		return; /* Already in line with player */

	nx = self->x + xo4[d];
	if (nx < 0 || nx > 63)
		return;
	ny = self->y + yo4[d];
	if (ny < 0 || ny > 63)
		return;

	ch = dynmap[windex(nx, ny)];
	if (ch != '=') /* stay on shop floor */
		return;

	/* Stay close to home base */
	if (abs(nx - self->homex) > 5 || abs(ny - self->homey) > 5)
		return;

	self->x = nx;
	self->y = ny;
}

static const struct note combat_fanfare_notes[] = {
	{ NOTE_E4, 100 },
	{ NOTE_B4, 200 },
	{ NOTE_E4, 100 },
	{ NOTE_B4, 200 },
	{ NOTE_E4, 100 },
	{ NOTE_B4, 400 },
};

static const struct tune combat_fanfare = {
	ARRAY_SIZE(combat_fanfare_notes),
	combat_fanfare_notes,
};

static void enter_combat(int cr)
{
	set_badgey_state(BADGEY_COMBAT);
	screen_changed = 1;
	play_tune(&combat_fanfare, NULL, NULL);
	player.cbx = 3;
	player.cby = 7;
	player.moving = 0; /* so we don't continue moving after combat finishes */

	int x = 2;
	int y = 1;
	for (int i = 0; i < creature[cr].no_in_party; i++) {
		combat_creature[i].x = x;
		combat_creature[i].y = y;
		combat_creature[i].type = creature[cr].type;
		combat_creature[i].hit_points = 100; /* TODO: something more sophisticated */
		x = x + 2;
		if (x > 7) {
			x = 3;
			y += 2;
		}
	}
	ncombat_creatures = creature[cr].no_in_party;
	overworld_combat_creature = cr;

#if TARGET_SIMULATOR
	printf("Entered combat!\n");
#endif
}

static void move_creature(int i)
{
	int t = creature[i].type;

	t = creature[i].type;
	if (creature_generic_data[t].move)
		creature_generic_data[t].move(&creature[i]);
	else
		generic_move(&creature[i]);
	if (creature[i].x == player.x && creature[i].y == player.y &&
			player.world->type != WORLD_TYPE_TOWN)
		enter_combat(i);
}

static void move_creatures(void)
{
	for (int i = 0; i < *ncreatures; i++) {
		move_creature(i);
	}
}

static void move_ship(int i)
{
	static unsigned int seed = 0xa5a5a5a5;
	int nx = ship[i].x;
	int ny = ship[i].y;

	if ((xorshift(&seed) % 10) == 0)
		ship[i].dir = xorshift(&seed) % 4;

	nx = nx + xo4[ship[i].dir];
	ny = ny + xo4[ship[i].dir];
	if (nx > 63)
		nx -= 64;
	if (nx < 0)
		nx += 64; 	
	if (ny > 63)
		ny -= 64;
	if (ny < 0)
		ny += 64; 	
	if (player.world->wm[windex(nx, ny)] == 'w') {
		ship[i].x = nx;
		ship[i].y = ny;
	} else {
		ship[i].dir = xorshift(&seed) % 4;
	}
}

static void move_ships(void)
{
	if (player.world->type != WORLD_TYPE_PLANET)
		return;
	for (int i = 0; i < nships; i++) {
		move_ship(i);
	}
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbClear();
	FbColor(WHITE);

	if (player.in_cave) {
		draw_cave_screen();
		screen_changed = 0;
		return;
	}

	int x, y, sx, sy, rx, ry;

	x = player.x - 3;
	rx = x;
	if (x < 0)
		x += 64;
	y = player.y - 4;
	ry = y;
	if (y < 0)
		y += 64;
	sx = 8;
	sy = 8;
	int count = 0;
	memset(visibility_cache, 0, sizeof(visibility_cache));
	do {
		unsigned char c = (unsigned char) player.world->wm[windex(x, y)];
		if (player.world->type == WORLD_TYPE_SPACE ||
			visibility_check(player.x, player.y, rx, ry)) {
			draw_cell(sx, sy, c);
			visibility_cache[sy / 16] |= (1 << (sx / 16));
		}
		x++;
		rx++;
		count++;
		if (x > 63)
			x -= 64;
		sx += 16;
		if ((count % 7) == 0) {
			if (count == 7 * 9)
				break;
			x = player.x - 3;
			rx = player.x - 3;
			if (x < 0)
				x += 64;
			sx = 8;
			y++;
			ry++;
			sy += 16;
			if (y > 63) {
				y = y - 64;
			}
		}
	} while (1);

	draw_cell(8 + 16 * 3, 8 + 16 * 4, '@');

	draw_creatures();
	draw_ships();

	char buf[20];
	snprintf(buf, sizeof(buf), "(%d, %d)", player.x, player.y);
	FbMove(0, 152);
	FbWriteString(buf);

	screen_changed = 0;
	FbPushBuffer();
}

static void badgey_cave_menu(void)
{
	static int menu_setup = 0;

	if (!menu_setup) {
		dynmenu_clear(&cave_menu);
		dynmenu_init(&cave_menu, cave_menu_item, ARRAY_SIZE(cave_menu_item));
		dynmenu_set_title(&cave_menu, "", "", "");
		if (player.x == 32 && player.y == 62)
			dynmenu_add_item(&cave_menu, "CLIMB UP", BADGEY_RUN, 0);
		dynmenu_add_item(&cave_menu, "NEVERMIND", BADGEY_RUN, 1);
		dynmenu_add_item(&cave_menu, "QUIT", BADGEY_EXIT_CONFIRM, 2);
		menu_setup = 1;
	}

	if (!dynmenu_let_user_choose(&cave_menu))
		return;

	switch (dynmenu_get_user_choice(&cave_menu)) {
	case 0: /* climb up */
		if (player.world->type == WORLD_TYPE_CAVE && player.world_level > 0) {
			struct badgey_world const *old_world = player.old_world[player.world_level];
			if (old_world) {
				player.world = old_world;
				player.world_level--;
				player.x = player.wx[player.world_level];
				player.y = player.wy[player.world_level];
				set_badgey_state(BADGEY_RUN);
				screen_changed = 1;
				player.in_cave = 0;
				spawn_planet_initial_monsters();
				spawn_planet_initial_ships();
			}
			menu_setup = 0;
		}
		break;
	case DYNMENU_SELECTION_ABORTED:
	case 1: /* nevermind */
		menu_setup = 0;
		set_badgey_state(BADGEY_RUN);
		screen_changed = 1;
		break;
	case 2: /* quit */
		screen_changed = 1;
		confirm_exit();
		menu_setup = 0;
		break;
	}
}

static void badgey_talk_to_shopkeeper(void)
{
	static int menu_setup = 0;
	int st, n;
	int stats = 0;

	st = player.in_shop;
	if (st < 0 || st >= (int) ARRAY_SIZE(proprietor)) {
#if TARGET_SIMULATOR
		printf("Bad value in st: %d %s:%d\n", st, __FILE__, __LINE__);
#endif
		set_badgey_state(BADGEY_RUN);
		return;
	}

	if (!menu_setup) {
		dynmenu_clear(&town_menu);
		dynmenu_init(&town_menu, town_menu_item, ARRAY_SIZE(town_menu_item));
		dynmenu_set_title(&town_menu, shopname[st], "", "");
		dynmenu_add_item(&town_menu, "NEVERMIND", BADGEY_RUN, 254);
		n = 1;
		for (size_t i = 0; i < ARRAY_SIZE(shop_item); i++) {
			if (shop_item[i].shop_type == st) {
				char menu_item[15];
				snprintf(menu_item, 15, "%2d %s",
					shop_item[i].price, shop_item[i].name);
				dynmenu_add_item(&town_menu, menu_item, BADGEY_RUN, i);
				n++;
			}
		}
		stats = n; n++;
		dynmenu_add_item(&town_menu, "STATS", BADGEY_STATS, stats); 
		dynmenu_add_item(&town_menu, "QUIT", BADGEY_EXIT_CONFIRM, 255);
		menu_setup = 1;
	}

	if (!dynmenu_let_user_choose(&town_menu))
		return;

	int choice = dynmenu_get_user_choice(&town_menu);

	if (choice == 254) { /* Nevermind */
		screen_changed = 1;
		menu_setup = 0;
		set_badgey_state(BADGEY_TOWN_MENU);
		return;
	}
	if (choice == 255) { /* exit */
		screen_changed = 1;
		confirm_exit();
		menu_setup = 0;
		return;
	}
	if (choice > 0 && choice < (int) ARRAY_SIZE(shop_item)) { /* Buy something */
		char message[255];
		if (player.money < shop_item[choice].price) {
			snprintf(message, sizeof(message), "\n\n"
					" SORRY YOU DO\n NOT HAVE\n ENOUGH MONEY\n"
					" MONEY FOR\n THAT\n");
		} else {
			snprintf(message, sizeof(message), "\n\nYOU PAID %2d\nFOR\n%s\n",
					shop_item[choice].price,
					shop_item[choice].name);
			player.money -= shop_item[choice].price;
			player.carrying[choice]++;
		}
		status_message(message);
		screen_changed = 1;
		menu_setup = 0;
		return;
	}
	if (choice == stats) {
		screen_changed = 1;
		set_badgey_state(BADGEY_STATS);
	}
}

static int citizen_near_player(void)
{
	for (int i = 0; i < *ncreatures; i++) {
		switch (creature[i].type) {
		case CREATURE_TYPE_CITIZEN:
		case CREATURE_TYPE_GUARD:
		case CREATURE_TYPE_ROBOT1:
		case CREATURE_TYPE_ROBOT2:
		case CREATURE_TYPE_ROBOT3:
			break;
		default:
			continue;
		}

		/* don't count shopkeepers */
		if (creature[i].type == CREATURE_TYPE_CITIZEN &&
			creature[i].csd.citizen.shopkeep != SHOP_NONE)
			continue;

		int dx = abs(player.x - creature[i].x);
		if (dx > 1 && dx < 63)
			continue;
		int dy = abs(player.y - creature[i].y);
		if (dy > 1 && dy < 63)
			continue;
		return i;
	}
	return -1;
}

static void badgey_talk_to_citizen(void)
{
	int c = citizen_near_player();
	char buf[255];
	int i;

	if (c < 0) {
		set_badgey_state(previous_badgey_state);
		return;
	}

	if (screen_changed) {
		i = creature[c].info;
		FbClear();
		FbColor(WHITE);
		FbBackgroundColor(BLACK);
		snprintf(buf, sizeof(buf), "\n %s:\n %s\n", creature[c].name, creature_info[i]);
		FbMove(0, 0);
		FbWriteString(buf);
		FbSwapBuffers();
		screen_changed = 0;
	}

	int down_latches = button_down_latches();

	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		screen_changed = 1;
		set_badgey_state(previous_badgey_state);
	}
}

static void badgey_town_menu(void)
{
	static int menu_setup = 0;
	int st;

	if (!menu_setup) {
		dynmenu_clear(&town_menu);
		dynmenu_init(&town_menu, town_menu_item, ARRAY_SIZE(town_menu_item));
		dynmenu_set_title(&town_menu, "", "", "");
		st = player.in_shop;
		dynmenu_add_item(&town_menu, "NEVERMIND", BADGEY_RUN, 0);
		if (st >= 0 && st < (int) ARRAY_SIZE(proprietor)) {
			dynmenu_add_item(&town_menu, "TALK TO", BADGEY_RUN, 1);
			dynmenu_add_item(&town_menu, proprietor[st], BADGEY_RUN, 1);
		} else {
#if TARGET_SIMULATOR
			if (st != SHOP_NONE) {
				printf("Bad value in st: %d\n", st);
			}
#endif
		}

		if (citizen_near_player() >= 0) {
			dynmenu_add_item(&town_menu, "TALK TO", BADGEY_RUN, 2);
			dynmenu_add_item(&town_menu, "  CITIZEN", BADGEY_RUN, 2);
		}

		dynmenu_add_item(&town_menu, "EQUIP WEAPON", BADGEY_STATS, 5);
		dynmenu_add_item(&town_menu, "EQUIP ARMOR", BADGEY_STATS, 6);
		dynmenu_add_item(&town_menu, "STATS", BADGEY_STATS, 3);
		dynmenu_add_item(&town_menu, "QUIT", BADGEY_EXIT_CONFIRM, 4);
		menu_setup = 1;
	}

	if (!dynmenu_let_user_choose(&town_menu))
		return;

	switch (dynmenu_get_user_choice(&town_menu)) {
	case 0: /* Nevermind */
		set_badgey_state(BADGEY_RUN);
		break;
	case 1: /* talk to shop keeper */
		set_badgey_state(BADGEY_TALK_TO_SHOPKEEPER);
		break;
	case 2: set_badgey_state(BADGEY_TALK_TO_CITIZEN);
		break;
	case 3: /* stats */
		set_badgey_state(BADGEY_STATS);
		break;
	case 4: /* exit */
		confirm_exit();
		break;
	case 5: /* equip weapon */
		set_badgey_state(BADGEY_EQUIP_WEAPON);
		break;
	case 6: /* equip armor */
		set_badgey_state(BADGEY_EQUIP_ARMOR);
		break;
	default:
		set_badgey_state(BADGEY_RUN);
		break;
	}
	screen_changed = 1;
	menu_setup = 0;
}

static void badgey_planet_menu(void)
{
	int underchar = player.world->wm[windex(player.x, player.y)];
	static int menu_setup = 0;

	if (!menu_setup) {
		dynmenu_clear(&planet_menu);
		dynmenu_init(&planet_menu, planet_menu_item, ARRAY_SIZE(planet_menu_item));
		strcpy(planet_menu.title, "");
		dynmenu_add_item(&planet_menu, "BLAST OFF", BADGEY_RUN, 0);
		if (underchar >= '0' && underchar <= '4')
			dynmenu_add_item(&planet_menu, "ENTER TOWN", BADGEY_RUN, 1);
		if (underchar >= '5' && underchar <= '9')
			dynmenu_add_item(&planet_menu, "ENTER CAVE", BADGEY_RUN, 1);
		dynmenu_add_item(&planet_menu, "EQUIP WEAPON", BADGEY_STATS, 5);
		dynmenu_add_item(&planet_menu, "EQUIP ARMOR", BADGEY_STATS, 6);
		dynmenu_add_item(&planet_menu, "STATS", BADGEY_STATS, 2);
		dynmenu_add_item(&planet_menu, "NEVERMIND", BADGEY_RUN, 3);
		dynmenu_add_item(&planet_menu, "QUIT", BADGEY_EXIT_CONFIRM, 4);
		menu_setup = 1;
	}

	if (!dynmenu_let_user_choose(&planet_menu))
		return;

	switch (dynmenu_get_user_choice(&planet_menu)) {
	case 0: /* blast off */
		if (player.world->type == WORLD_TYPE_PLANET && player.world_level > 0) {
			struct badgey_world const *old_world = player.old_world[player.world_level];
			if (old_world) {
				player.world = old_world;
				player.world_level--;
				player.x = player.wx[player.world_level];
				player.y = player.wy[player.world_level];
				set_badgey_state(BADGEY_RUN);
				screen_changed = 1;
				creature = &space_creature[0];
				ncreatures = &nspace_creatures;
			}
			menu_setup = 0;
		}
		break;
	case 1: /* Enter town or cave */
		menu_setup = 0;
		if (underchar >= '0' && underchar <= '9')
			set_badgey_state(BADGEY_ENTER_TOWN_OR_CAVE);
		else
			set_badgey_state(BADGEY_RUN);
		break;
	case 2:
		screen_changed = 1;
		set_badgey_state(BADGEY_STATS);
		break;
	case DYNMENU_SELECTION_ABORTED:
	case 3: /* nevermind */
		menu_setup = 0;
		set_badgey_state(BADGEY_RUN);
		screen_changed = 1;
		break;
	case 4: /* quit */
		screen_changed = 1;
		confirm_exit();
		menu_setup = 0;
		break;
	case 5: /* equip weapon */
		screen_changed = 1;
		menu_setup = 0;
		set_badgey_state(BADGEY_EQUIP_WEAPON);
		break;
	case 6: /* equip armor */
		screen_changed = 1;
		menu_setup = 0;
		set_badgey_state(BADGEY_EQUIP_ARMOR);
		break;
	}
}

static void badgey_run(void)
{
	static uint64_t last_ms = 0;
	uint64_t now = rtc_get_ms_since_boot();
	int tick = 0;

	if (now - last_ms > 300) {
		screen_changed = 1;
		move_creatures();
		move_ships();
		last_ms = now;
		tick = 1;
		water_scroll++;
		if (water_scroll > 15)
			water_scroll = 0;
	}

	draw_screen();
	FbPushBuffer();
	if (player.in_cave)
		cave_check_buttons();
	else
		check_buttons(tick);
}

enum townfeature {
	town_ponds = 1 << 1,
	town_creek = 1 << 2,
	town_armoury = 1 << 4,
	town_weapons = 1 << 5,
	town_hackerspace = 1 << 6,
	town_temple = 1 << 7,
	town_inn = 1 << 9, /* for saving game progress to flash? */
};

static const struct town_info {
	const char *name;
	uint32_t feature;
} towninfo[] = {
	/* on planet 0 "OSSARIA" */
	{ "ZONNU",
		town_ponds | town_armoury | town_temple | town_inn,
	},
	{ "QUAZON",
		town_creek | town_armoury | town_weapons,
	},
	{ "DORVO",
		town_armoury | town_temple | town_weapons,
	},
	{ "BALF",
		town_ponds | town_armoury | town_weapons | town_hackerspace,
	},
	{ "ONVAL",
		town_creek | town_weapons | town_armoury | town_temple,
	},

	/* on planet 1 "NW42" */
	{ "SURSEE",
		town_armoury | town_temple | town_weapons,
	},
	{ "CALEV",
		town_creek | town_weapons | town_armoury | town_temple,
	},
	{ "NORJIG",
		town_ponds | town_armoury | town_weapons | town_hackerspace,
	},
	{ "KALFO",
		town_creek | town_armoury | town_weapons,
	},
	{ "BURNIP",
		town_ponds | town_armoury | town_temple | town_inn,
	},

	/* on planet 2 "BORTON" */
	{ "WASSU",
		town_creek | town_weapons | town_armoury | town_temple,
	},
	{ "JARLS",
		town_ponds | town_armoury | town_temple | town_inn,
	},
	{ "KORVIN",
		town_armoury | town_temple | town_weapons,
	},
	{ "LAKNIV",
		town_ponds | town_armoury | town_weapons | town_hackerspace,
	},
	{ "NEPHEST",
		town_creek | town_armoury | town_weapons,
	},

	/* on planet 3 "SKANG" */
	{ "HOJAX",
		town_ponds | town_armoury | town_weapons | town_hackerspace,
	},
	{ "SPEVO",
		town_armoury | town_temple | town_weapons,
	},
	{ "TORXUN",
		town_creek | town_armoury | town_weapons,
	},
	{ "TALSU",
		town_ponds | town_armoury | town_temple | town_inn,
	},
	{ "MERODOX",
		town_creek | town_weapons | town_armoury | town_temple,
	},

	/* on planet 3 "GNARG" */
	{ "JALTA",
		town_creek | town_armoury | town_weapons,
	},
	{ "SPINU",
		town_ponds | town_armoury | town_weapons | town_hackerspace,
	},
	{ "ILATI",
		town_armoury | town_temple | town_weapons,
	},
	{ "FRUNTZ",
		town_creek | town_weapons | town_armoury | town_temple,
	},
	{ "YARNOW",
		town_ponds | town_armoury | town_temple | town_inn,
	},
};

const char *hackerspacename[] = {
	"SEGVAULT",
	"BODGERY",
	"HACKALOT",
	"HACKHAUS",
	"COREDUMP",

	"CHAOSCLUB",
	"NOIZBRIDGE",
	"HACKDEN",
	"KLUDGERY",
	"GROKHAUS",

	"KAOSDORF",
	"HACKRVA",
	"SIGTRAP",
	"PAGEFAULT",
	"CPU_THREEPIO",

	"TXRXLABS",
	"FORKBOMB",
	"STACKTRACE",
	"DEADBEEF",
	"NULLPTR",

	"VOIDSTAR",
	"REZISTOR",
	"NERDCLUB",
	"SIGSEGV",
	"OHMS_LAW",
};

const char *weapons_store_name[] = {
	"WEAPONS",
	"ARMAMENTS",
	"ARMS_INC",
	"BLASTERS",
	"GUNS_N_STUF",
};

const char *armoury_name[] = {
	"ARMOR",
	"ARMOR_INC",
	"SHIELDS",
	"HELMSHIELD",
	"BLASTPROOF",
};

const char *pubname[] = {
	"BABELFISH",
	"SPACE_BAR",
	"TRADER_VI",
	"PLANETFALL",
	"BAR_ZERO",

	"CANTINA_X",
	"SCOOTERS",
	"RED_DWARF",
	"SCOTTYS",
	"GOLDEN_HART",

	"KRYTENS",
	"TECH_NOIR",
	"ALS_ALES",
	"MILLIWAYS",
	"QUARKS",

	"TRIBBLES",
	"EMOHAWK_PUB",
	"NOSTROMO_BAR",
	"MAGRATHEA",
	"SOLOS_SUDS",

	"DAGOBAH",
	"SERENITY",
	"PAN_GALACTIC",
	"TEN_FOUR",
	"MOES_BAR",
};

static int paint_town(int x, int y, void *cookie)
{
	char *c = cookie;
	dynmap[windex(x, y)] = *c;
	return 0;
}

static void paint_hline(int x1, int y, int x2, char c)
{
	int t;
	if (x1 > x2) {
		t = x1;
		x1 = x2;
		x2 = t;
	}
	for (int i = x1; i <= x2; i++)
		dynmap[windex(i, y)] = c;
}

static void paint_vline(int x, int y1, int y2, char c)
{
	int t;
	if (y1 > y2) {
		t = y1;
		y1 = y2;
		y2 = t;
	}
	for (int i = y1; i <= y2; i++)
		dynmap[windex(x, i)] = c;
}

/* Paint a line using char c of width w from x1 to y1
 * This is used to create roads, streams, walls, and so on in towns.
 */
static void paint_wide_line(int x1, int y1, int x2, int y2, int w, char c)
{
	int dx, dy;
	int xa, ya, xb, yb;

	dx = abs(x2 - x1);
	dy = abs(y2 - y1);
	if (dy > dx) { /* a mostly vertical line */
		xa = x1 - w / 2;
		xb = x2 - w / 2;
		for (int i = 0; i < w; i++) {
			bline(xa, y1, xb, y2, paint_town, &c);
			xa++;
			xb++;
		}
	} else { /* a mostly horizontal line */
		ya = y1 - w / 2;
		yb = y2 - w / 2;
		for (int i = 0; i < w; i++) {
			bline(x1, ya, x2, yb, paint_town, &c);
			ya++;
			yb++;
		}
	}
}

/* Return true if we find a char that blocks a building from being constructed*/
static int building_blocking_char(char c, char roadchar)
{
	if (c == roadchar) /* don't build on top of roads */
		return 1;
	if (c >= 'A' && c <= 'Z') /* don't build on top of signage */
		return 1;
	if (c == '_') /* don't build on top of signage */
		return 1;
	if (c == '#') /* don't build on top of walls */
		return 1;
	if (c == '=') /* don't build on top of other building's floors */
		return 1;
	return 0; /* must be ok to build here. */
}

/* Check that a building location is not blocked by something important */
static int building_location_ok(int x, int y, int w, int h, char roadchar)
{
	/* First check the four corners to more quickly reject bad locations */
	if (building_blocking_char(dynmap[windex(x, y)], roadchar))
		return 0;
	if (building_blocking_char(dynmap[windex(x + w - 1, y)], roadchar))
		return 0;
	if (building_blocking_char(dynmap[windex(x, y + h - 1)], roadchar))
		return 0;
	if (building_blocking_char(dynmap[windex(x + w - 1, y + h - 1)], roadchar))
		return 0;

	/* Since roads are contiguous, we only have to check the borders of the building */

	/* Check top edge */
	for (int i = x; i < x + w - 2; i++)
		if (building_blocking_char(dynmap[windex(i, y)], roadchar))
			return 0;
	/* check bottom edge */
	for (int i = x; i < x + w - 2; i++)
		if (building_blocking_char(dynmap[windex(i, y + h - 1)], roadchar))
			return 0;
	/* check left edge */
	for (int i = y + 1; i < y + h - 2; i++)
		if (building_blocking_char(dynmap[windex(x, i)], roadchar))
			return 0;
	/* check right edge */
	for (int i = y + 1; i < y + h - 2; i++)
		if (building_blocking_char(dynmap[windex(x + w - 1, i)], roadchar))
			return 0;
	return 1; /* nothing blocking, ok to build here */
}

static int distance_to_road(int x, int y, int xo, int yo, char roadchar)
{
	int i = 1;

	do {
		int tx = x + i * xo;
		int ty = y + i * yo;
		if (tx <= 6 || tx >= 58 || ty <= 6 || ty >= 58)
			return 1000000; /* infinite distance */
		char ch = dynmap[windex(tx, ty)];
		if (ch == roadchar)
			return i;
		if (ch == '#')
			return 1000000; /* can't go through walls */
		if ((ch >= 'A' && ch <= 'Z') || ch == '_')
			return 1000000; /* can't go through signage */
		i++;
	} while (i < 64);
	return 100000;
}

/* Creates a door in a new building and connects that door to the nearest existing
 * road with more road.  Returns 0-3 if the door was added on the top, right, bottom
 * or left, respectively. */
static int connect_building_to_road(int x, int y, int w, int h, int roadchar)
{
	int rc = 0;
	int d[4];
	int min, ans;
	int sx[4] = { x + w / 2, x + w, x + w / 2, x - 1 };
	int sy[4] = { y - 1, y + h / 2, y + h, y + h / 2 };

	for (int i = 0; i < 4; i++)
		d[i] = distance_to_road(sx[i], sy[i], xo4[i], yo4[i], roadchar);

	min = 1000;
	ans = -1;
	for (int i = 0; i < 4; i++) {
		if (min > d[i]) {
			min = d[i];
			ans = i;
		}
	}
	if (ans == -1) { /* Ok, fine.  Just put in a doors on the sides and forget the road */
		dynmap[windex(x, y + h / 2)] = '=';
		dynmap[windex(x + w - 1, y + h / 2)] = '=';
		rc = 1; /* left door */
	} else {
		for (int i = -1; i < d[ans]; i++) /* -1 to start at the wall of the building to put in a door */
			dynmap[windex(sx[ans] + i * xo4[ans], sy[ans] + i * yo4[ans])] =
				i == -1 ? '=' : roadchar;
		rc = ans;
	}
	return rc;
}

static int generate_building(const char *name, unsigned char shoptype, char roadchar, unsigned int *seed)
{

	int x, y, h, w, i;

	h = 6;
	w = strlen(name) + 2;
	/* choose building location */
	i = 0;
	do {
		x = xorshift(seed) % 42 + 8;
		y = xorshift(seed) % 42 + 8;
		i++;
	} while (!building_location_ok(x, y, w, h, roadchar) && i < 100);
	if (i >= 100) {
#ifdef TARGET_SIMULATOR
		printf("Failed to find building location for %s\n", name);
#endif
		return -1;
	}
#ifdef TARGET_SIMULATOR
	printf("building location took %d iterations\n", i);
#endif
	paint_hline(x, y, x + w - 1, '#'); /* top wall */
	paint_hline(x, y + h - 1, x + w - 1, '#'); /* bottom wall */
	paint_vline(x, y + 1, y + h - 2, '#'); /* left wall */
	paint_vline(x + w - 1, y + 1, y + h - 2, '#'); /* right wall */
	for (i = 1; i <= h - 2; i++) /* do the floor */
			paint_hline(x + 1, y + i, x + w - 2, '=');
	int rc = connect_building_to_road(x, y, w, h, roadchar);
	/* rc is 0, 1, 2 or 3 depending if door was top, right, bottom or left, respectively. */
	if (rc == 0) {/* door in top wall */
		/* put name at bottom part of building */
		memcpy(&dynmap[windex(x + 1, y + h - 3)], name, strlen(name));
		add_shopkeeper(x + 1, y + h - 2, shoptype, seed);
	} else { /* door not in top wall */
		/* put name at top part of building */
		memcpy(&dynmap[windex(x + 1, y + 2)], name, strlen(name));
		add_shopkeeper(x + 1, y + 1, shoptype, seed);
	}

	return 0;
}

static void fractal_road(int x1, int y1, int x2, int y2, int width, char roadchar, unsigned int *seed)
{
	int mx, my;
	int dx, dy, d2;
	int rx, ry;

	dx = abs(x1 - x2);
	dy = abs(y1 - y2);
	d2 = dx * dx + dy * dy;

	if (d2 < (2 * width) * (2 * width)) {
		paint_wide_line(x1, y1, x2, y2, width, roadchar);
		return;
	}

	if (x1 < x2)
		mx = x1 + (x2 - x1) / 2;
	else
		mx = x2 + (x1 - x2) / 2;
	if (y1 < y2)
		my = y1 + (y2 - y1) / 2;
	else
		my = y2 + (y1 - y2) / 2;

	rx = dx / 3;
	ry = dy / 3;
	if (rx > 0)
		rx = xorshift(seed) % rx;
	if (ry > 0)
		ry = xorshift(seed) % ry;
	mx = mx + rx;
	my = my + ry;

	fractal_road(x1, y1, mx, my, width, roadchar, seed);
	fractal_road(mx, my, x2, y2, width, roadchar, seed);
}

static void generate_road_system(unsigned int *seed, int roadwidth, int roadchar)
{
	int x0, y0, x1, y1, x2, y2, x3, y3;

	x0 = 20; y0 = 32;
	x1 = 20; y1 = 8;
	x2 = 58; y2 = 20;
	x3 = 20; y3 = 56;

	x1 += (xorshift(seed) % 24);
	x3 += (xorshift(seed) % 24);
	y2 += (xorshift(seed) % 24);

	fractal_road(x0, y0, x1, y1, roadwidth, roadchar, seed);
	fractal_road(x1, y1, x2, y2, roadwidth, roadchar, seed);
	fractal_road(x2, y2, x3, y3, roadwidth, roadchar, seed);
	fractal_road(x3, y3, x0, y0, roadwidth, roadchar, seed);
	if (xorshift(seed) % 2)
		fractal_road(x1, y1, x3, y3, roadwidth, roadchar, seed);
	if (xorshift(seed) % 2)
		fractal_road(x0, y0, x2, y2, roadwidth, roadchar, seed);
}

static void add_creek_to_town(unsigned int *seed)
{
	int x1, y1, x2, y2;

	if (xorshift(seed) & 0x01) {
		x1 = 0;
		x2 = 63;
		y1 = xorshift(seed) % 54 + 5;
		y2 = xorshift(seed) % 54 + 5;
		paint_wide_line(x1, y1, x2, y2, 5, 'w');
	} else {
		x1 = xorshift(seed) % 54 + 5;
		x2 = xorshift(seed) % 54 + 5;
		y1 = 0;
		y2 = 63;
		paint_wide_line(x1, y1, x2, y2, 5, 'w');
	}
}

static void add_shop_item(int shoptype, int item)
{
	if (shop[shoptype].nitems >= MAX_ITEMS_PER_SHOP) {
#if TARGET_SIMULATOR
		printf("Too many items in shop %d\n", shoptype);
#endif
		return;
	}
	shop[shoptype].item[shop[shoptype].nitems] = item;
	shop[shoptype].nitems++;
}

static void arrange_shop_contents(__attribute__((unused)) int town)
{
	for (int i = 0; i < NUMSHOPS; i++)
		shop[i].nitems = 0;

	for (size_t i = 0; i < ARRAY_SIZE(shop_item); i++) {
		int st = shop_item[i].shop_type;
		if (st < NUMSHOPS)
			add_shop_item(st, i);
	}

	/* Here is where we will add specialty items to shops based on town */
}

static void generate_town(int town_number)
{
	int x, y;
	unsigned int seed;
	int treecount = 0;

	x = player.x;
	y = player.y;

	char left = whats_there(player.world->wm, x, y, -1, 0);
	char right = whats_there(player.world->wm, x, y, 1, 0);
	char up = whats_there(player.world->wm, x, y, 0, -1);
	char down = whats_there(player.world->wm, x, y, 0, -1);

	treecount = ((up == 'f') + (right == 'f') + (left == 'f') + (down == 'f'));
	switch (treecount) {
	case 0:
		treecount = 100;
		break;
	case 1:
		treecount = 200;
		break;
	case 2:
		treecount = 400;
		break;
	case 3:
		treecount = 800;
		break;
	case 4:
		treecount = 1600;
		break;
	}
	memset(dynmap, '.', sizeof(dynmap)); /* cover the map with grass */
	memset(dynmap, '.', sizeof(dynmap)); /* cover the map with grass */

	/* cover topmost 5 rows of map with what's up */
	paint_wide_line(0, 2, 63, 2, 5, up);
	/* cover bottommost 5 rows of map with what's down */
	paint_wide_line(0, 60, 63, 60, 5, down);
	/* cover leftmost 5 cols of map with what's left */
	paint_wide_line(2, 0, 2, 63, 5, left);
	/* cover rightmost 5 cols of map with what's right */
	paint_wide_line(61, 0, 61, 63, 5, right);

	seed = (player.x + 64 * player.y * (town_number + 1)) ^ 0x5a5a5a5a;

	/* Sprinkle some trees around randomly */
	for (int i = 0; i < treecount; i++) {
		int tx = (xorshift(&seed) % 54) + 5;
		int ty = (xorshift(&seed) % 54) + 5;
		dynmap[windex(tx, ty)] = 'f';
	}

	/* Put a sign at the town entrance */
	/* Figure which world we are on. */
	int world_no = -1;
	for (size_t i = 0; i < ARRAY_SIZE(space.subworld); i++)
		if (player.world == space.subworld[i]) {
			world_no = i;
			break;
		}
#ifdef __linux
	if (world_no == -1) {
		printf("BUG detected at %s:%d, planet %s not in space.subworld\n",
				__FILE__, __LINE__, player.world->name);
		raise(SIGTRAP); /* trigger gdb, in case we're running under gdb. */
	}
#endif
	int town = town_number + (world_no * 5);


	int has_moat = ((xorshift(&seed) % 100) < 25);
	int has_wall = ((xorshift(&seed) % 100) < 25);

	if (has_moat) { /* paint moat surrounding town */
		paint_wide_line(2, 0, 2, 63, 3, 'w');
		paint_wide_line(62, 0, 62, 63, 3, 'w');
		paint_wide_line(0, 2, 63, 2, 3, 'w');
		paint_wide_line(0, 62, 63, 62, 3, 'w');
	}

	if (has_wall) { /* paint wall surrounding town */
		paint_wide_line(4, 0, 4, 63, 3, '#');
		paint_wide_line(60, 0, 60, 63, 3, '#');
		paint_wide_line(0, 4, 63, 4, 3, '#');
		paint_wide_line(0, 60, 63, 60, 3, '#');
	}

	if (towninfo[town].feature & town_creek)
		add_creek_to_town(&seed);

	char roadchar = (xorshift(&seed) % 2) ? 'd' : 'b';
	int roadwidth = (xorshift(&seed) % 3) + 3;
	paint_wide_line(0, 32, 20, 32, roadwidth, roadchar); /* build road into town, piercing moat & wall, if any. */
	generate_road_system(&seed, roadwidth, roadchar);

	/* Write the "welcome to such-and-such town" sign */
	char *l = &dynmap[windex(8, 28)];
	memcpy(l, "WELCOME", 7);
	l = &dynmap[windex(8, 29)];
	memcpy(l, "TO_", 3);
	l += 3;
	memcpy(l, towninfo[town].name, strlen(towninfo[town].name));

	/* Generate buildings */
	generate_building(pubname[town], SHOP_PUB, roadchar, &seed);
	generate_building("INN", SHOP_INN, roadchar, &seed);
	if (towninfo[town].feature & town_hackerspace)
		generate_building(hackerspacename[town], SHOP_HACKERSPACE, roadchar, &seed);
	if (towninfo[town].feature & town_weapons)
		generate_building(weapons_store_name[town % 5], SHOP_WEAPONS, roadchar, &seed);
	if (towninfo[town].feature & town_armoury)
		generate_building(armoury_name[town % 5], SHOP_ARMOURY, roadchar, &seed);
	if (towninfo[town].feature & town_temple)
		generate_building("TEMPLE", SHOP_TEMPLE, roadchar, &seed);

	arrange_shop_contents(town);

	for (int i = 0; i < 8; i++)
		add_guard(roadchar, &seed);

	for (int i = 0; i < 8; i++)
		add_citizen(roadchar, &seed);

	for (int i = 0; i < 2; i++)
		add_robot1(roadchar, &seed);

	for (int i = 0; i < 2; i++)
		add_robot3(roadchar, &seed);
}

static void enter_dynmap(int x, int y)
{
	player.world_level++;
	player.wx[player.world_level] = player.x;
	player.wy[player.world_level] = player.y;
	player.old_world[player.world_level] = player.world;
	player.x = x;
	player.y = y;
	player.world = &dynworld;
}

static void enter_town(int town_number)
{
	creature = &town_creature[0];
	ncreatures = &ntown_creatures;
	*ncreatures = 0;
	generate_town(town_number);
	dynworld.type = WORLD_TYPE_TOWN;
	enter_dynmap(6, 32);
	player.in_town = 1;
}

static void dig_cave(char *map, int x, int y, int dir, unsigned int *seed) 
{
	/* check we're not too close to the edge of the map */ 
	if (x < 1 || x > 62 || y < 1 || y > 62)
		return; /* quit digging */

	/* Check we're not about to connect to another existing tunnel */
	for (int i = -2; i < 3; i++) {
		int td = dir + i;
		if (td < 0)
			td += 8;
		if (td > 7)
			td -= 8;
		if (map[windex(x + xo8[td], y + yo8[td])] == ' ') /* would connect to existing tunnel? */
			return; /* quit digging */
	}

	map[windex(x, y)] = ' '; /* safe to dig out x,y */

	/* Maybe branch left or right */
	if ((xorshift(seed) % 100) < 30) {
		int newdir;
		if ((xorshift(seed) % 100) < 50) {
			newdir = dir + 2;
			if (newdir > 7)
				newdir -= 8;
		} else {
			newdir = dir - 2;
			if (newdir < 0)
				newdir += 8;
		}
		dig_cave(map, x + xo8[newdir], y + yo8[newdir], newdir, seed);
	}
	if ((xorshift(seed) % 100) < 5) /* 5% chance to quit digging */
		return;
	dig_cave(map, x + xo8[dir], y + yo8[dir], dir, seed);
}

static void print_cave(char *map)
{
	for (int i = 0; i < 64; i++) {
		for (int j = 0; j < 64; j++) {
			if (j == 32 && i == 62)
				printf("X");
			else
				printf("%c", map[windex(j, i)]);
		}
		printf("\n");
	}
}

static void generate_cave(int cave_number)
{
	int world_no = -1;
	for (size_t i = 0; i < ARRAY_SIZE(space.subworld); i++)
		if (player.world == space.subworld[i]) {
			world_no = i;
			break;
		}
#ifdef __linux
	if (world_no == -1) {
		printf("BUG detected at %s:%d, planet %s not in space.subworld\n",
				__FILE__, __LINE__, player.world->name);
		raise(SIGTRAP); /* trigger gdb, in case we're running under gdb. */
	}
#endif
	int caveno = cave_number + (world_no * 5) + 5;
	unsigned int seed = (player.x + 64 * player.y * (caveno + 1)) ^ 0x5a5a5a5a;

	memset(dynmap, '#', sizeof(dynmap)); /* Fill the map with walls. */
	dig_cave(dynmap, 32, 62, 0, &seed);
	print_cave(dynmap);
}

static void enter_cave(int cave_number)
{
	generate_cave(cave_number);
	dynworld.type = WORLD_TYPE_CAVE;
	enter_dynmap(32, 62);
	player.dir = 0;
	player.in_cave = 1;
	screen_changed = 1;
}

static void badgey_enter_town_or_cave(void)
{
	int underchar = player.world->wm[windex(player.x, player.y)];
	if (underchar >= '0' && underchar <= '4') {
		enter_town(underchar - '0');
	} else if (underchar >= '5' && underchar <= '9') {
		enter_cave(underchar - '0');
	}
	set_badgey_state(BADGEY_RUN);
}

static void badgey_stats(void)
{
	char buf[100];
	int eq;

	if (screen_changed) {
		FbMove(0, 0);
		snprintf(buf, sizeof(buf), "HP: %d\n", player.hp);
		FbWriteString(buf);
		snprintf(buf, sizeof(buf), "GP: %d\n", player.money);
		FbWriteString(buf);

		eq = player.equipped_armor;
		snprintf(buf, sizeof(buf), "WEARING:\n  %s\n",
			eq == EQUIPPED_NONE ? "NO ARMOR" : shop_item[eq].name);
		FbWriteString(buf);

		eq = player.equipped_weapon;
		snprintf(buf, sizeof(buf), "WIELDING:\n  %s\n",
			eq == EQUIPPED_NONE ? "NO WEAPON" : shop_item[eq].name);
		FbWriteString(buf);
	
		FbSwapBuffers();
		screen_changed = 0;
	}

	int down_latches = button_down_latches();

	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_A, down_latches) ||
		BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		screen_changed = 1;
		set_badgey_state(previous_badgey_state);
	}
}

static void badgey_equip(void)
{
	static int menu_setup = 0;
	int count = 0;
	unsigned char t1, t2;
	static struct dynmenu item_menu;
	static struct dynmenu_item item_menu_item[15];
	char *title;

	if (badgey_state == BADGEY_EQUIP_ARMOR) {
		t1 = ITEM_TYPE_ARMOR;
		t2 = t1;
		title = "EQUIP ARMOR";
	} else if (badgey_state == BADGEY_EQUIP_WEAPON) {
		t1 = ITEM_TYPE_RANGED_WEAPON;
		t2 = ITEM_TYPE_MELEE_WEAPON;
		title = "EQUIP WEAPON";
	}

	if (!menu_setup) {
		dynmenu_clear(&item_menu);
		dynmenu_init(&item_menu, item_menu_item, ARRAY_SIZE(item_menu_item));
		dynmenu_set_title(&item_menu, title, "", "");
		for (int i = 0; i < (int) ARRAY_SIZE(shop_item); i++) {
			if (player.carrying[i] == 0)
				continue;
			if (shop_item[i].item_type == t1 ||
				shop_item[i].item_type == t2) {
				dynmenu_add_item(&item_menu, shop_item[i].name, badgey_state, i);
				count++;
				if (count >= (int) ARRAY_SIZE(item_menu_item) - 1)
					break;
			}
		}
		dynmenu_add_item(&item_menu, "NEVERMIND", BADGEY_RUN, DYNMENU_SELECTION_ABORTED);
		menu_setup = 1;
	}

	if (!dynmenu_let_user_choose(&item_menu))
		return;

	int choice = dynmenu_get_user_choice(&item_menu);
	if (choice != DYNMENU_SELECTION_ABORTED &&
		choice != (unsigned char) DYNMENU_SELECTION_ABORTED) {
		if (badgey_state == BADGEY_EQUIP_ARMOR)
			player.equipped_armor = choice;
		else if (badgey_state == BADGEY_EQUIP_WEAPON)
			player.equipped_weapon = choice;
	}
	menu_setup = 0;
	set_badgey_state(BADGEY_RUN);
}

static void remove_combat_creature(int i)
{
	if (i < 0 || i >= ncombat_creatures)
		return;
	if (i < ncombat_creatures - 1)
		combat_creature[i] = combat_creature[ncombat_creatures - 1];
	ncombat_creatures--;
}

static void move_combat_creature(__attribute__((unused)) int i)
{
}

static void move_combat_creatures(void)
{
	for (int i = 0; i < ncombat_creatures;) {
		move_combat_creature(i);
		if (combat_creature[i].hit_points <= 0)
			remove_combat_creature(i);
		else
			i++;
	}
}

static void draw_combat_field(void)
{
	int ch;

	ch = player.world->wm[windex(player.x, player.y)];
	if (ch == 'f') /* change forest to grass */
		ch = '.';
	if (ch >= '0' && ch <= '9') /* town or cave? change to grass */
		ch = '.';

	for (int y = 0; y < 9; y++) {
		for (int x = 0; x < 7; x++) {
			draw_cell(16 * x + 8, 16 * y + 8, ch);
		}
	}
}

static void draw_combat_creatures(void)
{

	for (int i = 0; i < ncombat_creatures; i++) {
		struct creature *c = &combat_creature[i];
		draw_creature_at_xy(16 * c->x + 8, 16 * c->y + 8, c->type, 0);
	}
}

static void draw_combat_player(void)
{
	FbMove(16 * player.cbx + 8, 16 * player.cby + 8);
	FbImage2(&badgey, 0);
}

static void draw_combat_screen(void)
{
	draw_combat_field();
	draw_combat_creatures();
	draw_combat_missiles();
	draw_combat_player();
}

static void player_strike_with_weapon(__attribute__((unused)) int direction)
{
	add_missile((16 * player.cbx + 8) * 256, (16 * player.cby + 8) * 256, 2048 * xo4[direction], 2048 * yo4[direction], 100); 
}

static void badgey_combat(void)
{
	int nx, ny;
	static int awaiting_direction = 0;
	static uint64_t last_ms = 0;
	static uint64_t last_missile_move_time = 0;
	uint64_t now = rtc_get_ms_since_boot();
	int direction = -1;
	static unsigned int seed = 0xabcdef98;

	if (now - last_missile_move_time > 100) {
		move_combat_missiles();
		last_missile_move_time = now;
	}
	
	if (now - last_ms > 300) {
		screen_changed = 1;
		move_combat_creatures();
		last_ms = now;
	}

	if (screen_changed) {
		draw_combat_screen();
		FbSwapBuffers();
		screen_changed = 0;
	}

	int down_latches = button_down_latches();

	nx = player.cbx;
	ny = player.cby;
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		if (awaiting_direction) {
			direction = 3;
		} else if (player.cbx > 0) {
			nx = player.cbx - 1;
		}
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		if (awaiting_direction) {
			direction = 1;
		} else if (player.cbx < 6) {
			nx = player.cbx + 1;
		}
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		if (awaiting_direction) {
			direction = 0;
		} else if (player.cby > 0) {
			ny = player.cby - 1;
		}
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		if (awaiting_direction) {
			direction = 2;
		} else if (player.cby < 8) {
			ny = player.cby + 1;
		}
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
	}
	if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		awaiting_direction = 1;
	}

	/* Move player to (nx, ny) if nothing's in the way */
	if (nx != player.cbx || ny != player.cby) {
		int collision = 0;
		for (int i = 0; i < ncombat_creatures; i++) {
			if (nx == combat_creature[i].x && ny == combat_creature[i].y)
				collision = 1;
		}
		if (!collision) {
			player.cbx = nx;
			player.cby = ny;
		}
	}

	if (awaiting_direction && direction != -1) {
		player_strike_with_weapon(direction);
		awaiting_direction = 0;
	}

	if (ncombat_creatures == 0) { /* player killed everything? */
		set_badgey_state(BADGEY_RUN);
		/* respawn the overworld creature far away so we don't immediately jump back into combat */
		respawn_monster(overworld_combat_creature, &seed);
	}
}

static void badgey_exit_confirm(void)
{
	static int menu_ready = 0;
	static struct dynmenu ecm;
	static struct dynmenu_item ecm_item[2];

	if (!menu_ready) {
		dynmenu_clear(&ecm);
		dynmenu_init(&ecm, ecm_item, ARRAY_SIZE(ecm_item));
		dynmenu_set_title(&ecm, "REALLY QUIT?", "", "");
		dynmenu_add_item(&ecm, "NO, DON'T QUIT", BADGEY_RUN, 1);
		dynmenu_add_item(&ecm, "YES, QUIT", BADGEY_EXIT_CONFIRM, 2);
		menu_ready = 1;
	}

	if (!dynmenu_let_user_choose(&ecm))
		return;

	switch (dynmenu_get_user_choice(&ecm)) {
	case 2:
		set_badgey_state(BADGEY_EXIT);
		break;
	case 1:
	default:
		unconfirm_exit();
		break;
	}
}

static void badgey_exit(void)
{
	set_badgey_state(BADGEY_CONTINUE); /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

/* You will need to rename badgey_cb() something else. */
void badgey_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (badgey_state) {
	case BADGEY_INIT:
		badgey_init();
		break;
	case BADGEY_CONTINUE:
		badgey_continue();
		break;
	case BADGEY_PLANET_MENU:
		badgey_planet_menu();
		break;
	case BADGEY_CAVE_MENU:
		badgey_cave_menu();
		break;
	case BADGEY_TOWN_MENU:
		badgey_town_menu();
		break;
	case BADGEY_RUN:
		badgey_run();
		break;
	case BADGEY_EXIT_CONFIRM:
		badgey_exit_confirm();
		break;
	case BADGEY_EXIT:
		badgey_exit();
		break;
	case BADGEY_ENTER_TOWN_OR_CAVE:
		badgey_enter_town_or_cave();
		break;
	case BADGEY_TALK_TO_SHOPKEEPER:
		badgey_talk_to_shopkeeper();
		break;
	case BADGEY_TALK_TO_CITIZEN:
		badgey_talk_to_citizen();
		break;
	case BADGEY_STATUS_MESSAGE:
		badgey_status_message();
		break;
	case BADGEY_STATS:
		badgey_stats();
		break;
	case BADGEY_EQUIP_WEAPON:
	case BADGEY_EQUIP_ARMOR:
		badgey_equip();
		break;
	case BADGEY_COMBAT:
		badgey_combat();
		break;
	default:
		break;
	}
}

