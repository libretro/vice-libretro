#include "libretro.h"

/* Having virtual CBM & virtual Ctrl defined causes said keys to not work.. */

static void libretro_keyboard()
{
    if (keyconvmap != NULL)
        keyboard_keyconvmap_free();
    keyboard_keyconvmap_alloc();
    keyboard_keyword_clear();

#if defined(__XCBM2__) || defined(__XCBM5x0__)
    kbd_lshiftrow   = 8;
    kbd_lshiftcol   = 4;
    kbd_rshiftrow   = 8;
    kbd_rshiftcol   = 4;
    key_ctrl_vshift = KEY_LSHIFT;

    keyboard_parse_set_pos_row(RETROK_ESCAPE,       8, 1, 8);           /*          Esc -> ESC          */
    keyboard_parse_set_pos_row(RETROK_1,            9, 1, 8);           /*            1 -> 1            */
    keyboard_parse_set_pos_row(RETROK_2,           10, 1, 8);           /*            2 -> 2            */
    keyboard_parse_set_pos_row(RETROK_3,           11, 1, 8);           /*            3 -> 3            */
    keyboard_parse_set_pos_row(RETROK_4,           12, 1, 8);           /*            4 -> 4            */
    keyboard_parse_set_pos_row(RETROK_5,           13, 1, 8);           /*            5 -> 5            */
    keyboard_parse_set_pos_row(RETROK_6,           13, 2, 8);           /*            6 -> 6            */
    keyboard_parse_set_pos_row(RETROK_7,           14, 1, 8);           /*            7 -> 7            */
    keyboard_parse_set_pos_row(RETROK_8,           15, 1, 8);           /*            8 -> 8            */
    keyboard_parse_set_pos_row(RETROK_9,            0, 1, 8);           /*            9 -> 9            */
    keyboard_parse_set_pos_row(RETROK_0,            1, 1, 8);           /*            0 -> 0            */
    keyboard_parse_set_pos_row(RETROK_MINUS,        1, 2, 8);           /*            - -> -            */
    keyboard_parse_set_pos_row(RETROK_EQUALS,       2, 1, 8);           /*            = -> =            */
    keyboard_parse_set_pos_row(RETROK_INSERT,       2, 2, 8);           /*          Ins -> Pound        */
    keyboard_parse_set_pos_row(RETROK_BACKSPACE,    3, 3, 8);           /*    Backspace -> DEL          */

    keyboard_parse_set_pos_row(RETROK_TAB,          8, 2, 8);           /*          Tab -> TAB          */
    keyboard_parse_set_pos_row(RETROK_q,            9, 2, 8);           /*            Q -> Q            */
    keyboard_parse_set_pos_row(RETROK_w,           10, 2, 8);           /*            W -> W            */
    keyboard_parse_set_pos_row(RETROK_e,           11, 2, 8);           /*            E -> E            */
    keyboard_parse_set_pos_row(RETROK_r,           12, 2, 8);           /*            R -> R            */
    keyboard_parse_set_pos_row(RETROK_t,           12, 3, 8);           /*            T -> T            */
    keyboard_parse_set_pos_row(RETROK_y,           13, 3, 8);           /*            Y -> Y            */
    keyboard_parse_set_pos_row(RETROK_u,           14, 2, 8);           /*            U -> U            */
    keyboard_parse_set_pos_row(RETROK_i,           15, 2, 8);           /*            I -> I            */
    keyboard_parse_set_pos_row(RETROK_o,            0, 2, 8);           /*            O -> O            */
    keyboard_parse_set_pos_row(RETROK_p,            1, 3, 8);           /*            P -> P            */
    keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  1, 4, 8);           /*            [ -> [            */
    keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 2, 3, 8);           /*            ] -> ]            */
    keyboard_parse_set_pos_row(RETROK_RETURN,       2, 4, 8);           /*       Return -> RETURN       */

    keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     8, 4, 64);          /*    Caps Lock -> Shift Lock   */
    keyboard_parse_set_pos_row(RETROK_a,            9, 3, 8);           /*            A -> A            */
    keyboard_parse_set_pos_row(RETROK_s,           10, 3, 8);           /*            S -> S            */
    keyboard_parse_set_pos_row(RETROK_d,           11, 3, 8);           /*            D -> D            */
    keyboard_parse_set_pos_row(RETROK_f,           11, 4, 8);           /*            F -> F            */
    keyboard_parse_set_pos_row(RETROK_g,           12, 4, 8);           /*            G -> G            */
    keyboard_parse_set_pos_row(RETROK_h,           13, 4, 8);           /*            H -> H            */
    keyboard_parse_set_pos_row(RETROK_j,           14, 3, 8);           /*            J -> J            */
    keyboard_parse_set_pos_row(RETROK_k,           15, 3, 8);           /*            K -> K            */
    keyboard_parse_set_pos_row(RETROK_l,            0, 3, 8);           /*            L -> L            */
    keyboard_parse_set_pos_row(RETROK_SEMICOLON,    0, 4, 8);           /*            ; -> ;            */
    keyboard_parse_set_pos_row(RETROK_QUOTE,        1, 5, 8);           /*            ' -> '            */
    keyboard_parse_set_pos_row(RETROK_BACKSLASH,    2, 5, 8);           /*            \ -> Pi           */

    keyboard_parse_set_pos_row(RETROK_LSHIFT,       8, 4, 2);           /*   Left Shift -> Left Shift   */
    keyboard_parse_set_pos_row(RETROK_z,            9, 4, 8);           /*            Z -> Z            */
    keyboard_parse_set_pos_row(RETROK_x,           10, 4, 8);           /*            X -> X            */
    keyboard_parse_set_pos_row(RETROK_c,           10, 5, 8);           /*            C -> C            */
    keyboard_parse_set_pos_row(RETROK_v,           11, 5, 8);           /*            V -> V            */
    keyboard_parse_set_pos_row(RETROK_b,           12, 5, 8);           /*            B -> B            */
    keyboard_parse_set_pos_row(RETROK_n,           13, 5, 8);           /*            N -> N            */
    keyboard_parse_set_pos_row(RETROK_m,           14, 4, 8);           /*            M -> M            */
    keyboard_parse_set_pos_row(RETROK_COMMA,       15, 4, 8);           /*            , -> ,            */
    keyboard_parse_set_pos_row(RETROK_PERIOD,      15, 5, 8);           /*            . -> .            */
    keyboard_parse_set_pos_row(RETROK_OEM_102,     15, 4, 33);          /*            < -> <            */
    keyboard_parse_set_pos_row(RETROK_OEM_102,     15, 5, 145);         /*            > -> >            */
    keyboard_parse_set_pos_row(RETROK_SLASH,        0, 5, 8);           /*            / -> /            */
    keyboard_parse_set_pos_row(RETROK_RSHIFT,       8, 4, 2);           /*  Right Shift -> Right Shift  */
    keyboard_parse_set_pos_row(RETROK_RCTRL,        3, 4, 8);           /*   Right Ctrl -> C=           */

    keyboard_parse_set_pos_row(RETROK_LCTRL,        8, 5, 8);           /*    Left Ctrl -> CTRL         */
    keyboard_parse_set_pos_row(RETROK_SPACE,       14, 5, 8);           /*        Space -> Space        */

    keyboard_parse_set_pos_row(RETROK_F1,           8, 0, 8);           /*           F1 -> F1           */
    keyboard_parse_set_pos_row(RETROK_F2,           9, 0, 8);           /*           F2 -> F2           */
    keyboard_parse_set_pos_row(RETROK_F3,          10, 0, 8);           /*           F3 -> F3           */
    keyboard_parse_set_pos_row(RETROK_F4,          11, 0, 8);           /*           F4 -> F4           */
    keyboard_parse_set_pos_row(RETROK_F5,          12, 0, 8);           /*           F5 -> F5           */
    keyboard_parse_set_pos_row(RETROK_F6,          13, 0, 8);           /*           F6 -> F6           */
    keyboard_parse_set_pos_row(RETROK_F7,          14, 0, 8);           /*           F7 -> F7           */
    keyboard_parse_set_pos_row(RETROK_F8,          15, 0, 8);           /*           F8 -> F8           */
    keyboard_parse_set_pos_row(RETROK_F9,           0, 0, 8);           /*           F9 -> F9           */
    keyboard_parse_set_pos_row(RETROK_F10,          1, 0, 8);           /*          F10 -> F10          */

    keyboard_parse_set_pos_row(RETROK_UP,           3, 0, 8);           /*           Up -> CRSR UP      */
    keyboard_parse_set_pos_row(RETROK_DOWN,         2, 0, 8);           /*         Down -> CRSR DOWN    */
    keyboard_parse_set_pos_row(RETROK_LEFT,         3, 1, 8);           /*         Left -> CRSR LEFT    */
    keyboard_parse_set_pos_row(RETROK_RIGHT,        3, 2, 8);           /*        Right -> CRSR RIGHT   */

    keyboard_parse_set_pos_row(RETROK_HOME,         4, 0, 8);           /*         Home -> CLR/HOME     */
    keyboard_parse_set_pos_row(RETROK_PAGEUP,       5, 0, 8);           /*         PgUp -> OFF/RVS      */
    keyboard_parse_set_pos_row(RETROK_PAGEDOWN,     6, 0, 8);           /*       PgDown -> NORM/GRAPH   */
    keyboard_parse_set_pos_row(RETROK_LALT,         7, 0, 0);           /*     Left Alt -> RUN/STOP     */

    keyboard_parse_set_pos_row(RETROK_DELETE,       4, 1, 8);           /*          Del -> Numpad ?     */
    keyboard_parse_set_pos_row(RETROK_END,          5, 1, 8);           /*          End -> Numpad CE    */
    keyboard_parse_set_pos_row(RETROK_KP_MULTIPLY,  6, 1, 8);           /*     Numpad * -> Numpad *     */
    keyboard_parse_set_pos_row(RETROK_KP_DIVIDE,    7, 1, 8);           /*     Numpad / -> Numpad /     */
    keyboard_parse_set_pos_row(RETROK_KP7,          4, 2, 8);           /*     Numpad 7 -> Numpad 7     */
    keyboard_parse_set_pos_row(RETROK_KP8,          5, 2, 8);           /*     Numpad 8 -> Numpad 8     */
    keyboard_parse_set_pos_row(RETROK_KP9,          6, 2, 8);           /*     Numpad 9 -> Numpad 9     */
    keyboard_parse_set_pos_row(RETROK_KP_MINUS,     7, 2, 8);           /*     Numpad - -> Numpad -     */
    keyboard_parse_set_pos_row(RETROK_KP4,          4, 3, 8);           /*     Numpad 4 -> Numpad 4     */
    keyboard_parse_set_pos_row(RETROK_KP5,          5, 3, 8);           /*     Numpad 5 -> Numpad 5     */
    keyboard_parse_set_pos_row(RETROK_KP6,          6, 3, 8);           /*     Numpad 6 -> Numpad 6     */
    keyboard_parse_set_pos_row(RETROK_KP_PLUS,      7, 3, 8);           /*     Numpad + -> Numpad +     */
    keyboard_parse_set_pos_row(RETROK_KP1,          4, 4, 8);           /*     Numpad 1 -> Numpad 1     */
    keyboard_parse_set_pos_row(RETROK_KP2,          5, 4, 8);           /*     Numpad 2 -> Numpad 2     */
    keyboard_parse_set_pos_row(RETROK_KP3,          6, 4, 8);           /*     Numpad 3 -> Numpad 3     */
    keyboard_parse_set_pos_row(RETROK_KP0,          4, 5, 8);           /*     Numpad 0 -> Numpad 0     */
    keyboard_parse_set_pos_row(RETROK_KP_PERIOD,    5, 5, 8);           /*     Numpad . -> Numpad .     */
    keyboard_parse_set_pos_row(RETROK_KP_ENTER,     7, 4, 8);           /* Numpad Enter -> Numpad Enter */

