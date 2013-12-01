/****************************************************************************/
/**
* Pebble watch face that displays time similar to the way the Android 4.2
* clock does it.
*
* @file   roboto.c
*
* @author Bob Hauck <bobh@haucks.org>
*
* This code may be used or modified in any way whatsoever without permission
* from the author.
*
*****************************************************************************/
#include "pebble.h"


#define TIME_FRAME      (GRect(0, 40, 144, 168-40))
#define DATE_FRAME      (GRect(0, 98, 144, 168-98))


/* Custom layer type for displaying time with different fonts for hour
* and minute.
*/
typedef struct _TimeLayer
{
    Layer *layer;
    GRect bounds;
    const char *hour_text;
    const char *minute_text;
    GFont hour_font;
    GFont minute_font;
    GTextLayoutCacheRef layout_cache;
    GColor text_color : 2;
    GColor background_color : 2;
    GTextOverflowMode overflow_mode : 2;
} TimeLayer;


Window *window;         /* main window */
TextLayer *date_layer;  /* layer for the date */
TimeLayer *time_layer;  /* layer for the time */

GFont *font_date;       /* font for date (normal) */
GFont *font_hour;       /* font for hour (bold) */
GFont *font_minute;     /* font for minute (thin) */


/* Called by the graphics layers when the time layer needs to be updated.
*/
void time_layer_update_proc(Layer *l, GContext* ctx)
{
    TimeLayer *tl = layer_get_data(l);

    tl->bounds = layer_get_bounds(l);

    if (tl->background_color != GColorClear)
    {
        graphics_context_set_fill_color(ctx, tl->background_color);
        graphics_fill_rect(ctx, tl->bounds, 0, GCornerNone);
    }

    graphics_context_set_text_color(ctx, tl->text_color);

    if (tl->hour_text && tl->minute_text)
    {
        GSize hour_sz =
            graphics_text_layout_get_max_used_size(ctx,
                                                   tl->hour_text,
                                                   tl->hour_font,
                                                   tl->bounds,
                                                   tl->overflow_mode,
                                                   GTextAlignmentLeft,
                                                   tl->layout_cache);
        GSize minute_sz =
            graphics_text_layout_get_max_used_size(ctx,
                                                   tl->minute_text,
                                                   tl->minute_font,
                                                   tl->bounds,
                                                   tl->overflow_mode,
                                                   GTextAlignmentLeft,
                                                   tl->layout_cache);
        int width = minute_sz.w + hour_sz.w;
        int half = tl->bounds.size.w / 2;
        GRect hour_bounds = tl->bounds;
        GRect minute_bounds = tl->bounds;

        hour_bounds.size.w = half - (width / 2) + hour_sz.w;
        minute_bounds.origin.x = hour_bounds.size.w + 1;
        minute_bounds.size.w = minute_sz.w;

        graphics_draw_text(ctx,
                           tl->hour_text,
                           tl->hour_font,
                           hour_bounds,
                           tl->overflow_mode,
                           GTextAlignmentRight,
                           tl->layout_cache);
        graphics_draw_text(ctx,
                           tl->minute_text,
                           tl->minute_font,
                           minute_bounds,
                           tl->overflow_mode,
                           GTextAlignmentLeft,
                           tl->layout_cache);
    }
}


/* Set the hour and minute text and mark the layer dirty. NOTE that the
* two strings must be static because we're only storing a pointer to them,
* not making a copy.
*/
void time_layer_set_text(TimeLayer *tl, char *hour_text, char *minute_text)
{
    tl->hour_text = hour_text;
    tl->minute_text = minute_text;

    layer_mark_dirty(tl->layer);
}


/* Set the time fonts. Hour and minute fonts can be different.
*/
void time_layer_set_fonts(TimeLayer *tl, GFont hour_font, GFont minute_font)
{
    tl->hour_font = hour_font;
    tl->minute_font = minute_font;

    if (tl->hour_text && tl->minute_text)
    {
        layer_mark_dirty(tl->layer);
    }
}


/* Set the text color of the time layer.
*/
void time_layer_set_text_color(TimeLayer *tl, GColor color)
{
    tl->text_color = color;

    if (tl->hour_text && tl->minute_text)
    {
        layer_mark_dirty(tl->layer);
    }
}


