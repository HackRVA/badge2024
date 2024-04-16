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
#include "assetList.h"

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
			FbImage2(&planet, 0);
		} else {
			/* Not in space, so ... */
			if ((c - '0') % 2 == 0)
				FbCharacter((unsigned char) 'T'); /* ... it's a town ... */
			else
				FbCharacter((unsigned char) 'C'); /* ... or it's a cave. */
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
		FbImage2(&water, 0);
		break;
	case 'f':
		FbImage2(&tree, 0);
		break;
	case 'm':
		FbImage2(&mountain, 0);
		break;
	case 'B':
		FbImage2(&badgey, 0);
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