#elif defined(__XPET__)
    switch (machine_keyboard_type)
    {
        case 0: /* Business (US) */
            kbd_lshiftrow   = 6;
            kbd_lshiftcol   = 0;
            kbd_rshiftrow   = 6;
            kbd_rshiftcol   = 6;
            key_ctrl_vshift = KEY_RSHIFT;
            key_ctrl_shiftl = KEY_LSHIFT;

            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    9, 0, 8);   /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_1,            1, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            0, 0, 8);   /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_3,            9, 1, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 1, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_5,            0, 1, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            9, 2, 8);   /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_7,            1, 2, 8);   /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_8,            0, 2, 8);   /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_9,            9, 3, 8);   /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_0,            1, 3, 8);   /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        9, 5, 8);   /*            - -> :            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       0, 3, 8);   /*            = -> -            */
            keyboard_parse_set_pos_row(RETROK_DELETE,       1, 5, 8);   /*          Del -> Up arrow     */
            keyboard_parse_set_pos_row(RETROK_RCTRL,        7, 6, 8);   /*   Right Ctrl -> RUN/STOP     */

            keyboard_parse_set_pos_row(RETROK_TAB,          4, 0, 8);   /*          Tab -> TAB          */
            keyboard_parse_set_pos_row(RETROK_q,            5, 0, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            4, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            5, 1, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            4, 2, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            5, 2, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            4, 3, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            5, 3, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 5, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            5, 5, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            4, 6, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 6, 8);   /*            [ -> [            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 2, 4, 8);   /*            ] -> ]            */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    4, 7, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       2, 0, 8);   /*          Esc -> ESC          */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     6, 0, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            3, 0, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            2, 1, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            3, 1, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 2, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            2, 3, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            3, 3, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            2, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            3, 5, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    2, 6, 8);   /*            ; -> ;            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        3, 6, 8);   /*            ' -> @            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       3, 4, 8);   /*       Return -> RETURN       */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        8, 0, 8);   /*    Left Ctrl -> OFF/RVS      */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       6, 0, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    4, 4, 8);   /*            \ -> \            */
            keyboard_parse_set_pos_row(RETROK_z,            7, 0, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            8, 1, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            6, 1, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            7, 1, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            6, 2, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            7, 2, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            8, 3, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        7, 3, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       6, 3, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        8, 6, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 6, 4);   /*  Right Shift -> Right Shift  */

            keyboard_parse_set_pos_row(RETROK_SPACE,        8, 2, 8);   /*        Space -> Space        */
            keyboard_parse_set_pos_row(RETROK_HOME,         8, 4, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_INSERT,       9, 4, 8);   /*          Ins -> STOP         */

            keyboard_parse_set_pos_row(RETROK_UP,           5, 4, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         5, 4, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         0, 5, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        0, 5, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_KP7,          1, 4, 8);   /*     Numpad 7 -> Numpad 7     */
            keyboard_parse_set_pos_row(RETROK_KP8,          0, 4, 8);   /*     Numpad 8 -> Numpad 8     */
            keyboard_parse_set_pos_row(RETROK_KP9,          1, 7, 8);   /*     Numpad 9 -> Numpad 9     */
            keyboard_parse_set_pos_row(RETROK_KP4,          5, 7, 8);   /*     Numpad 4 -> Numpad 4     */
            keyboard_parse_set_pos_row(RETROK_KP5,          2, 7, 8);   /*     Numpad 5 -> Numpad 5     */
            keyboard_parse_set_pos_row(RETROK_KP6,          3, 7, 8);   /*     Numpad 6 -> Numpad 6     */
            keyboard_parse_set_pos_row(RETROK_KP1,          8, 7, 8);   /*     Numpad 1 -> Numpad 1     */
            keyboard_parse_set_pos_row(RETROK_KP2,          7, 7, 8);   /*     Numpad 2 -> Numpad 2     */
            keyboard_parse_set_pos_row(RETROK_KP3,          6, 7, 8);   /*     Numpad 3 -> Numpad 3     */
            keyboard_parse_set_pos_row(RETROK_KP0,          7, 4, 8);   /*     Numpad 0 -> Numpad 0     */
            keyboard_parse_set_pos_row(RETROK_KP_PERIOD,    6, 4, 8);   /*     Numpad . -> Numpad .     */
            break;

        case 4: /* Graphics (US) */
            kbd_lshiftrow   = 8;
            kbd_lshiftcol   = 0;
            kbd_rshiftrow   = 8;
            kbd_rshiftcol   = 5;
            key_ctrl_vshift = KEY_RSHIFT;
            key_ctrl_shiftl = KEY_LSHIFT;

            keyboard_parse_set_pos_row(RETROK_1,            0, 0, 8);   /*            1 -> !            */
            keyboard_parse_set_pos_row(RETROK_2,            1, 0, 8);   /*            2 -> "            */
            keyboard_parse_set_pos_row(RETROK_3,            0, 1, 8);   /*            3 -> #            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 1, 8);   /*            4 -> $            */
            keyboard_parse_set_pos_row(RETROK_5,            0, 2, 8);   /*            5 -> %            */
            keyboard_parse_set_pos_row(RETROK_6,            1, 2, 8);   /*            6 -> '            */
            keyboard_parse_set_pos_row(RETROK_7,            0, 3, 8);   /*            7 -> &            */
            keyboard_parse_set_pos_row(RETROK_8,            1, 3, 8);   /*            8 -> \            */
            keyboard_parse_set_pos_row(RETROK_9,            0, 4, 8);   /*            9 -> (            */
            keyboard_parse_set_pos_row(RETROK_0,            1, 4, 8);   /*            0 -> )            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        0, 5, 8);   /*            - -> Left arrow   */

            keyboard_parse_set_pos_row(RETROK_q,            2, 0, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            3, 0, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            2, 1, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            3, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 2, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 2, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            2, 3, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            3, 3, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            2, 4, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            3, 4, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  2, 5, 8);   /*            [ -> Up arrow     */

            keyboard_parse_set_pos_row(RETROK_a,            4, 0, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            5, 0, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            4, 1, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            5, 1, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            4, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            5, 2, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 3, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            5, 3, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            4, 4, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 4, 8);   /*            ; -> :            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       6, 5, 8);   /*       Return -> RETURN       */

            keyboard_parse_set_pos_row(RETROK_z,            6, 0, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            7, 0, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            6, 1, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            7, 1, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            6, 2, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            7, 2, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            6, 3, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        7, 3, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       6, 4, 8);   /*            . -> ;            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        7, 4, 8);   /*            / -> ?            */

            keyboard_parse_set_pos_row(RETROK_LSHIFT,       8, 0, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_LCTRL,        9, 0, 8);   /*    Left Ctrl -> OFF/RVS      */
            keyboard_parse_set_pos_row(RETROK_INSERT,       8, 1, 8);   /*          Ins -> @            */
            keyboard_parse_set_pos_row(RETROK_HOME,         9, 1, 8);   /*         Home -> [            */
            keyboard_parse_set_pos_row(RETROK_PAGEUP,       8, 2, 8);   /*         PgUp -> ]            */
            keyboard_parse_set_pos_row(RETROK_SPACE,        9, 2, 8);   /*        Space -> Space        */
            keyboard_parse_set_pos_row(RETROK_END,          9, 3, 8);   /*          End -> <            */
            keyboard_parse_set_pos_row(RETROK_PAGEDOWN,     8, 4, 8);   /*       PgDown -> >            */
            keyboard_parse_set_pos_row(RETROK_ESCAPE,       9, 4, 8);   /*          Esc -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_RCTRL,        9, 4, 8);   /*   Right Ctrl -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       8, 5, 4);   /*  Right Shift -> Right Shift  */

            keyboard_parse_set_pos_row(RETROK_DELETE,       0, 6, 8);   /*          Del -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_UP,           1, 6, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         1, 6, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         0, 7, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        0, 7, 8);   /*        Right -> CRSR RIGHT   */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    1, 8, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_KP7,          2, 6, 8);   /*     Numpad 7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_KP8,          3, 6, 8);   /*     Numpad 8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_KP9,          2, 7, 8);   /*     Numpad 9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_KP_DIVIDE,    3, 7, 8);   /*     Numpad / -> /            */
            keyboard_parse_set_pos_row(RETROK_KP4,          4, 6, 8);   /*     Numpad 4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_KP5,          5, 6, 8);   /*     Numpad 5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_KP6,          4, 7, 8);   /*     Numpad 6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_KP_MULTIPLY,  5, 7, 8);   /*     Numpad * -> *            */
            keyboard_parse_set_pos_row(RETROK_KP1,          6, 6, 8);   /*     Numpad 1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_KP2,          7, 6, 8);   /*     Numpad 2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_KP3,          6, 7, 8);   /*     Numpad 3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_KP_PLUS,      7, 7, 8);   /*     Numpad + -> +            */
            keyboard_parse_set_pos_row(RETROK_KP0,          8, 6, 8);   /*     Numpad 0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_KP_PERIOD,    9, 6, 8);   /*     Numpad . -> .            */
            keyboard_parse_set_pos_row(RETROK_KP_MINUS,     8, 7, 8);   /*     Numpad - -> Minus        */
            keyboard_parse_set_pos_row(RETROK_KP_ENTER,     9, 7, 8);   /* Numpad Enter -> =            */
            break;
    }

