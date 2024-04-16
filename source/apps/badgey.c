#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "colors.h"
#include "menu.h"
#include "button.h"
#include "framebuffer.h"
#include "bline.h"
#include "dynmenu.h"
#include "utils.h"

struct dynmenu planet_menu;
struct dynmenu_item planet_menu_item[5];

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
	"wwwwww.....ff..mmff.ff.....wwww...................wwwwwwwwwwwwww"
	"wwwwww......f..fmm..ff.....w...................wwwwwwwwwwwwwwwww"
	"wwwww..........ff.................................wwwwwwwwwwwwww"
	"wwwww...........f.............................wwwwwwwwwwwwwwwwww"
	"wwwwww..w................................wwwwwwwwwwwwwwwwwwwwwww"
	"wwwww...ww...................wwwwwwm..................wwwwwwwwww"
	"wwwwwwwww.................f.wwwwwmmm................wwwwww.wwwww"
	"wwwww..ww................fffwwwmmm................wwwwwww..wwwww"
	"wwwwww..www.............ffffffmm.....................w.....wwwww"
	"wwwww.....................fffffmmfff....................wwwwwwww"
	"wwwww......................fffffmffffff................wwwwwwwww"
	"www....fmf...................ffffffff..................wwwwwwwww"
	"www.....mmf.............fff....fffff......................wwwwww"
	"wwwww...fmmf....................fffff.................www.wwwwww"
	"wwwww...ffmfff....................fffff...............wwwwwwwwww"
	"wwwww...f...ffff........................................wwwwwwww"
	"wwwww.w......f.ff.......................................wwwwwwww"
	"wwwwwww........fff....................................w.wwww..ww"
	"wwwwwwww.......ffff...................................wwwww...ww"
	"wwwwww...........f..f.................................wwww....ww"
	"wwwww..............ff......................f...........ww.....ww"
	"wwwwww...........................mm..m....ff................wwww"
	"wwwww...............f........m..mm...mm...ff................wwww"
	"wwwww........................mmmm.....mmmff.............wwwwwwww"
	"wwwwww......................mmm........mmfffff........wwwwwwwwww"
	"wwwwww......................m...........mf..ff........wwwwwwwwww"
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
	"wwwww......w................mmm........................w.....www"
	"wwwwwww....w.............wwwwwmm..................w....www...www"
	"wwwwwwww...ww.......ffff....wwwwww................w.w..wwwwwwwww"
	"wwwwwwww..........ffff........wwwww.....m.......w.www...wwwwwwww"
	"wwwww...........ffffffff........www.....mm......www......wwwwwww"
	"wwwww.........fffffffffffffff..wwww......mmm......w......wwwwwww"
	"wwwwww....w..fffff..........wwwwwwwwww.....m.....wwwwwwwwwwwwwww"
	"wwwwwww..ww..........wwww...wwwwwwwwwwwwwwwww............wwwwwww"
	"wwwwwww.wwww.....wwwwwwwwww.wwwwwwwwwwwwwwwwwwwww.........wwwwww"
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
	"wwwwwww..........wwwwww.................wwwwwwwwwwwwwww......www"
	"wwwwwww........ffwwwwwww................wwww...wwwwwww......wwww"
	"wwwwww........ff.wwwwwww...............www.....wwwwww.....w.wwww"
	"wwwww..mmm....ff....wwww..............www......wwwww..ww..wwwwww"
	"wwwwww....m...ff.....w................ww.......wwwwwwww...wwwwww"
	"wwwwwwwww.....fff....www.......f.........ff...wwwwwwwwwwwwwwwwww"
	"wwwwwwwww.....f..f...w.w......fff.......fff...wwwwww.wwwwwwwwwww"
	"wwwwwwww........f....w.........ff......ffff..wwwww....wwwwwwwwww"
	"wwwwwww........................fff.....fff............wwwwwwwwww"
	"wwwww............f..............ffff....f.............wwwwwwwwww"
	"www..............................ffff...f............wwwwwwwwwww"
	"www..............................ffff.............wwwwwwwwwwwwww"
	"www....ww.........................f.ff............wwwwwwwwwwwwww"
	"www..wwwww..........................ffff...........wwwwwww...www"
	"wwwwwwwwww...........................fff...........wwwwww.....ww"
	"wwwwwwww.............................................ww.......ww"
	"wwwwww.....................................................ww.ww"
	"wwwww...................................fffff.............wwwwww"
	"wwwwww............mm...................fmfff............wwwwwwww"
	"wwwwww.f...fffff...mm................fffmf.........ww..wwwwwwwww"
	"wwwwww.ff....fffff..mm.................fmfff......wwwwwwwwwwwwww"
	"wwwww..fff......fffffm................fmmff.......wwwwwwwwwwwwww"
	"wwwww..fffff......ffmm..............fffmf..........wwwwwwwwwwwww"
	"wwwww..ff.fff....ffmm................ffmff.........wwwwwwwwwwwww"
	"wwwww...f..ffffffffmm...................ff.........w.wwwwww..www"
	"wwwww........ff.fmmm....................ff.............wwww..www"
	"wwwwwwww..m..............................f......w........www..ww"
	"wwwwwwwww.mmm.................................w.w.........w...ww"
	"wwwwwwwwwww....w............ffffff............wwwww..........www"
	"wwwwww.wwwwwwwww..........fffffffmfff.............w..ww.....wwww"
	"wwwwww.......w.........ffffffffffmmmfff...........wwwwwwwwwwwwww"
	"wwwwww.......w.............fffffffmmmmfffff.......wwwwwwwwwwwwww"
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
	"wwwwww.....www........mmmfffff......mmmf................wwwwwwww"
	"wwwwww..wwwwww......mmm..............mmmff..............wwwwwwww"
	"wwwwwww.wwwwwwf......ffff..............fff.................wwwww"
	"wwwwwwwwwwwwwwff......fffff........wwfff..w................wwwww"
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
	"www.....................ff....w..............mmmff........wwwwww"
	"ww...................f...f....www............wwmfff...wwwwwwwwww"
	"www....w......................w............mmffmf.........wwwwww"
	"wwwwwwwww.....................w..........mffff...........wwwwwww"
	"wwwwwwwwww..............................ffff..f............wwwww"
	"wwwwwwwwww.......ff........................ff.f............wwwww"
	"wwwwwwwwwww.....fff........................ff............wwwwwww"
	"wwwwwwwwwww......f..........................f..........wwwwwwwww"
	"wwwwwww..............................................wwwwwwwwwww"
	"www.................................................wwwwwwwwwwww"
	"wwwwwwwww...........................................wwwww...wwww"
	"wwwwwwwwwww............fff....................mm.....wwwww....ww"
	"wwww...wwww.............fff....................mm....ww.....wwww"
	"wwwww..www......fff.........................m...m..........wwwww"
	"wwww..............fff.......fff............mmm...........wwwwwww"
	"wwww...............ffff...fff...................w....wwwwwwwwwww"
	"wwww................f.fff..fffff................w...wwwwwwwwwwww"
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
	"wwww......ww.............mm.......m............r........wwwwwwww"
	"www.....wwww..............mm......m....................wwwwwwwww"
	"wwww...wwwww...............mm.fff....................wwwwwwwwwww"
	"wwwwwwwwwww.................fff......................wwwwwwwwwww"
	"wwwwwwwww..........f.................................wwwwwwwwwww"
	"wwwwww......fff....................................wwwwwwwwwwwww"
	"wwwwwwwww..fff..fff.................www.............wwwww...wwww"
	"wwwwwwww..........fff.fff.f.........www......mm......ww.....wwww"
	"wwwwwwwwww......w...fff..............wwww.....mm..........wwwwww"
	"wwwwwwwwwwww...www....fffffffff.........w......mm.......wwwwwwww"
	"wwwwwwwwwwww...ww.......fff.............wwww.........wwwwwwwwwww"
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
	"wwwwwwwwwww.............................w............wwwwwwwwwww"
	"wwwwwwwwwwwww..........................................wwwwwwwww"
	"wwwwwwwwww.......fffff.........mmmff.....................wwwwwww"
	"wwwww....ww..m......fffff..fffmmmwfffff..............wwwwwwwwwww"
	"wwwwwwww..w.mm..........fffffm........fffff......wwwwwwwwwwwwwww"
	"wwwwwww......m..f........ffffffffff...............w..wwwwww.wwww"
	"wwwwwwww........ff..ffff....fffffffff............ww..wwww...wwww"
	"wwwwwwwwwww......fffff.........fffffffffffffffff.....ww.w...wwww"
	"wwwwwwwwwww...fffff..f..............fffffffff..............wwwww"
	"wwwwwwwww..fffff....f..................fffff..............wwwwww"
	"www.......................................................wwwwww"
	"wwwww................................f...................wwwwwww"
	"wwwwwwww...ww................fffm.fffff..................wwwwwww"
	"wwwwwwwwwwww................mmmfffff.................w..wwwwwwww"
	"wwwwwwwww...............fffmfffffwfffff..............w.wwwwwwwww"
	"wwwwww.................fffffff.....fffff......www....wwwwwwwwwww"
	"wwwwww...mm.....fff..fffffww..........f.........wwwwwwwwwwwwwwww"
	"wwwwwww..m.......ffffff.fffff..fffff...........ww...wwwwwwwwwwww"
	"wwwwwww........fffff.......fffff...fffff......ww.....wwwwwwwwwww"
	"wwwwwwwww...fffff................f.ff................ww.wwwwwwww"
	"wwwwwwwwww................................................wwwwww"
	"wwwwwwwwww............................................w...wwwwww"
	"wwwwwwww.............................................wwwwwwwwwww"
	"wwwwwwww...................mmmm......................wwwwwwwwwww"
	"wwwwwww.................mmmmff.......................wwwwwwwwwww"
	"wwwwww...............mmmmm.fff.........................wwwwwwwww"
	"wwww...............................fffff...................wwwww"
	"wwwwwww.....ww..................fffff..........m.............www"
	"wwwwwwwwwwwww................fffff............mm........ww...www"
	"wwwwwwwww...wwww.............................mm.........www.wwww"
	"wwwwwwwwww...................................m.........wwwwwwwww"
	"wwwwwwwwww...................................m.........wwwwwwwww"
	"wwwwwwwwww..............mfffff...........................ww.wwww"
	"wwwwww................mmm.fffff.............................wwww"
	"wwww..................mfffff................................wwww"
	"wwwwww...............mmfffff.f.......................w......wwww"
	"wwww...............fffff...............m.............wwww.wwwwww"
	"wwwwwwww............fffff..f.........m..........fff..wwwwwwwwwww"
	"wwwwww...........fffff..............mm...........fff.wwwwwwwwwww"
	"wwwwww..................f..........mm..........fff...wwwwwwwwwww"
	"wwwwwww.............f..............mm...........fff...wwwwwwwwww"
	"wwwwwww...........................mm.........fff......wwwwwwwwww"
	"wwwwwwwwww........................m...........fff......wwwwwwwww"
	"wwwwwwwww........ffffff..........mm........ffff.........wwwwwwww"
	"wwwwwwwwww...........ffffff......m.....fff.............wwwwwwwww"
	"wwwwwwwww........ffffff..................fff.......m.......wwwww"
	"wwwwwwww......ffffff....................ffff.....mm.........wwww"
	"wwwwwwww..............................fff........m.m.....wwwwwww"
	"wwwwww..............................fff........mm.......wwwwwwww"
	"wwwwwww..........w............ww.....................wwwwwwwwwww"
	"wwwww....m......ww.....mm......w.............w.......wwwwwwwwwww"
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
	"wwwwwwww..........w....mm.ff.........w.......mm..........wwwwwww"
	"wwwwwwwwwww.............mmf...................m.........wwwwwwww"
	"wwww.....www........ff...mm........................wwwwwwwwwwwww"
	"wwwww......w.........f................ww........wwwwwwwwwwww.www"
	"wwwwww...............ff...............w.......wwwww.....www..www"
	"wwwwwwww..............f......ffff....ww......wwww..f.....ww..www"
	"wwwwwwwww......................ff.....www...www....fff....f..www"
	"wwwwwwwww..........ff..................wwwwwwww......f...fffwwww"
	"wwwwwwww............fffff...............wwwwww......ff...f...www"
	"wwwwwwww.............fffff.......www..wwwwww........f....f....ww"
	"www....w................fffff......wwwww.............f..ff.w..ww"
	"wwww.....................fffff........www...............fffw..ww"
	"wwwwwww...ff................fffff.......www................ww.ww"
	"wwwwwwwww..ffmm...............fffff......wwwf.............wwwwww"
	"wwwwwwwww..fffmm...............fffff......wfff...........wwwwwww"
	"wwwwwwww....fffmmmmm..........fffff.......w.ff...........wwwwwww"
	"wwwwwwww....fffmm...m..........fffff.....ww.ffm...........wwwwww"
	"wwwwwww......fffm..............fffff........ff.mm.........wwwwww"
	"wwwww........fffm.............fffff..........ff.mmww......wwwwww"
	"ww....ww......ffm...............fffff.........f...www.....wwwwww"
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
	"wwwwwww..........................mff......................wwwwww"
	"wwwwwwww.........................mmff........fff..........wwwwww"
	"wwwwwwww..........................m...........fff..........wwwww"
	"wwwwwwww..........................mff.............m........wwwww"
	"wwwwww............................mff..........fffmm.......wwwww"
	"www................................mff..........fffmm......wwwww"
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
	"www....w......mmfff......w..............ww...fff....ww...wwwwwww"
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
	.landingx = 32,
	.landingy = 32,
};

