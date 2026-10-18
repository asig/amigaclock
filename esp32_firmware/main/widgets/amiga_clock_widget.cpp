/*
 * This file is part of AmigaClock.
 *
 * Copyright (C) 2026 Andreas Signer <asigner@gmail.com>
 *
 * AmigaClock is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * AmigaClock is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with AmigaClock.  If not, see <https://www.gnu.org/licenses/>.
 */
 
#include "widgets/amiga_clock_widget.h"

#include <math.h>
#include <stdatomic.h>
#include <time.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ble_services.h"
#include "board_pins.h"

namespace widgets {

namespace {

constexpr const char *const TAG = "amiga_clock_widget";

constexpr int HIRES_SCALE_DIVIDER = 2;
constexpr int LORES_SCALE_DIVIDER = 4;

// Height of a text line on the screen (Amiga font height is 8)
constexpr int TEXT_HEIGHT = 16;

// Amiga WB 1.2 Colors
const static lv_color_t AMIGA_BLUE   = lv_color_hex(0x0055aa);
const static lv_color_t AMIGA_BLACK  = lv_color_hex(0x000000);
const static lv_color_t AMIGA_WHITE  = lv_color_hex(0xffffff);
const static lv_color_t AMIGA_ORANGE = lv_color_hex(0xff8800);

canvas::SoftwareRasterizer software_rasterizer;
canvas::LvglRasterizer lvgl_rasterizer;

}

// Clock dimensions. 
// Raw values, "reverse-engineered" from Amiberry screenshots :-)
#define CLOCK_RAW_RADIUS 560
#define CLOCK_RAW_BORDER_W 15
#define CLOCK_RAW_BORDER_SPACE 20
#define CLOCK_RAW_HAND_MIN_TOP 70
#define CLOCK_RAW_HAND_MIN_BOTTOM 385
#define CLOCK_RAW_HAND_MIN_WIDTH 42
#define CLOCK_RAW_HAND_HOUR_TOP 68
#define CLOCK_RAW_HAND_HOUR_BOTTOM 245
#define CLOCK_RAW_HAND_HOUR_WIDTH 55
#define CLOCK_RAW_TICK_SMALL_H 55
#define CLOCK_RAW_TICK_BIG_H 74
#define CLOCK_RAW_TICK_BIG_W 52

#define CLOCK_DATE_LINE_H 40

// Clock dimensions scaled to the actual display resolution, including spacing
#define CANVAS_BORDER 20


static const char * const month_names[12] = {
    "Jan",
    "Feb",
    "Mar",
    "Apr",
    "May",
    "Jun",
    "Jul",
    "Aug",
    "Sep",
    "Oct",
    "Nov",
    "Dec"
};

// Initialize clock dimensions based on the actual clock size.
static void init_clock_dimensions(struct ClockDimensions *dim, lv_coord_t clock_width, int tick_line_width, int second_hand_width) {
    dim->radius = clock_width / 2;
    dim->border_w = CLOCK_RAW_BORDER_W * dim->radius / CLOCK_RAW_RADIUS;
    dim->border_space = CLOCK_RAW_BORDER_SPACE * dim->radius / CLOCK_RAW_RADIUS;
    dim->hand_min_top = CLOCK_RAW_HAND_MIN_TOP * dim->radius / CLOCK_RAW_RADIUS;
    dim->hand_min_bottom = CLOCK_RAW_HAND_MIN_BOTTOM * dim->radius / CLOCK_RAW_RADIUS;
    dim->hand_min_width = CLOCK_RAW_HAND_MIN_WIDTH * dim->radius / CLOCK_RAW_RADIUS;
    dim->hand_hour_top = CLOCK_RAW_HAND_HOUR_TOP * dim->radius / CLOCK_RAW_RADIUS;
    dim->hand_hour_bottom = CLOCK_RAW_HAND_HOUR_BOTTOM * dim->radius / CLOCK_RAW_RADIUS;
    dim->hand_hour_width = CLOCK_RAW_HAND_HOUR_WIDTH * dim->radius / CLOCK_RAW_RADIUS;
    dim->tick_small_h = CLOCK_RAW_TICK_SMALL_H * dim->radius / CLOCK_RAW_RADIUS;
    dim->tick_big_h = CLOCK_RAW_TICK_BIG_H * dim->radius / CLOCK_RAW_RADIUS;
    dim->tick_big_w = CLOCK_RAW_TICK_BIG_W * dim->radius / CLOCK_RAW_RADIUS;
    dim->tick_line_width = tick_line_width;
    dim->second_hand_width = second_hand_width;
}

// Rotates clockwise by the given angle in radians.
static lv_point_t rotate_point(lv_point_t p, double rad) {
    return (lv_point_t){
        (lv_coord_t)(p.x * cos(rad) - p.y * sin(rad)),
        (lv_coord_t)(p.x * sin(rad) + p.y * cos(rad))
    };
}

static lv_point_t mv(lv_point_t pt, lv_point_t offset) {
    return (lv_point_t){
        (lv_coord_t)(pt.x + offset.x),
        (lv_coord_t)(pt.y + offset.y)
    };
}

static time_t last_tick_time = 0;

const std::string widgets::AmigaClockWidget::PROPERTY_SHOW_DATE = "show_date";
const std::string widgets::AmigaClockWidget::PROPERTY_SHOW_SECONDS = "show_seconds";
const std::string widgets::AmigaClockWidget::PROPERTY_RESOLUTION = "resolution";
const std::string widgets::AmigaClockWidget::PROPERTY_ANTIALIAS = "antialias";

AmigaClockWidget::AmigaClockWidget() {
    show_date = true;
    show_seconds = true;
    set_resolution(Resolution::SuperHiRes);
    antialias = false;

    update_rasterizer();
    update_clock_dimensions();

    offscreen = lv_obj_create(NULL);
    clockface_canvas = new Canvas(LCD_H_RES, LCD_V_RES, offscreen);

    screen_canvas = new Canvas(LCD_H_RES, LCD_V_RES, lv_scr_act());
    lv_obj_center(screen_canvas->canvas());
}

AmigaClockWidget::~AmigaClockWidget() {
    delete screen_canvas;
    delete clockface_canvas;
    lv_obj_del(offscreen);
}

bool AmigaClockWidget::tick() {    
    struct tm t1, t2;
    localtime_r(&last_tick_time, &t1);
    last_tick_time = time(NULL);
    localtime_r(&last_tick_time, &t2);

    return t1.tm_sec != t2.tm_sec;
}

void AmigaClockWidget::render() {        
    screen_canvas->clear(AMIGA_BLUE);

    struct tm t;
    localtime_r(&last_tick_time, &t);


    this->draw_clockface(clockface_canvas, t);

    // Now upscale clockface
    const lv_color_t *src = clockface_canvas->buffer();
    const int src_width = clockface_canvas->width();
    const int logical_width = src_width / scale_divider;
    const int logical_height = clockface_canvas->height() / scale_divider;

    lv_color_t *dst = screen_canvas->buffer();
    const int dst_width = screen_canvas->width();
    const int dst_offset_x = (LCD_H_RES - logical_width * scale_divider) / 2;
    for (int y = 0; y < logical_height; y++) {
        for (int x = 0; x < logical_width; x++) {
            const lv_color_t color = src[y * src_width + x];
            for (int i = 0; i < scale_divider; i++) {
                const int dst_y = y * scale_divider + i;
                for (int j = 0; j < scale_divider; j++) {
                    const int dst_x = x * scale_divider + j + dst_offset_x;
                    dst[dst_y * dst_width  + dst_x] = color;
                }
            }
        }
    }

    // Show date line if enabled
    if (show_date) {
        draw_date_line(t.tm_mday, t.tm_mon, t.tm_year + 1900);
    }
    
    screen_canvas->invalidate();
}

void AmigaClockWidget::draw_clockface(Canvas *c, const struct tm &t) {    
    c->clear(AMIGA_BLUE);

    int radius = clock_dims.radius;
    rasterizer->fill_circle(c, clock_center.x, clock_center.y, radius, AMIGA_BLACK);
    rasterizer->fill_circle(c, clock_center.x, clock_center.y, radius - clock_dims.border_w, AMIGA_WHITE);

    // Draw min markers
    for (int i = 0; i < 60; i++) {
        if (i % 5 == 0) {
            draw_big_tick(c, i*6); // Draw the 5-minute marker
        } else {
            draw_tick(c, i*6); // Each minute marker is 6 degrees apart
        }    
    }
    
    // Draw clock hands
    // Minute hand moves every 10 secs
    // Hour hand moves every 2 mins
    draw_hour_hand(c, (t.tm_hour % 12) * 30 + t.tm_min/2);
    draw_minute_hand(c, t.tm_min * 6 + t.tm_sec/10);
    if (show_seconds) {
        draw_second_hand(c, t.tm_sec * 6);
    }
}

void AmigaClockWidget::set_property(const std::string &name, const std::string &value) {
    ESP_LOGI(TAG, "Setting property %s to %s", name.c_str(), value.c_str());
    if (name == PROPERTY_SHOW_DATE) {
        show_date = (value == "true");
    } else if (name == PROPERTY_SHOW_SECONDS) {
        show_seconds = (value == "true");
    } else if (name == PROPERTY_RESOLUTION) {
        if (value == "super-hi-res") {
            set_resolution(Resolution::SuperHiRes);
        } else if (value == "hi-res") {
            set_resolution(Resolution::HiRes);
        } else if (value == "lo-res") {
            set_resolution(Resolution::LoRes);
        }
    } else if (name == PROPERTY_ANTIALIAS) {
        antialias = (value == "true");
        update_rasterizer();
    }
    update_clock_dimensions();
}

std::string AmigaClockWidget::get_property(const std::string &name) const {
    std::string res;
    if (name == PROPERTY_SHOW_DATE) {
        res = show_date ? "true" : "false";
    } else if (name == PROPERTY_SHOW_SECONDS) {
        res = show_seconds ? "true" : "false";
    } else if (name == PROPERTY_RESOLUTION) {
        switch (resolution) {
            case Resolution::SuperHiRes: res = "super-hi-res"; break;
            case Resolution::HiRes: res = "hi-res"; break;
            case Resolution::LoRes: res = "lo-res"; break;
        }
    } else if (name == PROPERTY_ANTIALIAS) {
        res = antialias ? "true" : "false";
    }
    ESP_LOGI(TAG, "Getting property %s: %s", name.c_str(), res.c_str());
    return res.c_str();
}

std::vector<std::string> AmigaClockWidget::get_property_names() const {
    return { PROPERTY_SHOW_DATE, PROPERTY_SHOW_SECONDS, PROPERTY_RESOLUTION, PROPERTY_ANTIALIAS };
}

void AmigaClockWidget::set_resolution(Resolution resolution) {
    this->resolution = resolution;
    switch (resolution) {
        case Resolution::SuperHiRes:
            scale_divider = 1;
            break;
        case Resolution::HiRes:
            scale_divider = HIRES_SCALE_DIVIDER;
            break;
        case Resolution::LoRes:
            scale_divider = LORES_SCALE_DIVIDER;
            break;
    }
}

void AmigaClockWidget::update_rasterizer() {
    rasterizer = antialias ? static_cast<canvas::Rasterizer *>(&lvgl_rasterizer)
                          : static_cast<canvas::Rasterizer *>(&software_rasterizer);
}

void AmigaClockWidget::update_clock_dimensions() {
    int clock_width = LCD_H_RES - CANVAS_BORDER - (show_date ? CLOCK_DATE_LINE_H : 0);
    int tick_width = 2;
    int second_hand_width = 2;
    switch (resolution) {
        case Resolution::SuperHiRes:
            clock_width = LCD_H_RES - CANVAS_BORDER - (show_date ? CLOCK_DATE_LINE_H : 0);
            tick_width = 2;
            second_hand_width = 2;
            break;
        case Resolution::HiRes:
            clock_width = (LCD_H_RES - (show_date ? CANVAS_BORDER + CLOCK_DATE_LINE_H : CANVAS_BORDER * 2)) /
                          HIRES_SCALE_DIVIDER;
            tick_width = 1;
            second_hand_width = 1;
            break;
        case Resolution::LoRes:
            clock_width = (LCD_H_RES - (show_date ? CANVAS_BORDER + CLOCK_DATE_LINE_H : CANVAS_BORDER * 2)) /
                          LORES_SCALE_DIVIDER;
            tick_width = 1;
            second_hand_width = 1;
            break;
    }
    init_clock_dimensions(&clock_dims, clock_width, tick_width, second_hand_width);

    clock_center = (lv_point_t){
        (lv_coord_t)((LCD_H_RES / scale_divider) / 2),
        (lv_coord_t)(clock_dims.radius + CANVAS_BORDER / scale_divider)
    };
}

void AmigaClockWidget::draw_tick(Canvas *c, double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    lv_coord_t top = -clock_dims.radius + clock_dims.border_w + clock_dims.border_space;
    lv_point_t line_points[2];
    line_points[0] = mv(rotate_point((lv_point_t){0,top}, rad), clock_center);
    line_points[1] = mv(rotate_point((lv_point_t){0,(lv_coord_t)(top+clock_dims.tick_small_h)}, rad), clock_center);
    rasterizer->draw_line(c, line_points, 2, AMIGA_BLACK, clock_dims.tick_line_width);
}

void AmigaClockWidget::draw_big_tick(Canvas *c, double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    lv_coord_t top = -clock_dims.radius + clock_dims.border_w + clock_dims.border_space;
    lv_point_t poly_points[] = {
        {0,                   top},
        {(lv_coord_t)(clock_dims.tick_big_w/2),  (lv_coord_t)(top + clock_dims.tick_big_h/2)},
        {0,                   (lv_coord_t)(top + clock_dims.tick_big_h)},
        {(lv_coord_t)(-clock_dims.tick_big_w/2), (lv_coord_t)(top + clock_dims.tick_big_h/2)}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), clock_center);
    }

    rasterizer->draw_filled_poly(c, poly_points, 4, AMIGA_BLACK);
}