#elif defined(__XVIC__)
    kbd_lshiftrow   = 1;
    kbd_lshiftcol   = 3;
    kbd_rshiftrow   = 6;
    kbd_rshiftcol   = 4;
    key_ctrl_vshift = KEY_RSHIFT;
    key_ctrl_shiftl = KEY_LSHIFT;
#if 0
    kbd_lcbmrow     = 0;
    kbd_lcbmcol     = 5;
    key_ctrl_vcbm   = KEY_LCBM;
#endif
    switch (opt_keyboard_keymap)
    {
        case 0: /* Symbolic */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    0, 1, 8);   /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_1,            0, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            0, 7, 32);  /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_2,            5, 6, 144); /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_AT,           5, 6, 0);   /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 7, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_DOLLAR,       1, 3, 1);   /*            $ -> $            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 7, 32);  /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_6,            6, 6, 145); /*            ^ -> ^            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 32);  /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_7,            2, 7, 129); /*            & -> &            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 7, 32);  /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_8,            6, 1, 144); /*            * -> *            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 32);  /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_9,            3, 7, 129); /*            ( -> )            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 7, 32);  /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 0, 129); /*            ) -> )            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 7, 32);  /*            - -> -            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 6, 2192);/*            _ -> _            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       6, 5, 32);  /*            = -> =            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 0, 144); /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_PLUS,         5, 0, 0);   /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 0, 8);   /*            \ -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 7, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_INSERT,       6, 0, 8);   /*       Insert -> INS          */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    7, 0, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        0, 5, 8);   /*    Left Ctrl -> CTRL         */
            keyboard_parse_set_pos_row(RETROK_q,            0, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 5, 1);   /*            [ -> [            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 2, 1);   /*            ] -> ]            */
            keyboard_parse_set_pos_row(RETROK_DELETE,       6, 6, 8);   /*       Delete -> Up Arrow     */
            keyboard_parse_set_neg_row(RETROK_PAGEUP,      -3, 0, 0);   /*       PageUp -> Restore      */

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       0, 3, 8);   /*          Esc -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 3, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    6, 2, 32);  /*            ; -> ;            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 144); /*            : -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        3, 0, 33);  /*            ' -> '            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        0, 7, 129); /*            " -> "            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       7, 1, 8);   /*       Return -> RETURN       */

            keyboard_parse_set_pos_row(RETROK_TAB,          7, 5, 8);   /*          Tab -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 3, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 3, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 3, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 3, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 3, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 3, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 3, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 4, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        0, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           7, 3, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         7, 3, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         7, 2, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        7, 2, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           7, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           7, 4, 1);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           7, 5, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           7, 5, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           7, 6, 8);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           7, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           7, 7, 8);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           7, 7, 1);   /*           F8 -> F8           */
            break;

        case 1: /* Positional */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    0, 1, 8);   /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_1,            0, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            0, 7, 8);   /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 7, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 7, 8);   /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 8);   /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 7, 8);   /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 8);   /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 7, 8);   /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 0, 8);   /*            - -> +            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 7, 8);   /*            = -> -            */
            keyboard_parse_set_pos_row(RETROK_INSERT,       6, 0, 8);   /*       Insert -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 7, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    7, 0, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_TAB,          0, 2, 8);   /*          Tab -> CTRL         */
            keyboard_parse_set_pos_row(RETROK_q,            0, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 6, 8);   /*            [ -> @            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 1, 8);   /*            ] -> *            */
            keyboard_parse_set_pos_row(RETROK_DELETE,       6, 6, 8);   /*       Delete -> Up Arrow     */
            keyboard_parse_set_neg_row(RETROK_PAGEUP,      -3, 0, 0);   /*       PageUp -> Restore      */

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       0, 3, 8);   /*          ESC -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 3, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 8);   /*            ; -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        6, 2, 8);   /*            ' -> ;            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 5, 8);   /*            \ -> =            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       7, 1, 8);   /*       Return -> RETURN       */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        0, 5, 8);   /*    Left Ctrl -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 3, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 3, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 3, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 3, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 3, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 3, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 3, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 4, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        0, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           7, 3, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         7, 3, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         7, 2, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        7, 2, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           7, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           7, 4, 1);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           7, 5, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           7, 5, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           7, 6, 8);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           7, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           7, 7, 8);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           7, 7, 1);   /*           F8 -> F8           */
            break;
    }

