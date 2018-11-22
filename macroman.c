#pragma lint -1
#pragma noroot

#include "macroman.h"

#define ONECHAR(x)                                                             \
  { 1, (char *)x }

#define MULTICHAR(x)                                                           \
  { sizeof(x) - 1, x }

#define BLANK                                                                  \
  { 0, (char *)0 }

struct charmap macroman[128] = {
    ONECHAR('A'), // 0x80
    ONECHAR('A'),
    ONECHAR('C'),
    ONECHAR('E'),
    ONECHAR('N'),
    ONECHAR('O'),
    ONECHAR('U'),
    ONECHAR('a'),
    ONECHAR('a'),
    ONECHAR('a'),
    ONECHAR('a'),
    ONECHAR('a'),
    ONECHAR('a'),
    ONECHAR('c'),
    ONECHAR('e'),
    ONECHAR('e'),

    ONECHAR('e'), // 0x90
    ONECHAR('e'),
    ONECHAR('i'),
    ONECHAR('i'),
    ONECHAR('i'),
    ONECHAR('i'),
    ONECHAR('n'),
    ONECHAR('o'),
    ONECHAR('o'),
    ONECHAR('o'),
    ONECHAR('o'),
    ONECHAR('o'),
    ONECHAR('u'),
    ONECHAR('u'),
    ONECHAR('u'),
    ONECHAR('u'),

    BLANK, // 0xA0
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    ONECHAR('*'),
    BLANK,
    BLANK,
    MULTICHAR("(r)"),
    MULTICHAR("(c)"),
    MULTICHAR("tm"),
    BLANK,
    BLANK,
    BLANK,
    MULTICHAR("AE"),
    ONECHAR('O'),

    BLANK, // 0xB0
    BLANK,
    MULTICHAR("<="),
    MULTICHAR(">="),
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    MULTICHAR("ae"),
    ONECHAR('o'),

    ONECHAR('?'), // 0xC0
    ONECHAR('!'),
    BLANK,
    BLANK,
    ONECHAR('f'),
    BLANK,
    BLANK,
    MULTICHAR("<<"),
    MULTICHAR(">>"),
    MULTICHAR("..."),
    ONECHAR(' '),
    ONECHAR('A'),
    ONECHAR('A'),
    ONECHAR('O'),
    MULTICHAR("OE"),
    MULTICHAR("oe"),

    MULTICHAR("--"), // 0xD0
    MULTICHAR("---"),
    ONECHAR('"'),
    ONECHAR('"'),
    ONECHAR('\''),
    ONECHAR('\''),
    BLANK,
    BLANK,
    ONECHAR('y'),
    ONECHAR('Y'),
    ONECHAR('/'),
    BLANK,
    BLANK,
    BLANK,
    MULTICHAR("fi"),
    MULTICHAR("fl"),

    BLANK, // 0xE0
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    ONECHAR('A'),
    ONECHAR('E'),
    ONECHAR('A'),
    ONECHAR('E'),
    ONECHAR('E'),
    ONECHAR('I'),
    ONECHAR('I'),
    ONECHAR('I'),
    ONECHAR('I'),
    ONECHAR('O'),
    ONECHAR('O'),

    ONECHAR('?'), // 0xF0
    ONECHAR('O'),
    ONECHAR('U'),
    ONECHAR('U'),
    ONECHAR('U'),
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,
    BLANK,

};
