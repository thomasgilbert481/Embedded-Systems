//------------------------------------------------------------------------------
// File:    menus.c
// Author:  Noah Cartwright
// Date:    March 31, 2026
// Course:  ECE 306 — Introduction to Embedded Systems
//
// Description:
//   Implements the three scrolling menus required for Homework 9.
//
//   MENU HIERARCHY
//   ──────────────
//   Splash screen  →  any button press  →  Main Menu
//   Main Menu      →  SW1               →  Resistors | Shapes | Song
//   Any sub-menu   →  SW2               →  Main Menu
//
//   THUMBWHEEL (ADC_Thumb, 10-bit, 0–1023)
//   ────────────────────────────────────────
//   Main menu   : 3 items, ADC divided into thirds  (÷ ADC_MAIN_DIV → 0–2)
//   Resistors   : 10 items, ADC divided into tenths (÷ ADC_SUB_DIV  → 0–9)
//   Shapes      : 10 items, ADC divided into tenths (÷ ADC_SUB_DIV  → 0–9)
//   Song        : 8 comfortable zones (ADC >> ADC_SONG_SHIFT → 0–7).
//                 Only CCW movement (zone number decreasing) advances the
//                 song.  SONG_CHARS_PER_ZONE (5) characters are added to
//                 the right end of the scrolling display buffer each time
//                 a zone boundary is crossed CCW.  A full CCW sweep (7→0)
//                 advances 35 characters; the complete lyrics (~243 chars)
//                 require ~7 full sweeps — "several twists".
//
//   DISPLAY MODES
//   ─────────────
//   Splash, Main Menu, Resistors : lcd_4line()
//   Shapes, Song                 : lcd_BIG_mid()
//
// Globals Read:    ADC_Thumb, sw1_action_pending, sw2_action_pending
// Globals Written: display_line[][], display_changed, update_display,
//                  sw1_action_pending, sw2_action_pending
//------------------------------------------------------------------------------

#include  "msp430.h"
#include  <string.h>
#include  "functions.h"
#include  "LCD.h"
#include  "ports.h"
#include  "macros.h"
#include  "globals.h"

// ─── Module-private persistent state ─────────────────────────────────────────

// Current position in the menu hierarchy
static char          menu_state     = MENU_SPLASH;

// "Last displayed" trackers — set to INVALID_ITEM so the first call to each
// menu always writes to the LCD even if the ADC reads 0.
static unsigned int  last_main_item = INVALID_ITEM;
static unsigned int  last_res_item  = INVALID_ITEM;
static unsigned int  last_shp_item  = INVALID_ITEM;

// Song state
static unsigned int  song_idx       = SONG_START; // index of next char to load
static char          song_buf[SONG_DISP_LEN + 1]; // 10-char window + null
static unsigned char song_zone_prev = SONG_ALT_RESET; // last ADC zone seen
static char          song_alt       = SONG_ALT_RESET; // alternating Line1/3 flag

// ─── Lookup tables ────────────────────────────────────────────────────────────

// Resistor colour names — each string is exactly 10 printable chars + null
static const char res_color[NUM_RESISTORS][LCD_LINE_BUF] = {
    " Black    ",   //  0
    " Brown    ",   //  1
    " Red      ",   //  2
    " Orange   ",   //  3
    " Yellow   ",   //  4
    " Green    ",   //  5
    " Blue     ",   //  6
    " Violet   ",   //  7
    " Gray     ",   //  8
    " White    "    //  9
};

// Resistor band value digits — centred in 10 chars
static const char res_value[NUM_RESISTORS][LCD_LINE_BUF] = {
    "    0     ",
    "    1     ",
    "    2     ",
    "    3     ",
    "    4     ",
    "    5     ",
    "    6     ",
    "    7     ",
    "    8     ",
    "    9     "
};