#elif defined(__XPLUS4__)
    kbd_lshiftrow   = 1;
    kbd_lshiftcol   = 7;
    kbd_rshiftrow   = 1;
    kbd_rshiftcol   = 7;
    key_ctrl_vshift = KEY_LSHIFT;
    key_ctrl_shiftl = KEY_LSHIFT;
#if 0
    kbd_lcbmrow     = 7;
    kbd_lcbmcol     = 5;
    key_ctrl_vcbm   = KEY_LCBM;
    kbd_lctrlrow    = 7;
    kbd_lctrlcol    = 2;
    key_ctrl_vctrl  = KEY_LCTRL;
#endif
    switch (opt_keyboard_keymap)
    {
        case 0: /* Symbolic */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    6, 4, 32);  /*            ` -> Esc          */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    6, 5, 1);   /*            ~ -> Pi           */
            keyboard_parse_set_pos_row(RETROK_1,            7, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            7, 3, 32);  /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_2,            0, 7, 144); /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_AT,           0, 7, 0);   /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 3, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_DOLLAR,       1, 3, 1);   /*            $ -> $            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 3, 32);  /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_6,            4, 3, 145); /*            ^ -> ^            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 32);  /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_7,            2, 3, 129); /*            & -> &            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 3, 32);  /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_8,            6, 1, 144); /*            * -> *            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 32);  /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_9,            3, 3, 129); /*            ( -> )            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 3, 32);  /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 0, 129); /*            ) -> )            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 6, 32);  /*            - -> -            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 6, 2192);/*            _ -> _            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       6, 5, 32);  /*            = -> =            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       6, 6, 144); /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_PLUS,         6, 6, 0);   /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    0, 2, 8);   /*            \ -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         7, 1, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_INSERT,       0, 0, 1);   /*       Insert -> INS          */
            keyboard_parse_set_pos_row(RETROK_DELETE,       0, 0, 8);   /*       Delete -> DEL          */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    0, 0, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        7, 2, 8);   /*    Left Ctrl -> Left Control */
            keyboard_parse_set_pos_row(RETROK_q,            7, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 5, 1);   /*            [ -> [            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 2, 1);   /*            ] -> ]            */
            keyboard_parse_set_pos_row(RETROK_RCTRL,        9, 4, 8);   /*   Right Ctrl -> Right Control*/

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       7, 7, 8);   /*          Esc -> Run/Stop     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 7, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    6, 2, 32);  /*            ; -> ;            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 144); /*            : -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        3, 0, 33);  /*            ' -> '            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        7, 3, 129); /*            " -> "            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       0, 1, 8);   /*       Return -> Return       */

            keyboard_parse_set_pos_row(RETROK_TAB,          7, 5, 8);   /*          Tab -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 7, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 7, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 7, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 7, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 7, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 7, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 7, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       1, 7, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        7, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           5, 3, 8);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         5, 0, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         6, 0, 8);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        6, 3, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           0, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           0, 5, 8);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           0, 6, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           0, 4, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           0, 5, 1);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           0, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           0, 3, 1);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 40);  /*           F8 -> HELP         */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 129); /*          sF8 -> F7           */
            break;

        case 1: /* Positional */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    6, 4, 8);   /*            ` -> Esc          */
            keyboard_parse_set_pos_row(RETROK_1,            7, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            7, 3, 8);   /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 3, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 3, 8);   /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 8);   /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 3, 8);   /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 8);   /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 3, 8);   /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        6, 6, 8);   /*            - -> +            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 6, 8);   /*            = -> -            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 5, 8);   /*            \ -> =            */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 3, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    0, 0, 8);   /*    Backspace -> Del          */

            keyboard_parse_set_pos_row(RETROK_TAB,          7, 2, 8);   /*          Tab -> Left Control */
            keyboard_parse_set_pos_row(RETROK_q,            7, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  0, 7, 8);   /*            [ -> @            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 1, 8);   /*            ] -> *            */
            keyboard_parse_set_pos_row(RETROK_INSERT,       0, 2, 8);   /*          Ins -> Pound        */
            keyboard_parse_set_pos_row(RETROK_RCTRL,        7, 2, 8);   /*   Right Ctrl -> Right Control*/

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       7, 7, 0);   /*          Esc -> Run/Stop     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 7, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 8);   /*            ; -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        6, 2, 8);   /*            ' -> ;            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       0, 1, 8);   /*       Return -> Return       */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        7, 5, 8);   /*    Left Ctrl -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 7, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 7, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 7, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 7, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 7, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 7, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 7, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       1, 7, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        7, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           5, 3, 8);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         5, 0, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         6, 0, 8);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        6, 3, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           0, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           0, 5, 8);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           0, 6, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           0, 4, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           0, 5, 1);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           0, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           0, 3, 1);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 40);  /*           F8 -> HELP         */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 129); /*          sF8 -> F7           */
            break;
    }

