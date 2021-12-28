#ifdef HACKRVA4BIT_LORES
   #define HACKRVA4_BITS PICTURE2BIT
   #define HACKRVA4_WIDTH lores_width
   #define HACKRVA4_HEIGHT lores_height
   #define HACKRVA4_CMAP lores_data_cmap
   #define HACKRVA4_DATA lores_data
   #include "assets/lores/lores.h"
#else
   #define HACKRVA4_BITS PICTURE4BIT
   #define HACKRVA4_WIDTH hackrva4_width
   #define HACKRVA4_HEIGHT hackrva4_height
   #define HACKRVA4_CMAP hackrva4_data_cmap
   #define HACKRVA4_DATA hackrva4_data
   #include "assets/hires/hackrva4bit.h"
#endif