/* Set the background color of the time layer.
*/
void time_layer_set_background_color(TimeLayer *tl, GColor color)
{
    tl->background_color = color;

    if (tl->hour_text && tl->minute_text)
    {
        layer_mark_dirty(tl->layer);
    }
}


/* Get the underlying layer.
*/
Layer *time_layer_get_layer(TimeLayer *tl)
{
    return tl->layer;
}


/* Create a TimeLayer.
*/
TimeLayer *time_layer_create(GRect frame)
{
    Layer *l = layer_create_with_data(frame, sizeof(TimeLayer));
    TimeLayer *tl = (TimeLayer *)layer_get_data(l);

    tl->layer = l;
    layer_set_update_proc(tl->layer, time_layer_update_proc);

    tl->text_color = GColorWhite;
    tl->background_color = GColorClear;
    tl->overflow_mode = GTextOverflowModeWordWrap;

    tl->hour_font = fonts_get_system_font(FONT_KEY_GOTHIC_14_BOLD);
    tl->minute_font = tl->hour_font;

    return tl;
}


/* Destroy a TimeLayer.
*/
void time_layer_destroy(TimeLayer *tl)
{
    layer_destroy(tl->layer);
}


/* Called by the OS once per minute. Update the time and date.
*/
void handle_minute_tick(struct tm *tick_time, TimeUnits units_changed)
{
    /* Need to be static because pointers to them are stored in the text
    * layers.
    */
    static char date_text[] = "XXX, XXX 00";
    static char hour_text[] = "00";
    static char minute_text[] = ":00";

    if (units_changed & DAY_UNIT)
    {
        strftime(date_text, sizeof(date_text), "%a, %b %d",tick_time);
        text_layer_set_text(date_layer, date_text);
    }

    if (clock_is_24h_style())
    {
        strftime(hour_text, sizeof(hour_text), "%H", tick_time);
    }
    else
    {
        strftime(hour_text, sizeof(hour_text), "%I", tick_time);
        if (hour_text[0] == '0')
        {
            /* This is a hack to get rid of the leading zero.
            */
            hour_text[0] = hour_text[1];
            hour_text[1] = 0;
        }
    }

    strftime(minute_text, sizeof(minute_text), ":%M", tick_time);
    time_layer_set_text(time_layer, hour_text, minute_text);
}


/* Initialize the application.
*/
void app_init(void)
{
    time_t t = time(NULL);
    struct tm *tick_time = localtime(&t);
    TimeUnits units_changed = SECOND_UNIT | MINUTE_UNIT | HOUR_UNIT | DAY_UNIT;
    ResHandle res_d;
    ResHandle res_h;
    ResHandle res_m;
    Layer *window_layer;

    window = window_create();
    window_layer = window_get_root_layer(window);
    window_set_background_color(window, GColorBlack);

    res_d = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_CONDENSED_21);
    res_h = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_BOLD_SUBSET_53);
    res_m = resource_get_handle(RESOURCE_ID_FONT_ROBOTO_LIGHT_SUBSET_53);

    font_date = fonts_load_custom_font(res_d);
    font_hour = fonts_load_custom_font(res_h);
    font_minute = fonts_load_custom_font(res_m);

    time_layer = time_layer_create(TIME_FRAME);
    time_layer_set_text_color(time_layer, GColorWhite);
    time_layer_set_background_color(time_layer, GColorClear);
    time_layer_set_fonts(time_layer, font_hour, font_minute);
    layer_add_child(window_layer, time_layer_get_layer(time_layer));

    date_layer = text_layer_create(DATE_FRAME);
    text_layer_set_text_color(date_layer, GColorWhite);
    text_layer_set_background_color(date_layer, GColorClear);
    text_layer_set_font(date_layer, font_date);
    text_layer_set_text_alignment(date_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(date_layer));

    handle_minute_tick(tick_time, units_changed);
    window_stack_push(window, true);
    tick_timer_service_subscribe(MINUTE_UNIT, handle_minute_tick);
}


/* Shut down the application
*/
void app_term(void)
{
    tick_timer_service_unsubscribe();
    time_layer_destroy(time_layer);
    text_layer_destroy(date_layer);
    window_destroy(window);

    fonts_unload_custom_font(font_date);
    fonts_unload_custom_font(font_hour);
    fonts_unload_custom_font(font_minute);
}


/********************* Main Program *******************/

int main(void)
{
    app_init();
    app_event_loop();
    app_term();

    return 0;
}

