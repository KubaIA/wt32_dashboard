#ifndef LV_CONF_H
#define LV_CONF_H

#include <stdint.h>

/* Színmélység: RGB565 (16 bit) a hardverhez igazítva */
#define LV_COLOR_DEPTH 16

/* ARM és egyéb nem-Xtensa assembly optimalizációk tiltása */
#define LV_USE_NATIVE_HELIUM_ASM 0
#define LV_USE_DRAW_ARM2D_SYNC   0
#define LV_USE_DRAW_SW_ASM       0

/* Memóriakezelés: beépített belső memóriaallokátor */
#define LV_USE_STDLIB_MALLOC    LV_STDLIB_BUILTIN
#define LV_MEM_SIZE (128 * 1024U) /* 128 KB dinamikus pool */

/* Operációs rendszer: None (az Arduino loop-ból hívjuk) */
#define LV_USE_OS   LV_OS_NONE

/* Tick időzítés és frissítés */
#define LV_DEF_REFR_PERIOD  16 /* ~60 FPS */
#define LV_DPI_DEF 160

/* A projekted képernyőin használt Montserrat betűméretek */
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_18 1
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_28 1
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_36 1
#define LV_FONT_MONTSERRAT_40 1
#define LV_FONT_MONTSERRAT_48 1

#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Szükséges widget modulok engedélyezése */
#define LV_USE_BUTTON    1
#define LV_USE_LABEL     1
#define LV_USE_SLIDER    1
#define LV_USE_BAR       1
#define LV_USE_SWITCH    1
#define LV_USE_IMAGE     1

#endif /* LV_CONF_H */