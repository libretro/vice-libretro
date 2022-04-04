/* Commodore font set */

const unsigned char font_array[128*8] = {
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
0x10, 0x28, 0x44, 0x44, 0x7c, 0x44, 0x44, 0x00, /* A 1 */
0x78, 0x44, 0x44, 0x78, 0x44, 0x44, 0x78, 0x00, /* B */
0x38, 0x44, 0x40, 0x40, 0x40, 0x44, 0x38, 0x00, /* C */
0x78, 0x44, 0x44, 0x44, 0x44, 0x44, 0x78, 0x00, /* D */
0x00, 0x7C, 0x44, 0x44, 0x44, 0x7C, 0x00, 0x00, /* square 5 */
0x00, 0x7C, 0x28, 0x28, 0x28, 0x2C, 0x00, 0x00, /* pi 6 */
0x00, 0x7C, 0x38, 0x10, 0x00, 0x7C, 0x00, 0x00, /* insert 7 */
0x00, 0x10, 0x38, 0x7C, 0x00, 0x7C, 0x00, 0x00, /* eject 8 */
0x00, 0x38, 0x28, 0x10, 0x7C, 0x7C, 0x00, 0x00, /* joystick 9 */
0x00, 0x38, 0x54, 0x7C, 0x44, 0x38, 0x00, 0x00, /* mouse 10 */
0x00, 0xA0, 0xFE, 0xA0, 0x0A, 0xFE, 0x0A, 0x00, /* tab 11 */
0x00, 0x10, 0x28, 0x44, 0xEE, 0x28, 0x38, 0x00, /* shift 12 */
0x00, 0x0E, 0x1A, 0x2A, 0x4A, 0xFA, 0x8E, 0x00, /* right amiga 13 */
0x00, 0x0E, 0x1E, 0x2E, 0x4E, 0xFE, 0x8E, 0x00, /* left amiga 14 */
0xA8, 0x00, 0xA8, 0x00, 0xA8, 0x00, 0xE8, 0x00, /* numpad 15 */
0x02, 0x02, 0x12, 0x32, 0x7E, 0x30, 0x10, 0x00, /* return 16 */
0x00, 0xB8, 0xC4, 0xE2, 0x02, 0x44, 0x38, 0x00, /* reset 17 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x82, 0xFE, 0x00, /* space 18 */
0x00, 0x44, 0x4C, 0x5C, 0x4C, 0x44, 0x00, 0x00, /* datasette reset 19 */
0x00, 0x20, 0x30, 0x38, 0x30, 0x20, 0x00, 0x00, /* datasette play 20 */
0x00, 0x22, 0x66, 0xEE, 0x66, 0x22, 0x00, 0x00, /* datasette rwd 21 */
0x00, 0x88, 0xCC, 0xEE, 0xCC, 0x88, 0x00, 0x00, /* datasette fwd 22 */
0x00, 0x7C, 0x7C, 0x7C, 0x7C, 0x7C, 0x00, 0x00, /* datasette stop 23 */
0x30, 0x70, 0xC6, 0xC0, 0xC6, 0x70, 0x30, 0x00, /* commodore 24 */
0x00, 0x00, 0x20, 0x7C, 0x20, 0x00, 0x00, 0x00, /* left arrow 25 */
0x00, 0x10, 0x38, 0x10, 0x10, 0x10, 0x00, 0x00, /* up arrow 26 */
0x00, 0x10, 0x30, 0x7C, 0x30, 0x10, 0x00, 0x00, /* left cursor 27 */
0x00, 0x10, 0x10, 0x7C, 0x38, 0x10, 0x00, 0x00, /* down cursor 28 */
0x00, 0x10, 0x18, 0x7C, 0x18, 0x10, 0x00, 0x00, /* right cursor 29 */
0x00, 0x10, 0x38, 0x7C, 0x10, 0x10, 0x00, 0x00, /* up cursor 30 */
0x00, 0x18, 0x24, 0x70, 0x20, 0x7C, 0x00, 0x00, /* £ 31 */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, /* space 32 */
0x00, 0x10, 0x10, 0x10, 0x00, 0x10, 0x00, 0x00, /* ! */
0x00, 0x28, 0x28, 0x00, 0x00, 0x00, 0x00, 0x00, /* " */
0x00, 0x28, 0x7C, 0x28, 0x7C, 0x28, 0x00, 0x00, /* # */
0x10, 0x38, 0x30, 0x18, 0x38, 0x10, 0x00, 0x00, /* $ */
0x00, 0x64, 0x68, 0x10, 0x2C, 0x4C, 0x00, 0x00, /* % */
0x00, 0x10, 0x28, 0x10, 0x2C, 0x38, 0x00, 0x00, /* & */
0x00, 0x10, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, /* ' */
0x00, 0x10, 0x20, 0x20, 0x20, 0x10, 0x00, 0x00, /* ( */
0x00, 0x10, 0x08, 0x08, 0x08, 0x10, 0x00, 0x00, /* ) */
0x00, 0x10, 0x54, 0x38, 0x54, 0x10, 0x00, 0x00, /* * */
0x00, 0x10, 0x10, 0x7c, 0x10, 0x10, 0x00, 0x00, /* + */
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x20, 0x00, /* , */
0x00, 0x00, 0x00, 0x38, 0x00, 0x00, 0x00, 0x00, /* - */
0x00, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, /* . */
0x00, 0x04, 0x08, 0x10, 0x20, 0x40, 0x00, 0x00, /* / */
0x00, 0x7C, 0x44, 0x44, 0x44, 0x7C, 0x00, 0x00, /* 0 */
0x00, 0x30, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, /* 1 */
0x00, 0x7C, 0x04, 0x7C, 0x40, 0x7C, 0x00, 0x00, /* 2 */
0x00, 0x7C, 0x04, 0x7C, 0x04, 0x7C, 0x00, 0x00, /* 3 */
0x00, 0x44, 0x44, 0x7C, 0x04, 0x04, 0x00, 0x00, /* 4 */
0x00, 0x7C, 0x40, 0x7C, 0x04, 0x7C, 0x00, 0x00, /* 5 */
0x00, 0x7C, 0x40, 0x7C, 0x44, 0x7C, 0x00, 0x00, /* 6 */
0x00, 0x7C, 0x04, 0x08, 0x10, 0x10, 0x00, 0x00, /* 7 */
0x00, 0x7C, 0x44, 0x7C, 0x44, 0x7C, 0x00, 0x00, /* 8 */
0x00, 0x7C, 0x44, 0x7C, 0x04, 0x7C, 0x00, 0x00, /* 9 */
0x00, 0x00, 0x10, 0x00, 0x10, 0x00, 0x00, 0x00, /* : */
0x00, 0x00, 0x10, 0x00, 0x10, 0x20, 0x00, 0x00, /* ; */
0x00, 0x08, 0x10, 0x20, 0x10, 0x08, 0x00, 0x00, /* < */
0x00, 0x00, 0x38, 0x00, 0x38, 0x00, 0x00, 0x00, /* = */
0x00, 0x20, 0x10, 0x08, 0x10, 0x20, 0x00, 0x00, /* > */
0x00, 0x38, 0x04, 0x18, 0x00, 0x10, 0x00, 0x00, /* ? */
0x00, 0x38, 0x4C, 0x5C, 0x40, 0x3C, 0x00, 0x00, /* @ */
0x00, 0x38, 0x44, 0x7C, 0x44, 0x44, 0x00, 0x00, /* A 65 */
0x00, 0x78, 0x44, 0x78, 0x44, 0x78, 0x00, 0x00, /* B */
0x00, 0x3C, 0x40, 0x40, 0x40, 0x3C, 0x00, 0x00, /* C */
0x00, 0x78, 0x44, 0x44, 0x44, 0x78, 0x00, 0x00, /* D */
0x00, 0x7C, 0x40, 0x78, 0x40, 0x7C, 0x00, 0x00, /* E */
0x00, 0x7C, 0x40, 0x78, 0x40, 0x40, 0x00, 0x00, /* F */
0x00, 0x3C, 0x40, 0x4C, 0x44, 0x3C, 0x00, 0x00, /* G */
0x00, 0x44, 0x44, 0x7C, 0x44, 0x44, 0x00, 0x00, /* H */
0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, /* I */
0x00, 0x04, 0x04, 0x04, 0x44, 0x38, 0x00, 0x00, /* J */
0x00, 0x44, 0x48, 0x70, 0x48, 0x44, 0x00, 0x00, /* K */
0x00, 0x40, 0x40, 0x40, 0x40, 0x7C, 0x00, 0x00, /* L */
0x00, 0x28, 0x54, 0x54, 0x54, 0x54, 0x00, 0x00, /* M */
0x00, 0x44, 0x64, 0x54, 0x4C, 0x44, 0x00, 0x00, /* N */
0x00, 0x38, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, /* O */
0x00, 0x78, 0x44, 0x78, 0x40, 0x40, 0x00, 0x00, /* P */
0x00, 0x38, 0x44, 0x54, 0x48, 0x34, 0x00, 0x00, /* Q */
0x00, 0x78, 0x44, 0x78, 0x48, 0x44, 0x00, 0x00, /* R */
0x00, 0x3C, 0x40, 0x38, 0x04, 0x78, 0x00, 0x00, /* S */
0x00, 0x7C, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, /* T */
0x00, 0x44, 0x44, 0x44, 0x44, 0x38, 0x00, 0x00, /* U */
0x00, 0x44, 0x44, 0x44, 0x28, 0x10, 0x00, 0x00, /* V */
0x00, 0x44, 0x54, 0x54, 0x54, 0x28, 0x00, 0x00, /* W */
0x00, 0x44, 0x28, 0x10, 0x28, 0x44, 0x00, 0x00, /* X */
0x00, 0x44, 0x28, 0x10, 0x10, 0x10, 0x00, 0x00, /* Y */
0x00, 0x7C, 0x08, 0x10, 0x20, 0x7C, 0x00, 0x00, /* Z */
0x00, 0x30, 0x20, 0x20, 0x20, 0x30, 0x00, 0x00, /* [ */
0x00, 0x40, 0x20, 0x10, 0x08, 0x04, 0x00, 0x00, /* backslash */
0x00, 0x18, 0x08, 0x08, 0x08, 0x18, 0x00, 0x00, /* ] */
0x00, 0x10, 0x28, 0x44, 0x00, 0x00, 0x00, 0x00, /* ^ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7C, 0x00, /* _ */
0x00, 0x10, 0x08, 0x00, 0x00, 0x00, 0x00, 0x00, /* ` */
0x00, 0x00, 0x60, 0x10, 0x70, 0x70, 0x00, 0x00, /* a */
0x00, 0x40, 0x40, 0x60, 0x50, 0x60, 0x00, 0x00, /* b */
0x00, 0x00, 0x30, 0x40, 0x40, 0x30, 0x00, 0x00, /* c */
0x00, 0x10, 0x10, 0x30, 0x50, 0x30, 0x00, 0x00, /* d */
0x00, 0x00, 0x30, 0x70, 0x40, 0x30, 0x00, 0x00, /* e */
0x00, 0x30, 0x20, 0x70, 0x20, 0x20, 0x00, 0x00, /* f */
0x00, 0x00, 0x20, 0x50, 0x30, 0x10, 0x60, 0x00, /* g */
0x00, 0x40, 0x40, 0x60, 0x50, 0x50, 0x00, 0x00, /* h */
0x00, 0x20, 0x00, 0x20, 0x20, 0x20, 0x00, 0x00, /* i */
0x00, 0x20, 0x00, 0x20, 0x20, 0x20, 0x40, 0x00, /* j */
0x00, 0x40, 0x40, 0x50, 0x60, 0x50, 0x00, 0x00, /* k */
0x00, 0x20, 0x20, 0x20, 0x20, 0x20, 0x00, 0x00, /* l */
0x00, 0x00, 0x28, 0x54, 0x54, 0x54, 0x00, 0x00, /* m */
0x00, 0x00, 0x60, 0x50, 0x50, 0x50, 0x00, 0x00, /* n */
0x00, 0x00, 0x20, 0x50, 0x50, 0x20, 0x00, 0x00, /* o */
0x00, 0x00, 0x60, 0x50, 0x60, 0x40, 0x40, 0x00, /* p */
0x00, 0x00, 0x30, 0x50, 0x30, 0x10, 0x10, 0x00, /* q */
0x00, 0x00, 0x50, 0x60, 0x40, 0x40, 0x00, 0x00, /* r */
0x00, 0x00, 0x30, 0x60, 0x30, 0x60, 0x00, 0x00, /* s */
0x00, 0x20, 0x70, 0x20, 0x20, 0x30, 0x00, 0x00, /* t */
0x00, 0x00, 0x50, 0x50, 0x50, 0x70, 0x00, 0x00, /* u */
0x00, 0x00, 0x50, 0x50, 0x50, 0x20, 0x00, 0x00, /* v */
0x00, 0x00, 0x44, 0x54, 0x54, 0x28, 0x00, 0x00, /* w */
0x00, 0x00, 0x50, 0x20, 0x20, 0x50, 0x00, 0x00, /* x */
0x00, 0x00, 0x50, 0x50, 0x30, 0x10, 0x60, 0x00, /* y */
0x00, 0x00, 0x70, 0x20, 0x40, 0x70, 0x00, 0x00, /* z */
0x00, 0x30, 0x20, 0x60, 0x20, 0x30, 0x00, 0x00, /* { */
0x00, 0x10, 0x10, 0x10, 0x10, 0x10, 0x00, 0x00, /* | */
0x00, 0x18, 0x08, 0x0C, 0x08, 0x18, 0x00, 0x00, /* } */
0x00, 0x34, 0x58, 0x00, 0x00, 0x00, 0x00, 0x00, /* ~ */
0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};
