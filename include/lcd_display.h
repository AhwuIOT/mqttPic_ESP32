// lcd_display.h
#ifndef LCD_DISPLAY_H
#define LCD_DISPLAY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void lcd_init(void);
void display_jpg_from_buf(const uint8_t *jpg_buf, size_t jpg_len);

#ifdef __cplusplus
}
#endif

#endif // LCD_DISPLAY_H