#elif defined(__X128__)
    kbd_lshiftrow   = 1;
    kbd_lshiftcol   = 7;
    kbd_rshiftrow   = 6;
    kbd_rshiftcol   = 4;
    key_ctrl_vshift = KEY_RSHIFT;
    key_ctrl_shiftl = KEY_LSHIFT;
    switch (opt_keyboard_keymap)
    {
        case 0: /* Symbolic */
            keyboard_parse_set_pos_row(RETROK_ESCAPE,       9, 0, 8);   /*          Esc -> ESC          */
            keyboard_parse_set_pos_row(RETROK_TAB,          8, 3, 8);   /*          Tab -> TAB          */
            keyboard_parse_set_pos_row(RETROK_LALT,        10, 0, 8);   /*     Left Alt -> ALT          */
            keyboard_parse_set_neg_row(RETROK_RALT,        -4, 1, 0);   /*    Right Alt -> CAPS         */

            keyboard_parse_set_pos_row(RETROK_HELP,         8, 0, 8);   /*         Help -> HELP         */
            keyboard_parse_set_pos_row(RETROK_END,          9, 3, 8);   /*          End -> LINE FEED    */
            keyboard_parse_set_neg_row(RETROK_F9,          -4, 0, 0);   /*           F9 -> 40/80        */
            keyboard_parse_set_pos_row(RETROK_F10,         10, 7, 8);   /*          F10 -> NO SCROLL    */

            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    7, 1, 32);  /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    6, 6, 1);   /*            ~ -> Pi           */
            keyboard_parse_set_pos_row(RETROK_1,            7, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            7, 3, 32);  /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_2,            5, 6, 144); /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_AT,           5, 6, 0);   /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 3, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_DOLLAR,       1, 3, 1);   /*            $ -> $            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 3, 32);  /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_6,            6, 6, 144); /*            ^ -> ^            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 32);  /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_7,            2, 3, 129); /*            & -> &            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 3, 32);  /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_8,            6, 1, 144); /*            * -> *            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 32);  /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_9,            3, 3, 129); /*            ( -> )            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 3, 32);  /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 0, 129); /*            ) -> )            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 3, 32);  /*            - -> -            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 6, 2192);/*            _ -> _            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       6, 5, 32);  /*            = -> =            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 0, 144); /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_PLUS,         5, 0, 8);   /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 0, 8);   /*            \ -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 3, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_INSERT,       0, 0, 1);   /*       Insert -> INS          */
            keyboard_parse_set_pos_row(RETROK_DELETE,       0, 0, 8);   /*       Delete -> DEL          */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    0, 0, 0);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        7, 2, 8);   /*    Left Ctrl -> CONTROL      */
            keyboard_parse_set_pos_row(RETROK_q,            7, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 5, 1);   /*            [ -> [            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 2, 1);   /*            ] -> ]            */
            keyboard_parse_set_neg_row(RETROK_PAGEUP,      -3, 0, 0);   /*       PageUp -> Restore      */
            keyboard_parse_set_pos_row(RETROK_PAGEDOWN,     6, 6, 8);   /*       PageDn -> Up Arrow     */

            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 7, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    6, 2, 32);  /*            ; -> ;            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 144); /*            : -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        3, 0, 33);  /*            ' -> '            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        7, 3, 129); /*            " -> "            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       0, 1, 8);   /*       Return -> RETURN       */

            keyboard_parse_set_pos_row(RETROK_LALT,         7, 5, 8);   /*     Left Alt -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 7, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 7, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 7, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 7, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 7, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 7, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 7, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 4, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        7, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           0, 7, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         0, 7, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         0, 2, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        0, 2, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           0, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           0, 4, 1);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           0, 5, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           0, 5, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           0, 6, 8);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           0, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           0, 3, 8);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 1);   /*           F8 -> F8           */

            keyboard_parse_set_pos_row(RETROK_KP7,          8, 6, 8);   /*     Numpad 7 -> Numpad 7     */
            keyboard_parse_set_pos_row(RETROK_KP8,          8, 1, 8);   /*     Numpad 8 -> Numpad 8     */
            keyboard_parse_set_pos_row(RETROK_KP9,          9, 6, 8);   /*     Numpad 9 -> Numpad 9     */
            keyboard_parse_set_pos_row(RETROK_KP_MINUS,     9, 2, 8);   /*     Numpad - -> Numpad -     */
            keyboard_parse_set_pos_row(RETROK_KP4,          8, 5, 8);   /*     Numpad 4 -> Numpad 4     */
            keyboard_parse_set_pos_row(RETROK_KP5,          8, 2, 8);   /*     Numpad 5 -> Numpad 5     */
            keyboard_parse_set_pos_row(RETROK_KP6,          9, 5, 8);   /*     Numpad 6 -> Numpad 6     */
            keyboard_parse_set_pos_row(RETROK_KP_PLUS,      9, 1, 8);   /*     Numpad + -> Numpad +     */
            keyboard_parse_set_pos_row(RETROK_KP1,          8, 7, 8);   /*     Numpad 1 -> Numpad 1     */
            keyboard_parse_set_pos_row(RETROK_KP2,          8, 4, 8);   /*     Numpad 2 -> Numpad 2     */
            keyboard_parse_set_pos_row(RETROK_KP3,          9, 7, 8);   /*     Numpad 3 -> Numpad 3     */
            keyboard_parse_set_pos_row(RETROK_KP0,         10, 1, 8);   /*     Numpad 0 -> Numpad 0     */
            keyboard_parse_set_pos_row(RETROK_KP_PERIOD,   10, 2, 8);   /*     Numpad . -> Numpad .     */
            keyboard_parse_set_pos_row(RETROK_KP_ENTER,     9, 4, 8);   /* Numpad Enter -> Numpad Enter */
            break;

        case 1: /* Positional */
            keyboard_parse_set_pos_row(RETROK_F1,           9, 0, 8);   /*           F1 -> ESC          */
            keyboard_parse_set_pos_row(RETROK_F2,           8, 3, 8);   /*           F2 -> TAB          */
            keyboard_parse_set_pos_row(RETROK_F3,          10, 0, 8);   /*           F3 -> ALT          */
            keyboard_parse_set_neg_row(RETROK_F4,          -4, 1, 0);   /*           F4 -> CAPS         */
            keyboard_parse_set_pos_row(RETROK_F5,           8, 0, 8);   /*           F5 -> HELP         */
            keyboard_parse_set_pos_row(RETROK_HELP,         8, 0, 8);   /*         Help -> HELP         */
            keyboard_parse_set_pos_row(RETROK_F6,           9, 3, 8);   /*           F6 -> LINE FEED    */
            keyboard_parse_set_neg_row(RETROK_F7,          -4, 0, 0);   /*           F7 -> 40/80        */
            keyboard_parse_set_pos_row(RETROK_F8,          10, 7, 8);   /*           F8 -> NO SCROLL    */
            keyboard_parse_set_pos_row(RETROK_F9,           0, 4, 8);   /*           F9 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F10,          0, 5, 8);   /*          F10 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F11,          0, 6, 8);   /*          F11 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F12,          0, 3, 8);   /*          F12 -> F7           */

            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    7, 1, 8);   /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_1,            7, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            7, 3, 8);   /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 3, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 3, 8);   /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 8);   /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 3, 8);   /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 8);   /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 3, 8);   /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 0, 8);   /*            - -> +            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 3, 8);   /*            = -> -            */
            keyboard_parse_set_pos_row(RETROK_INSERT,       6, 0, 8);   /*          Ins -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 3, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    0, 0, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_TAB,          7, 2, 8);   /*          Tab -> CTRL         */
            keyboard_parse_set_pos_row(RETROK_q,            7, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 6, 8);   /*            [ -> @            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 1, 8);   /*            ] -> *            */
            keyboard_parse_set_pos_row(RETROK_DELETE,       6, 6, 8);   /*          Del -> Up Arrow     */
            keyboard_parse_set_neg_row(RETROK_PAGEUP,      -3, 0, 0);   /*       PageUp -> Restore      */

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       7, 7, 8);   /*          Esc -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 7, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 8);   /*            ; -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        6, 2, 8);   /*            ' -> ;            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 5, 8);   /*            \ -> =            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       0, 1, 8);   /*       Return -> RETURN       */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        7, 5, 8);   /*    Left Ctrl -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 7, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 7, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 7, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 7, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 7, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 7, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 7, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 4, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        7, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           0, 7, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         0, 7, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         0, 2, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        0, 2, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_KP7,          8, 6, 8);   /*     Numpad 7 -> Numpad 7     */
            keyboard_parse_set_pos_row(RETROK_KP8,          8, 1, 8);   /*     Numpad 8 -> Numpad 8     */
            keyboard_parse_set_pos_row(RETROK_KP9,          9, 6, 8);   /*     Numpad 9 -> Numpad 9     */
            keyboard_parse_set_pos_row(RETROK_KP_MINUS,     9, 2, 8);   /*     Numpad - -> Numpad -     */
            keyboard_parse_set_pos_row(RETROK_KP4,          8, 5, 8);   /*     Numpad 4 -> Numpad 4     */
            keyboard_parse_set_pos_row(RETROK_KP5,          8, 2, 8);   /*     Numpad 5 -> Numpad 5     */
            keyboard_parse_set_pos_row(RETROK_KP6,          9, 5, 8);   /*     Numpad 6 -> Numpad 6     */
            keyboard_parse_set_pos_row(RETROK_KP_PLUS,      9, 1, 8);   /*     Numpad + -> Numpad +     */
            keyboard_parse_set_pos_row(RETROK_KP1,          8, 7, 8);   /*     Numpad 1 -> Numpad 1     */
            keyboard_parse_set_pos_row(RETROK_KP2,          8, 4, 8);   /*     Numpad 2 -> Numpad 2     */
            keyboard_parse_set_pos_row(RETROK_KP3,          9, 7, 8);   /*     Numpad 3 -> Numpad 3     */
            keyboard_parse_set_pos_row(RETROK_KP0,         10, 1, 8);   /*     Numpad 0 -> Numpad 0     */
            keyboard_parse_set_pos_row(RETROK_KP_PERIOD,   10, 2, 8);   /*     Numpad . -> Numpad .     */
            keyboard_parse_set_pos_row(RETROK_KP_ENTER,     9, 4, 8);   /* Numpad Enter -> Numpad Enter */
            break;
    }