static const struct badgey_world NW42 = {
	.name = "NW42",
	.type = WORLD_TYPE_PLANET,
	.wm = NW42_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
};

static const struct badgey_world borton = {
	.name = "BORTON",
	.type = WORLD_TYPE_PLANET,
	.wm = borton_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
};


static const struct badgey_world skang = {
	.name = "SKANG",
	.type = WORLD_TYPE_PLANET,
	.wm = skang_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
};


static const struct badgey_world gnarg = {
	.name = "BORTON",
	.type = WORLD_TYPE_PLANET,
	.wm = gnarg_map,
	.subworld = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
};

static const struct badgey_world space = {
	.name = "SPACE",
	.type = WORLD_TYPE_SPACE,
	.wm = space_map,
	.subworld = { &ossaria, &NW42, &borton, &skang, &gnarg, NULL, NULL, NULL, NULL, NULL },
	.landingx = 32,
	.landingy = 32,
};

static struct player {
	struct badgey_world const *world; /* current world */
	int x, y; /* coords in current world */
	int world_level;
	struct badgey_world const *old_world[5];
	int wx[5], wy[5]; /* coords at each level */
} player = {
	.world = &space,
	.x = 32,
	.y = 32,
	.world_level = 0,
	.old_world = { 0 },
	.wx = { 0 },
	.wy = { 0 },
};

