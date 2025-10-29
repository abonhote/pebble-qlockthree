#include <pebble.h>
#include <stdlib.h>

static Window *s_main_window;
static Layer *s_canvas_layer;
static GFont s_bold_font;
static GFont s_normal_font;
static GFont s_font;


static char s_lines[11][15] =
{
    "ES-ISCH--ZÄ",
    "-FOIF-PUNKT",
    "VIERTEL-AB-",
    "ZWÄNZG-VOR-",
    "PUNKT-HALBI",
    "ZWEI--FOIFI",
    "VIERI-DRÜ--",
    "ACHTI-NÜNI-",
    "SÄCHSI-ELFI",
    "-SIEBNI-EIS",
    "ZÄNI-ZWÖLFI"
};

// Bitmaske für hervorgehobene Zeichen (1 = weis, 0 = grau)
static bool s_highlight[11][15];  // 11 Zeilen, max 12 Zeichen pro Zeile


static void canvas_update_proc(Layer *layer, GContext *ctx)
{
    GRect bounds = layer_get_bounds(layer);
    int char_width = 12;
    int line_width = 11 * char_width;
    int y = 2;
    int start_x = (bounds.size.w - line_width) / 2;
    for (int line = 0; line < 11; line++)
    {
        int x = start_x;
        int col = 0;
        for (size_t i = 0; i < strlen(s_lines[line]); i++)
        {
            char single_char[4] = {0};  // Für UTF-8 Zeichen + \0
            // Zeichen kopieren (mit UTF-8 Support)
            if ((s_lines[line][i] & 0x80) == 0)
            {
                // ASCII (1 Byte)
                single_char[0] = s_lines[line][i];
                single_char[1] = '\0';
            }
            else if ((s_lines[line][i] & 0xE0) == 0xC0)
            {
                // UTF-8 2-Byte Zeichen
                single_char[0] = s_lines[line][i];
                single_char[1] = s_lines[line][i + 1];
                single_char[2] = '\0';
                i++;  // Überspringe zweites Byte
            }
            // Farbe setzen basierend auf Highlight-Status
            if (s_highlight[line][col])
            {
                graphics_context_set_text_color(ctx, GColorWhite);
                s_font = s_bold_font;
            }
            else
            {
                graphics_context_set_text_color(ctx, GColorDarkGray);
                s_font = s_normal_font;
            }
            graphics_draw_text(ctx, single_char, s_font
                               ,
                               GRect(x, y, char_width + 5, 20),
                               GTextOverflowModeWordWrap,
                               GTextAlignmentLeft, NULL);
            x += char_width;
            col++;
        }
        y += 15;
    }
}

// https://www.drk.com.ar/en/count-characters-in-utf8-string-cpp/
size_t utf8len(const char *s)
{
    size_t len = 0;
    while (*s)
        len += (*(s++) & 0xC0) != 0x80;
    return len;
}

static void highlight_word(const char *word)
{
    size_t wordlength = utf8len(word);
    APP_LOG(APP_LOG_LEVEL_DEBUG, "Word: %s - length: %d", word, wordlength);
    for (int line = 0; line < 11; line++)
    {
        char *pos = strstr(s_lines[line], word);
        if (pos != NULL)
        {
            int start_index = pos - s_lines[line];
            for (size_t i = 0; i < wordlength; i++)
            {
                s_highlight[line][start_index + i] = true;
            }
            break;
        }
    }
}

static void update_time(struct tm *tick_time, TimeUnits units_changed)
{
    int hour = tick_time->tm_hour % 12;
    int minute = tick_time->tm_min;
    if (hour == 0) hour = 12;
    // Alle Highlights zurücksetzen
    memset(s_highlight, 0, sizeof(s_highlight));
    highlight_word("ES");
    highlight_word("ISCH");
    if (hour == 1) highlight_word("EIS");
    if (hour == 2) highlight_word("ZWEI");
    if (hour == 3) highlight_word("DRÜ");
    if (hour == 4) highlight_word("VIERI");
    if (hour == 5) highlight_word("FOIFI");
    if (hour == 6) highlight_word("SÄCHSI");
    if (hour == 7) highlight_word("SIEBNI");
    if (hour == 8) highlight_word("ACHTI");
    if (hour == 9) highlight_word("NÜNI");
    if (hour == 10) highlight_word("ZÄNI");
    if (hour == 11) highlight_word("ELFI");
    if (hour == 12) highlight_word("ZWÖLFI");
    // Minuten
    if (minute == 0) highlight_word("PUNKT");
    if (minute >= 5 && minute < 10) highlight_word("FOIF");
    if (minute >= 10 &&
            minute < 15) highlight_word("ZÄ"); // könnte knallen mit "Zäni"
    if (minute >= 15 && minute < 20) highlight_word("VIERTEL");
    if (minute >= 20 && minute < 25) highlight_word("ZWÄNZG");
    if (minute >= 5 && minute < 25) highlight_word("AB");
    if (minute >= 25 &&
            minute < 40)
        highlight_word("HALBI"); // 25: foif vor halbi; 30: halbi; 35 foif ab halbi
    if (minute >= 25 && minute < 30) highlight_word("VOR");
    if (minute >= 35 && minute < 40) highlight_word("AB");
    if (minute >= 40 && minute < 45) highlight_word("ZWÄNZG");
    if (minute >= 45 && minute < 50) highlight_word("VIERTEL");
    if (minute >= 50 && minute < 55) highlight_word("ZÄ");
    if (minute >= 55 && minute != 0) highlight_word("FOIF");
    if (minute > 40 && minute != 0) highlight_word("VOR");
    // Neu zeichnen
    layer_mark_dirty(s_canvas_layer);
}

static void randomize_dashes()
{
    for (int line = 0; line < 11; line ++)
    {
        for (size_t i = 0; i < strlen(s_lines[line]); i++)
        {
            if (s_lines[line][i] == '-')
            {
                s_lines[line][i] = 'A' + (rand() % 26);
            }
        }
    }
}
static void window_load(Window *window)
{
    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);
    // Canvas Layer erstellen
    s_canvas_layer = layer_create(bounds);
    layer_set_update_proc(s_canvas_layer, canvas_update_proc);
    layer_add_child(window_layer, s_canvas_layer);
}

static void window_unload(Window *window)
{
    layer_destroy(s_canvas_layer);
}

static void init()
{
    srand(time(NULL));
    randomize_dashes();
    s_main_window = window_create();
    window_set_background_color(s_main_window, GColorBlack);
    window_set_window_handlers(s_main_window, (WindowHandlers)
    {
        .load = window_load,
        .unload = window_unload
    });
    window_stack_push(s_main_window, true);
    s_bold_font = fonts_load_custom_font(resource_get_handle(
            RESOURCE_ID_FONT_ANONYMOUSPROBOLD_12));
    s_normal_font = fonts_load_custom_font(resource_get_handle(
            RESOURCE_ID_FONT_ANONYMOUSPRO_11));
    // Initiales Zeit-Update (manuell mit aktuellem Zeitstempel)
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    update_time(tick_time, MINUTE_UNIT);
    tick_timer_service_subscribe(MINUTE_UNIT, update_time);
}

static void deinit()
{
    fonts_unload_custom_font(s_bold_font);
    fonts_unload_custom_font(s_normal_font);
    window_destroy(s_main_window);
}

int main(void)
{
    init();
    app_event_loop();
    deinit();
}