// Shape names — each string is exactly 10 printable chars + null
static const char shp_name[NUM_SHAPES][LCD_LINE_BUF] = {
    "  Circle  ",   //  0
    "  Square  ",   //  1
    " Triangle ",   //  2
    " Octagon  ",   //  3
    " Pentagon ",   //  4
    " Hexagon  ",   //  5
    "   Cube   ",   //  6
    "   Oval   ",   //  7
    "  Sphere  ",   //  8
    " Cylinder "    //  9
};

// Fight-song lyrics.
//   10 leading spaces  → characters scroll ON  from the right.
//   10 trailing spaces → characters scroll OFF to the left.
static const char song_text[] =
    "          "
    "We're the Red and White from State "
    "And we know we are the best. "
    "A hand behind our back, "
    "We can take on all the rest. "
    "Come over the hill, Carolina. "
    "Devils and Deacs stand in line. "
    "The Red and White from N.C. State. "
    "Go State!"
    "          ";

// Total usable characters in song_text (excludes the null terminator)
#define SONG_LEN  ((unsigned int)(sizeof(song_text) - 1u))

// ─── Internal helper ──────────────────────────────────────────────────────────

//------------------------------------------------------------------------------
// Function: flag_refresh
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description: Sets the two display-driver flags so Display_Process() will
//              call Display_Update() on the next main-loop tick.
// Parameters:  None
// Returns:     None
// Globals Modified: display_changed, update_display
//------------------------------------------------------------------------------
static void flag_refresh(void)
{
    display_changed = TRUE;
    update_display  = TRUE;
}

//==============================================================================
// Function: Menu_Process
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Top-level menu dispatcher.  Must be called every main-loop iteration.
//   Routes execution to the correct handler based on menu_state.  Also
//   handles the splash-screen → main-menu transition when either push-button
//   is pressed.
// Parameters:  None
// Returns:     None
// Globals Modified: menu_state, sw1_action_pending, sw2_action_pending,
//                   display_line[][], display_changed, update_display,
//                   last_main_item, last_res_item, last_shp_item,
//                   song_idx, song_buf, song_zone_prev, song_alt
//==============================================================================
void Menu_Process(void)
{
    switch (menu_state)
    {
        // ── Splash: LCD is already in lcd_BIG_mid() — set by main.c ──────────
        case MENU_SPLASH:
            // Either button press advances to the main menu
            if (sw1_action_pending || sw2_action_pending)
            {
                sw1_action_pending = CLEAR_FLAG;
                sw2_action_pending = CLEAR_FLAG;
                lcd_4line();                    // restore 4-line layout for main menu
                last_main_item = INVALID_ITEM;  // force full redraw on first entry
                menu_state     = MENU_MAIN;
            }
            break;

        case MENU_MAIN:
            Main_Menu();
            break;

        case MENU_RESISTOR:
            Resistor_Menu();
            break;

        case MENU_SHAPES:
            Shapes_Menu();
            break;

        case MENU_SONG:
            Song_Menu();
            break;

        default:
            // Safety net — should never reach here
            menu_state = MENU_MAIN;
            break;
    }
}