/* Program states.  Initial state is BADGEY_INIT */
enum badgey_state_t {
	BADGEY_INIT,
	BADGEY_PLANET_MENU,
	BADGEY_RUN,
	BADGEY_EXIT,
};

static enum badgey_state_t badgey_state = BADGEY_INIT;
static int screen_changed = 0;

static void badgey_init(void)
{
	FbInit();
	FbClear();
	badgey_state = BADGEY_RUN;
	screen_changed = 1;
}

static void check_buttons(void)
{
	int newx, newy;

	newx = player.x;
	newy = player.y;

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_LEFT, down_latches)) {
		newx--;
		if (newx < 0)
			newx += 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_RIGHT, down_latches)) {
		newx++;
		if (newx > 63)
			newx -= 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		newy--;
		if (newy < 0)
			newy += 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		newy++;
		if (newy > 63)
			newy -= 64;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		if (player.world->type == WORLD_TYPE_PLANET)
			badgey_state = BADGEY_PLANET_MENU;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_B, down_latches)) {
		badgey_state = BADGEY_EXIT;
	}
	int c = player.world->wm[windex(newx, newy)];
	if (player.world->type == WORLD_TYPE_SPACE) {
		/* Prevent player from driving into a star */
		if (c == '*') {
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
				player.y = new_world->landingx;
				player.world = new_world;
				screen_changed = 1;
				return;
			}
		}
	} else {
		/* Prevent player from traversing water or mountains */
		if (player.world->wm[windex(newx, newy)] == 'w' ||
			player.world->wm[windex(newx, newy)] == 'm') {
			return;
		}
	}
	if (newx != player.x || newy != player.y) {
		player.x = newx;
		player.y = newy;
		player.wx[player.world_level] = newx;
		player.wy[player.world_level] = newy;
		screen_changed = 1;
	}
}