void AmigaClockWidget::draw_minute_hand(Canvas *c, double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    lv_coord_t top = -clock_dims.hand_min_top - clock_dims.hand_min_bottom;
    lv_point_t poly_points[] = {
        {0,                   top},
        {(lv_coord_t)(clock_dims.hand_min_width/2),  (lv_coord_t)(top + clock_dims.hand_min_top)},
        {0,                   (lv_coord_t)(top + clock_dims.hand_min_top + clock_dims.hand_min_bottom)},
        {(lv_coord_t)(-clock_dims.hand_min_width/2), (lv_coord_t)(top + clock_dims.hand_min_top)}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), clock_center);
    }
    rasterizer->draw_filled_poly(c, poly_points, 4, AMIGA_BLACK);
}

void AmigaClockWidget::draw_hour_hand(Canvas *c, double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    lv_coord_t top = -clock_dims.hand_hour_top - clock_dims.hand_hour_bottom;
    lv_point_t poly_points[] = {
        {0,                   top},
        {(lv_coord_t)(clock_dims.hand_hour_width/2),  (lv_coord_t)(top + clock_dims.hand_hour_top)},
        {0,                   (lv_coord_t)(top + clock_dims.hand_hour_top + clock_dims.hand_hour_bottom)},
        {(lv_coord_t)(-clock_dims.hand_hour_width/2), (lv_coord_t)(top + clock_dims.hand_hour_top)}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), clock_center);
    }
    rasterizer->draw_filled_poly(c, poly_points, 4, AMIGA_BLACK);
}

void AmigaClockWidget::draw_second_hand(Canvas *c, double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    lv_coord_t top = (lv_coord_t)(-clock_dims.hand_min_top - clock_dims.hand_min_bottom);
    lv_point_t line_points[2];
    line_points[0] = mv(rotate_point((lv_point_t){0,top}, rad), clock_center);
    line_points[1] = mv(rotate_point((lv_point_t){0,0}, rad), clock_center);
    rasterizer->draw_line(c, line_points, 2, AMIGA_ORANGE, clock_dims.second_hand_width);
}

void AmigaClockWidget::draw_date_line(int day, int month, int year) {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%2d %s %02d", day, month_names[month], year % 100);

    int pos_x = (LCD_H_RES - 9*8) / 2;
    int pos_y = LCD_V_RES - CLOCK_DATE_LINE_H/2 - TEXT_HEIGHT/2;
    
    screen_canvas->draw_string(date_str, pos_x, pos_y, AMIGA_WHITE, AMIGA_BLUE);
}

} // namespace widgets