//==============================================================================
// Function: Main_Menu
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Displays a 3-item scrolling main menu using lcd_4line() layout.
//   The thumbwheel divides the 10-bit ADC range into three equal regions:
//     0–340   → item 0: Resistors
//     341–681 → item 1: Shapes
//     682–1023 → item 2: Song
//   The selected item is shown on Line 2 with a '>' marker.  The item
//   immediately above (if any) appears on Line 1 and the item below on
//   Line 3.  Line 4 prompts the user to press SW1.
//
//   SW1 → enter the selected sub-menu.
//   SW2 → no effect (already at top level; flag is consumed).
//
// Parameters:  None
// Returns:     None
// Globals Modified: menu_state, sw1_action_pending, sw2_action_pending,
//                   last_main_item, last_res_item, last_shp_item,
//                   song_idx, song_buf, song_zone_prev, song_alt,
//                   display_line[][], display_changed, update_display
//==============================================================================
void Main_Menu(void)
{
    unsigned int item;

    // Map 12-bit ADC to one of 3 items; clamp in case ADC reads exactly 4095
    item = ADC_Thumb / ADC_MAIN_DIV;
    if (item >= NUM_MAIN_ITEMS)
    {
        item = NUM_MAIN_ITEMS - 1u;
    }

    // Refresh the display only when the selection actually changes
    if (item != last_main_item)
    {
        last_main_item = item;

        switch (item)
        {
            case MAIN_RESISTORS:
                strcpy(display_line[MENU_LINE1], "          "); // no item above
                strcpy(display_line[MENU_LINE2], ">Resistors");
                strcpy(display_line[MENU_LINE3], " Shapes   ");
                strcpy(display_line[MENU_LINE4], "SW1=Select");
                break;

            case MAIN_SHAPES:
                strcpy(display_line[MENU_LINE1], " Resistors");
                strcpy(display_line[MENU_LINE2], "> Shapes  ");
                strcpy(display_line[MENU_LINE3], " Song     ");
                strcpy(display_line[MENU_LINE4], "SW1=Select");
                break;

            case MAIN_SONG:
            default:
                strcpy(display_line[MENU_LINE1], " Shapes   ");
                strcpy(display_line[MENU_LINE2], "> Song    ");
                strcpy(display_line[MENU_LINE3], "          "); // no item below
                strcpy(display_line[MENU_LINE4], "SW1=Select");
                break;
        }
        flag_refresh();
    }

    // SW2 at top level: consume the flag, no other action
    if (sw2_action_pending)
    {
        sw2_action_pending = CLEAR_FLAG;
    }

    // SW1: enter the currently selected sub-menu
    if (sw1_action_pending)
    {
        sw1_action_pending = CLEAR_FLAG;

        switch (item)
        {
            case MAIN_RESISTORS:
                last_res_item = INVALID_ITEM;   // force redraw on first entry
                menu_state    = MENU_RESISTOR;
                break;

            case MAIN_SHAPES:
                last_shp_item = INVALID_ITEM;   // force redraw on first entry
                lcd_BIG_mid();                  // switch to large-centre layout
                menu_state    = MENU_SHAPES;
                break;

            case MAIN_SONG:
            default:
                // Initialise song scrolling state
                song_idx       = SONG_START;
                song_zone_prev = (unsigned char)(ADC_Thumb >> ADC_SONG_SHIFT);
                song_alt       = SONG_ALT_RESET;
                memset(song_buf, ' ', SONG_DISP_LEN); // blank display window
                song_buf[SONG_DISP_LEN] = '\0';

                lcd_BIG_mid();

                // Prime the LCD with the initial (all-blank) state
                strcpy(display_line[BIG_TOP], "Red&White ");
                strcpy(display_line[BIG_MID],  song_buf);
                strcpy(display_line[BIG_BOT], "White&Red ");
                flag_refresh();

                menu_state = MENU_SONG;
                break;
        }
    }
}