/* Draw a cell of the world at screen coords (x, y) */
static void draw_cell(int x, int y, unsigned char c)
{
	FbMove(x, y);
	switch (c) {
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
		if (player.world->type == WORLD_TYPE_SPACE) { /* player is in space?  it's a planet */
			FbCharacter((unsigned char) 'O');
		} else {
			/* Not in space, so ... */
			if ((c - '0') % 2 == 0)
				FbCharacter((unsigned char) 'T'); /* ... it's a town ... */
			else
				FbCharacter((unsigned char) 'C'); /* ... or it's a cave. */
		}
		break;
	case '*':
		FbCharacter(c);
		break;
	case ' ': /* empty space, no need to draw anything */
		break;
	case '.':
		FbColor(GREEN);
		FbCharacter(c);
		break;
	case 'w':
		FbColor(BLUE);
		FbCharacter(c);
		break;
	case 'f':
		FbColor(x11_olive_drab);
		FbCharacter(c);
		break;
	case 'm':
		FbColor(x11_gray);
		FbCharacter(c);
		break;
	case 'B':
		FbColor(WHITE);
		FbCharacter(c);
		break;
	default:
		FbCharacter((unsigned char) '?'); /* unknown */
		break;
	}
}

struct visibility_check_data {
	int px, py, dx, dy, answer;
};

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
	if (c == 'm' || c == 'f') {/* mountains and forest block visibility */
		d->answer = 0;
		return 1;
	}
	return 0;
}

