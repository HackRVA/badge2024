#ifdef DRBOB_LORES
   #define DRBOB_BITS PICTURE2BIT
   #define DRBOB_WIDTH drbob_width
   #define DRBOB_HEIGHT drbob_height
   #define DRBOB_CMAP drbob_data_cmap
   #define DRBOB_DATA drbob_data
   #include "assets/lores/drbob.h"
#else
   #define DRBOB_BITS PICTURE8BIT
   #define DRBOB_WIDTH drbob_width
   #define DRBOB_HEIGHT drbob_height
   #define DRBOB_CMAP drbob_data_cmap
   #define DRBOB_DATA drbob_data
   #include "assets/hires/drbob.h"
#endif