//==============================================================================
// Function: Resistor_Menu
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Displays the 10 resistor colour-code entries using lcd_4line() layout.
//   The thumbwheel divides the 10-bit ADC range into 10 equal regions
//   (÷ ADC_SUB_DIV), selecting Black (0) through White (9).
//
//   Display layout:
//     Line 1 — "  Color   "  (fixed header)
//     Line 2 — selected colour name (e.g. " Orange   ")
//     Line 3 — "  Value   "  (fixed header)
//     Line 4 — numeric band value (e.g. "    3     ")
//
//   The display is updated only when the thumbwheel crosses into a new
//   region, preventing unnecessary LCD writes.
//
//   SW2 → return to Main_Menu.
//   SW1 → no action (thumbwheel drives selection).
//
// Parameters:  None
// Returns:     None
// Globals Modified: menu_state, sw1_action_pending, sw2_action_pending,
//                   last_res_item, last_main_item,
//                   display_line[][], display_changed, update_display
//==============================================================================
void Resistor_Menu(void)
{
    unsigned int item;

    // Map 12-bit ADC to one of 10 colour codes; clamp to valid range
    item = ADC_Thumb / ADC_SUB_DIV;
    if (item >= NUM_RESISTORS)
    {
        item = NUM_RESISTORS - 1u;
    }

    // Refresh only when selection changes
    if (item != last_res_item)
    {
        last_res_item = item;

        strcpy(display_line[MENU_LINE1], "  Color   ");
        strcpy(display_line[MENU_LINE2], res_color[item]);
        strcpy(display_line[MENU_LINE3], "  Value   ");
        strcpy(display_line[MENU_LINE4], res_value[item]);

        flag_refresh();
    }

    // SW1: no action in this sub-menu
    if (sw1_action_pending)
    {
        sw1_action_pending = CLEAR_FLAG;
    }

    // SW2: return to main menu
    if (sw2_action_pending)
    {
        sw2_action_pending = CLEAR_FLAG;
        last_main_item     = INVALID_ITEM;  // force main-menu redraw
        menu_state         = MENU_MAIN;
    }
}

//==============================================================================
// Function: Shapes_Menu
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Displays the 10 shape names using lcd_BIG_mid() layout.
//   The thumbwheel divides the 10-bit ADC range into 10 equal regions
//   (÷ ADC_SUB_DIV), selecting Circle (0) through Cylinder (9).
//
//   Display layout:
//     Line 1 (small top) — previous shape name, or blank at the first entry
//     BIG centre line    — currently selected shape name (large text)
//     Line 3 (small bot) — next shape name, or blank at the last entry
//
//   Blank padding at the extremes guarantees the chosen shape is always
//   visible on the large centre line.
//
//   SW2 → return to Main_Menu (restores lcd_4line()).
//   SW1 → no action (thumbwheel drives selection).
//
// Parameters:  None
// Returns:     None
// Globals Modified: menu_state, sw1_action_pending, sw2_action_pending,
//                   last_shp_item, last_main_item,
//                   display_line[][], display_changed, update_display
//==============================================================================
void Shapes_Menu(void)
{
    unsigned int item;

    // Map 12-bit ADC to one of 10 shapes; clamp to valid range
    item = ADC_Thumb / ADC_SUB_DIV;
    if (item >= NUM_SHAPES)
    {
        item = NUM_SHAPES - 1u;
    }

    // Refresh only when selection changes
    if (item != last_shp_item)
    {
        last_shp_item = item;

        // Previous shape (Line 1) — blank if this is the first shape
        if (item > 0u)
        {
            strcpy(display_line[BIG_TOP], shp_name[item - 1u]);
        }
        else
        {
            strcpy(display_line[BIG_TOP], "          ");
        }

        // Current shape — shown on the large centre line
        strcpy(display_line[BIG_MID], shp_name[item]);

        // Next shape (Line 3) — blank if this is the last shape
        if (item < (NUM_SHAPES - 1u))
        {
            strcpy(display_line[BIG_BOT], shp_name[item + 1u]);
        }
        else
        {
            strcpy(display_line[BIG_BOT], "          ");
        }

        flag_refresh();
    }

    // SW1: no action in this sub-menu
    if (sw1_action_pending)
    {
        sw1_action_pending = CLEAR_FLAG;
    }

    // SW2: return to main menu, restore 4-line layout
    if (sw2_action_pending)
    {
        sw2_action_pending = CLEAR_FLAG;
        lcd_4line();                    // switch back to 4-line LCD mode
        last_main_item     = INVALID_ITEM;  // force main-menu redraw
        menu_state         = MENU_MAIN;
    }
}