static int visibility_check(int px, int py, int x, int y)
{
	struct visibility_check_data d = { px, py, x, y, 1 };
	(void) bline(px, py, x, y, visibility_evaluator, &d);
	return d.answer;
}

static void draw_screen(void)
{
	if (!screen_changed)
		return;
	FbClear();
	FbColor(WHITE);

	int x, y, sx, sy, rx, ry;

	x = player.x - 3;
	rx = x;
	if (x < 0)
		x += 64;
	y = player.y - 4;
	ry = y;
	if (y < 0)
		y += 64;
	sx = 0;
	sy = 0;
	int count = 0;
	do {
		unsigned char c = (unsigned char) player.world->wm[windex(x, y)];
		if (player.world->type == WORLD_TYPE_SPACE || visibility_check(player.x, player.y, rx, ry))
			draw_cell(sx, sy, c);
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
			sx = 0;
			y++;
			ry++;
			sy += 16;
			if (y > 63) {
				y = y - 64;
			}
		}
	} while (1);

	draw_cell(16 * 3, 16 * 4, 'B');

	char buf[20];
	snprintf(buf, sizeof(buf), "(%d, %d)", player.x, player.y);
	FbMove(0, 152);
	FbWriteString(buf);

	screen_changed = 0;
	FbPushBuffer();
}

static void badgey_planet_menu(void)
{
	static int local_screen_changed = 1;
	static int menu_setup = 0;

	if (!menu_setup) {
		dynmenu_clear(&planet_menu);
		dynmenu_init(&planet_menu, planet_menu_item, ARRAY_SIZE(planet_menu_item));
		strcpy(planet_menu.title, "");
		dynmenu_add_item(&planet_menu, "BLAST_OFF", BADGEY_RUN, 0);
		dynmenu_add_item(&planet_menu, "QUIT", BADGEY_EXIT, 1);
		menu_setup = 1;
	}

	if (local_screen_changed)
		dynmenu_draw(&planet_menu);

	int down_latches = button_down_latches();
	if (BUTTON_PRESSED(BADGE_BUTTON_UP, down_latches)) {
		dynmenu_change_current_selection(&planet_menu, -1);
		local_screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_DOWN, down_latches)) {
		dynmenu_change_current_selection(&planet_menu, 1);
		local_screen_changed = 1;
	} else if (BUTTON_PRESSED(BADGE_BUTTON_A, down_latches)) {
		switch (planet_menu.current_item) {
		case 0: /* blast off */
			if (player.world->type == WORLD_TYPE_PLANET && player.world_level > 0) {
				struct badgey_world const *old_world = player.old_world[player.world_level];
				if (old_world) {
					player.world = old_world;
					player.world_level--;
					player.x = player.wx[player.world_level];
					player.y = player.wy[player.world_level];
					badgey_state = BADGEY_RUN;
					/* global */ screen_changed = 1;
					local_screen_changed = 1;
				}
			}
			break;
		case 1: /* quit */
			local_screen_changed = 1;
			/* global */ screen_changed = 1;
			badgey_state = BADGEY_EXIT;
			break;
			
		}
	}
	if (local_screen_changed)
		FbSwapBuffers();
}

static void badgey_run(void)
{
	check_buttons();
	draw_screen();
}

static void badgey_exit(void)
{
	badgey_state = BADGEY_INIT; /* So that when we start again, we do not immediately exit */
	returnToMenus();
}

/* You will need to rename badgey_cb() something else. */
void badgey_cb(__attribute__((unused)) struct menu_t *m)
{
	switch (badgey_state) {
	case BADGEY_INIT:
		badgey_init();
		break;
	case BADGEY_PLANET_MENU:
		badgey_planet_menu();
		break;
	case BADGEY_RUN:
		badgey_run();
		break;
	case BADGEY_EXIT:
		badgey_exit();
		break;
	default:
		break;
	}
}