#else /* X64 */
    kbd_lshiftrow   = 1;
    kbd_lshiftcol   = 7;
    kbd_rshiftrow   = 6;
    kbd_rshiftcol   = 4;
    key_ctrl_vshift = KEY_RSHIFT;
    key_ctrl_shiftl = KEY_LSHIFT;
#if 0
    kbd_lcbmrow     = 7;
    kbd_lcbmcol     = 5;
    key_ctrl_vcbm   = KEY_LCBM;
    kbd_lctrlrow    = 7;
    kbd_lctrlcol    = 2;
    key_ctrl_vctrl  = KEY_LCTRL;
#endif
    switch (opt_keyboard_keymap)
    {
        case 0: /* Symbolic */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    7, 1, 32);  /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    6, 6, 1);   /*            ~ -> Pi           */
            keyboard_parse_set_pos_row(RETROK_1,            7, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            7, 3, 32);  /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_2,            5, 6, 144); /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_AT,           5, 6, 0);   /*            @ -> @            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 3, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_DOLLAR,       1, 3, 1);   /*            $ -> $            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 3, 32);  /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_6,            6, 6, 144); /*            ^ -> ^            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 32);  /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_7,            2, 3, 129); /*            & -> &            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 3, 32);  /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_8,            6, 1, 144); /*            * -> *            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 32);  /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_9,            3, 3, 129); /*            ( -> )            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 3, 32);  /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 0, 129); /*            ) -> )            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 3, 32);  /*            - -> -            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 6, 2192);/*            _ -> _            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       6, 5, 32);  /*            = -> =            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 0, 144); /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_PLUS,         5, 0, 8);   /*            + -> +            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 0, 8);   /*            \ -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 3, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_INSERT,       0, 0, 1);   /*       Insert -> INS          */
            keyboard_parse_set_pos_row(RETROK_DELETE,       0, 0, 8);   /*       Delete -> DEL          */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    0, 0, 0);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        7, 2, 8);   /*    Left Ctrl -> Ctrl         */
            keyboard_parse_set_pos_row(RETROK_q,            7, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 5, 1);   /*            [ -> [            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 2, 1);   /*            ] -> ]            */
            keyboard_parse_set_neg_row(RETROK_PAGEUP,      -3, 0, 0);   /*       PageUp -> Restore      */

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       7, 7, 8);   /*          Esc -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 7, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    6, 2, 32);  /*            ; -> ;            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 144); /*            : -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        3, 0, 33);  /*            ' -> '            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        7, 3, 129); /*            " -> "            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       0, 1, 8);   /*       Return -> Return       */

            keyboard_parse_set_pos_row(RETROK_TAB,          7, 5, 8);   /*          Tab -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 7, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 7, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 7, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 7, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 7, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 7, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 7, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 4, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        7, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           0, 7, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         0, 7, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         0, 2, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        0, 2, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           0, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           0, 4, 1);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           0, 5, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           0, 5, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           0, 6, 8);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           0, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           0, 3, 8);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 1);   /*           F8 -> F8           */
            break;

        case 1: /* Positional */
            keyboard_parse_set_pos_row(RETROK_BACKQUOTE,    7, 1, 8);   /*            ` -> Left Arrow   */
            keyboard_parse_set_pos_row(RETROK_1,            7, 0, 8);   /*            1 -> 1            */
            keyboard_parse_set_pos_row(RETROK_2,            7, 3, 8);   /*            2 -> 2            */
            keyboard_parse_set_pos_row(RETROK_3,            1, 0, 8);   /*            3 -> 3            */
            keyboard_parse_set_pos_row(RETROK_4,            1, 3, 8);   /*            4 -> 4            */
            keyboard_parse_set_pos_row(RETROK_5,            2, 0, 8);   /*            5 -> 5            */
            keyboard_parse_set_pos_row(RETROK_6,            2, 3, 8);   /*            6 -> 6            */
            keyboard_parse_set_pos_row(RETROK_7,            3, 0, 8);   /*            7 -> 7            */
            keyboard_parse_set_pos_row(RETROK_8,            3, 3, 8);   /*            8 -> 8            */
            keyboard_parse_set_pos_row(RETROK_9,            4, 0, 8);   /*            9 -> 9            */
            keyboard_parse_set_pos_row(RETROK_0,            4, 3, 8);   /*            0 -> 0            */
            keyboard_parse_set_pos_row(RETROK_MINUS,        5, 0, 8);   /*            - -> +            */
            keyboard_parse_set_pos_row(RETROK_EQUALS,       5, 3, 8);   /*            = -> -            */
            keyboard_parse_set_pos_row(RETROK_INSERT,       6, 0, 8);   /*          Ins -> Pound        */
            keyboard_parse_set_pos_row(RETROK_HOME,         6, 3, 8);   /*         Home -> CLR/HOME     */
            keyboard_parse_set_pos_row(RETROK_BACKSPACE,    0, 0, 8);   /*    Backspace -> DEL          */

            keyboard_parse_set_pos_row(RETROK_TAB,          7, 2, 8);   /*          Tab -> CTRL         */
            keyboard_parse_set_pos_row(RETROK_q,            7, 6, 8);   /*            Q -> Q            */
            keyboard_parse_set_pos_row(RETROK_w,            1, 1, 8);   /*            W -> W            */
            keyboard_parse_set_pos_row(RETROK_e,            1, 6, 8);   /*            E -> E            */
            keyboard_parse_set_pos_row(RETROK_r,            2, 1, 8);   /*            R -> R            */
            keyboard_parse_set_pos_row(RETROK_t,            2, 6, 8);   /*            T -> T            */
            keyboard_parse_set_pos_row(RETROK_y,            3, 1, 8);   /*            Y -> Y            */
            keyboard_parse_set_pos_row(RETROK_u,            3, 6, 8);   /*            U -> U            */
            keyboard_parse_set_pos_row(RETROK_i,            4, 1, 8);   /*            I -> I            */
            keyboard_parse_set_pos_row(RETROK_o,            4, 6, 8);   /*            O -> O            */
            keyboard_parse_set_pos_row(RETROK_p,            5, 1, 8);   /*            P -> P            */
            keyboard_parse_set_pos_row(RETROK_LEFTBRACKET,  5, 6, 8);   /*            [ -> @            */
            keyboard_parse_set_pos_row(RETROK_RIGHTBRACKET, 6, 1, 8);   /*            ] -> *            */
            keyboard_parse_set_pos_row(RETROK_DELETE,       6, 6, 8);   /*          Del -> Up Arrow     */
            keyboard_parse_set_neg_row(RETROK_PAGEUP,      -3, 0, 0);   /*       PageUp -> Restore      */

            keyboard_parse_set_pos_row(RETROK_ESCAPE,       7, 7, 8);   /*          Esc -> RUN/STOP     */
            keyboard_parse_set_pos_row(RETROK_CAPSLOCK,     1, 7, 64);  /*    Caps Lock -> Shift Lock   */
            keyboard_parse_set_pos_row(RETROK_a,            1, 2, 8);   /*            A -> A            */
            keyboard_parse_set_pos_row(RETROK_s,            1, 5, 8);   /*            S -> S            */
            keyboard_parse_set_pos_row(RETROK_d,            2, 2, 8);   /*            D -> D            */
            keyboard_parse_set_pos_row(RETROK_f,            2, 5, 8);   /*            F -> F            */
            keyboard_parse_set_pos_row(RETROK_g,            3, 2, 8);   /*            G -> G            */
            keyboard_parse_set_pos_row(RETROK_h,            3, 5, 8);   /*            H -> H            */
            keyboard_parse_set_pos_row(RETROK_j,            4, 2, 8);   /*            J -> J            */
            keyboard_parse_set_pos_row(RETROK_k,            4, 5, 8);   /*            K -> K            */
            keyboard_parse_set_pos_row(RETROK_l,            5, 2, 8);   /*            L -> L            */
            keyboard_parse_set_pos_row(RETROK_SEMICOLON,    5, 5, 8);   /*            ; -> :            */
            keyboard_parse_set_pos_row(RETROK_QUOTE,        6, 2, 8);   /*            ' -> ;            */
            keyboard_parse_set_pos_row(RETROK_BACKSLASH,    6, 5, 8);   /*            \ -> =            */
            keyboard_parse_set_pos_row(RETROK_RETURN,       0, 1, 8);   /*       Return -> Return       */

            keyboard_parse_set_pos_row(RETROK_LCTRL,        7, 5, 8);   /*    Left Ctrl -> C=           */
            keyboard_parse_set_pos_row(RETROK_LSHIFT,       1, 7, 2);   /*   Left Shift -> Left Shift   */
            keyboard_parse_set_pos_row(RETROK_z,            1, 4, 8);   /*            Z -> Z            */
            keyboard_parse_set_pos_row(RETROK_x,            2, 7, 8);   /*            X -> X            */
            keyboard_parse_set_pos_row(RETROK_c,            2, 4, 8);   /*            C -> C            */
            keyboard_parse_set_pos_row(RETROK_v,            3, 7, 8);   /*            V -> V            */
            keyboard_parse_set_pos_row(RETROK_b,            3, 4, 8);   /*            B -> B            */
            keyboard_parse_set_pos_row(RETROK_n,            4, 7, 8);   /*            N -> N            */
            keyboard_parse_set_pos_row(RETROK_m,            4, 4, 8);   /*            M -> M            */
            keyboard_parse_set_pos_row(RETROK_COMMA,        5, 7, 8);   /*            , -> ,            */
            keyboard_parse_set_pos_row(RETROK_PERIOD,       5, 4, 8);   /*            . -> .            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 7, 33);  /*            < -> <            */
            keyboard_parse_set_pos_row(RETROK_OEM_102,      5, 4, 145); /*            > -> >            */
            keyboard_parse_set_pos_row(RETROK_SLASH,        6, 7, 8);   /*            / -> /            */
            keyboard_parse_set_pos_row(RETROK_RSHIFT,       6, 4, 4);   /*  Right Shift -> Right Shift  */
            keyboard_parse_set_pos_row(RETROK_SPACE,        7, 4, 8);   /*        Space -> Space        */

            keyboard_parse_set_pos_row(RETROK_UP,           0, 7, 1);   /*           Up -> CRSR UP      */
            keyboard_parse_set_pos_row(RETROK_DOWN,         0, 7, 8);   /*         Down -> CRSR DOWN    */
            keyboard_parse_set_pos_row(RETROK_LEFT,         0, 2, 1);   /*         Left -> CRSR LEFT    */
            keyboard_parse_set_pos_row(RETROK_RIGHT,        0, 2, 8);   /*        Right -> CRSR RIGHT   */

            keyboard_parse_set_pos_row(RETROK_F1,           0, 4, 8);   /*           F1 -> F1           */
            keyboard_parse_set_pos_row(RETROK_F2,           0, 4, 1);   /*           F2 -> F2           */
            keyboard_parse_set_pos_row(RETROK_F3,           0, 5, 8);   /*           F3 -> F3           */
            keyboard_parse_set_pos_row(RETROK_F4,           0, 5, 1);   /*           F4 -> F4           */
            keyboard_parse_set_pos_row(RETROK_F5,           0, 6, 8);   /*           F5 -> F5           */
            keyboard_parse_set_pos_row(RETROK_F6,           0, 6, 1);   /*           F6 -> F6           */
            keyboard_parse_set_pos_row(RETROK_F7,           0, 3, 8);   /*           F7 -> F7           */
            keyboard_parse_set_pos_row(RETROK_F8,           0, 3, 1);   /*           F8 -> F8           */
            break;
    }
#endif
}