//==============================================================================
// Function: Song_Menu
// Author:   Noah Cartwright
// Date:     March 31, 2026
// Description:
//   Scrolls the "Red and White" fight-song character-by-character across
//   the large centre line using lcd_BIG_mid() layout.  Characters enter
//   from the right and exit to the left.
//
//   Scroll mechanism (zone-based ratchet):
//     ADC_SONG_SHIFT (7) right-shifts the 10-bit ADC into 8 zones (0–7).
//     Each time the thumbwheel moves CCW (current zone < previous zone),
//     SONG_CHARS_PER_ZONE (5) new song characters are shifted into the
//     right end of the 10-character display buffer and the same number
//     of old characters are shifted off the left.
//     CW movement updates the zone tracker but does NOT advance the song,
//     allowing re-triggering after a CW return.
//     A full CCW sweep (zone 7 → 0) advances 35 characters.  The complete
//     lyrics (~243 chars with padding) require approximately 7 full sweeps.
//
//   Display layout (lcd_BIG_mid):
//     Line 1 (small top) — alternates "Red&White " / "White&Red " on advance
//     BIG centre line    — 10-character scrolling song window
//     Line 3 (small bot) — the complementary alternating string
//
//   SW2 → return to Main_Menu (restores lcd_4line()).
//   SW1 → no action.
//
// Parameters:  None
// Returns:     None
// Globals Modified: menu_state, sw1_action_pending, sw2_action_pending,
//                   song_idx, song_buf, song_zone_prev, song_alt,
//                   last_main_item,
//                   display_line[][], display_changed, update_display
//==============================================================================
void Song_Menu(void)
{
    unsigned char current_zone; // ADC zone computed this call (0–7)
    unsigned int  ch_cnt;       // loop counter for character-advance loop
    char          need_refresh; // TRUE when song_buf was updated this call

    need_refresh = FALSE;

    // ── Compute current ADC zone ──────────────────────────────────────────────
    current_zone = (unsigned char)(ADC_Thumb >> ADC_SONG_SHIFT);

    // ── CCW movement detected: advance the song ───────────────────────────────
    if (current_zone < song_zone_prev)
    {
        // Advance up to SONG_CHARS_PER_ZONE characters; stop at end of lyrics
        for (ch_cnt = 0u; ch_cnt < SONG_CHARS_PER_ZONE; ch_cnt++)
        {
            if (song_idx < SONG_LEN)
            {
                // Shift the 10-char buffer one position to the left
                memmove(song_buf, song_buf + 1u, SONG_DISP_LEN - 1u);

                // Place the next song character at the right end
                song_buf[SONG_DISP_LEN - 1u] = song_text[song_idx];
                song_idx++;

                need_refresh = TRUE;
            }
        }

        // Toggle the alternating Line-1 / Line-3 header on every advance
        if (need_refresh)
        {
            song_alt = (song_alt == SONG_ALT_RESET) ? SET_FLAG : SONG_ALT_RESET;
        }
    }

    // Always update zone tracker — enables re-triggering after a CW sweep
    song_zone_prev = current_zone;

    // ── Write to LCD only when content changed ────────────────────────────────
    if (need_refresh)
    {
        if (song_alt == SONG_ALT_RESET)
        {
            strcpy(display_line[BIG_TOP], "Red&White ");
            strcpy(display_line[BIG_BOT], "White&Red ");
        }
        else
        {
            strcpy(display_line[BIG_TOP], "White&Red ");
            strcpy(display_line[BIG_BOT], "Red&White ");
        }

        strcpy(display_line[BIG_MID], song_buf);
        flag_refresh();
    }

    // SW1: no action in this sub-menu
    if (sw1_action_pending)
    {
        sw1_action_pending = CLEAR_FLAG;
    }

    // SW2: return to main menu, restore 4-line layout
    if (sw2_action_pending)
    {
        sw2_action_pending = CLEAR_FLAG;
        lcd_4line();                    // switch back to 4-line LCD mode
        last_main_item     = INVALID_ITEM;  // force main-menu redraw
        menu_state         = MENU_MAIN;
    }
}
