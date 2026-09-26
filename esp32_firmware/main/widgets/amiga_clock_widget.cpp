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

#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ble_services.h"
#include "board_pins.h"
#include "graphics.h"

namespace widgets {

static const char *TAG = "amiga_clock_widget";

// Amiga WB 1.2 Colors
const static lv_color_t AMIGA_BLUE   = lv_color_hex(0x0055aa);
const static lv_color_t AMIGA_BLACK  = lv_color_hex(0x000000);
const static lv_color_t AMIGA_WHITE  = lv_color_hex(0xffffff);
const static lv_color_t AMIGA_ORANGE = lv_color_hex(0xff8800);

// Height of a text line on the screen (Amiga font height is 8)
constexpr int TEXT_HEIGHT = 16;

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


struct ClockDimensions {
    lv_point_t center;

    lv_coord_t radius;
    lv_coord_t border_w;
    lv_coord_t border_space;
    lv_coord_t hand_min_top;
    lv_coord_t hand_min_bottom;
    lv_coord_t hand_min_width;
    lv_coord_t hand_hour_top;
    lv_coord_t hand_hour_bottom;
    lv_coord_t hand_hour_width;
    lv_coord_t tick_small_h;
    lv_coord_t tick_big_h;
    lv_coord_t tick_big_w;
};

static struct ClockDimensions clock_dims_without_date;
static struct ClockDimensions clock_dims_with_date;



static lv_timer_t *timer;


static bool show_date;
static bool show_seconds;

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
static void init_clock_dimensions(struct ClockDimensions *dim, lv_coord_t clock_width) {
    dim->center.x = graphics::CANVAS_W / 2;
    dim->center.y = CANVAS_BORDER + clock_width/2;

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


AmigaClockWidget::AmigaClockWidget() {
    static bool initialized = false;
    if (!initialized) {
        init_clock_dimensions(&clock_dims_without_date, graphics::CANVAS_W-CANVAS_BORDER*2);
        init_clock_dimensions(&clock_dims_with_date, graphics::CANVAS_W-CANVAS_BORDER-CLOCK_DATE_LINE_H);
        initialized = true;
    }

    show_date = true;
    show_seconds = true;
    pick_clock_dimensions();
}

AmigaClockWidget::~AmigaClockWidget() {
}

bool AmigaClockWidget::tick() {    
    struct tm t1, t2;
    localtime_r(&last_tick_time, &t1);
    last_tick_time = time(NULL);
    localtime_r(&last_tick_time, &t2);

    return t1.tm_sec != t2.tm_sec;
}

void AmigaClockWidget::render() {
    // Blue background
    graphics::clear(AMIGA_BLUE);

    // Clock face
    int x = clock_dims->center.x - clock_dims->radius;
    int y = clock_dims->center.y - clock_dims->radius;
    int w = clock_dims->radius * 2;

    graphics::fill_circle(x, y, w, w, AMIGA_WHITE, AMIGA_BLACK, clock_dims->border_w);

    // Draw min markers
    for (int i = 0; i < 60; i++) {
        if (i % 5 == 0) {
            draw_big_tick(i*6); // Draw the 5-minute marker
        } else {
            draw_tick(i*6); // Each minute marker is 6 degrees apart
        }    
    }

    struct tm t;
    localtime_r(&last_tick_time, &t);
    
    // Draw clock hands
    // Minute hand moves every 10 secs
    // Hour hand moves every 2 mins
    draw_hour_hand((t.tm_hour % 12) * 30 + t.tm_min/2);
    draw_minute_hand(t.tm_min * 6 + t.tm_sec/10);
    if (show_seconds) {
        draw_second_hand(t.tm_sec * 6);
    }

    if (show_date) {
        draw_date_line(t.tm_mday, t.tm_mon, t.tm_year + 1900);
    }

}

void AmigaClockWidget::set_property(const std::string &name, const std::string &value) {
    if (name == PROPERTY_SHOW_DATE) {
        show_date = (value == "true");
    } else if (name == PROPERTY_SHOW_SECONDS) {
        show_seconds = (value == "true");
    }
}

std::string AmigaClockWidget::get_property(const std::string &name) const {
    if (name == PROPERTY_SHOW_DATE) {
        return show_date ? "true" : "false";
    } else if (name == PROPERTY_SHOW_SECONDS) {
        return show_seconds ? "true" : "false";
    }
    return "";
}

std::vector<std::string> AmigaClockWidget::get_property_names() const {
    return { PROPERTY_SHOW_DATE, PROPERTY_SHOW_SECONDS };
}

void AmigaClockWidget::pick_clock_dimensions() {
    clock_dims = show_date ? &clock_dims_with_date : &clock_dims_without_date;
}

void widgets::AmigaClockWidget::draw_tick(double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    lv_coord_t top = -clock_dims->radius + clock_dims->border_w + clock_dims->border_space;
    lv_point_t line_points[2];
    line_points[0] = mv(rotate_point((lv_point_t){0,top}, rad), clock_dims->center);
    line_points[1] = mv(rotate_point((lv_point_t){0,(lv_coord_t)(top+clock_dims->tick_small_h)}, rad), clock_dims->center);
    graphics::draw_line(line_points, 2, AMIGA_BLACK, 2);
}

void widgets::AmigaClockWidget::draw_big_tick(double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    lv_coord_t top = -clock_dims->radius + clock_dims->border_w + clock_dims->border_space;
    lv_point_t poly_points[] = {
        {0,                   top},
        {(lv_coord_t)(clock_dims->tick_big_w/2),  (lv_coord_t)(top + clock_dims->tick_big_h/2)},
        {0,                   (lv_coord_t)(top + clock_dims->tick_big_h)},
        {(lv_coord_t)(-clock_dims->tick_big_w/2), (lv_coord_t)(top + clock_dims->tick_big_h/2)}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), clock_dims->center);
    }

    graphics::draw_filled_poly(poly_points, 4, AMIGA_BLACK);
}

void widgets::AmigaClockWidget::draw_minute_hand(double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    lv_coord_t top = -clock_dims->hand_min_top - clock_dims->hand_min_bottom;
    lv_point_t poly_points[] = {
        {0,                   top},
        {(lv_coord_t)(clock_dims->hand_min_width/2),  (lv_coord_t)(top + clock_dims->hand_min_top)},
        {0,                   (lv_coord_t)(top + clock_dims->hand_min_top + clock_dims->hand_min_bottom)},
        {(lv_coord_t)(-clock_dims->hand_min_width/2), (lv_coord_t)(top + clock_dims->hand_min_top)}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), clock_dims->center);
    }
    graphics::draw_filled_poly(poly_points, 4, AMIGA_BLACK);
}

void widgets::AmigaClockWidget::draw_hour_hand(double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    lv_coord_t top = -clock_dims->hand_hour_top - clock_dims->hand_hour_bottom;
    lv_point_t poly_points[] = {
        {0,                   top},
        {(lv_coord_t)(clock_dims->hand_hour_width/2),  (lv_coord_t)(top + clock_dims->hand_hour_top)},
        {0,                   (lv_coord_t)(top + clock_dims->hand_hour_top + clock_dims->hand_hour_bottom)},
        {(lv_coord_t)(-clock_dims->hand_hour_width/2), (lv_coord_t)(top + clock_dims->hand_hour_top)}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), clock_dims->center);
    }
    graphics::draw_filled_poly(poly_points, 4, AMIGA_BLACK);
}

void widgets::AmigaClockWidget::draw_second_hand(double angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    lv_coord_t top = (lv_coord_t)(-clock_dims->hand_min_top - clock_dims->hand_min_bottom);
    lv_point_t line_points[2];
    line_points[0] = mv(rotate_point((lv_point_t){0,top}, rad), clock_dims->center);
    line_points[1] = mv(rotate_point((lv_point_t){0,0}, rad), clock_dims->center);
    graphics::draw_line(line_points, 2, AMIGA_ORANGE, 2);
}

void widgets::AmigaClockWidget::draw_date_line(int day, int month, int year) {
    char date_str[11];
    snprintf(date_str, sizeof(date_str), "%2d %s %02d", day, month_names[month], year % 100);

    int pos_x = (graphics::CANVAS_W - 9*8) / 2;
    int pos_y = graphics::CANVAS_H - CLOCK_DATE_LINE_H/2 - TEXT_HEIGHT/2;
    
    graphics::draw_string(date_str, pos_x, pos_y, AMIGA_WHITE, AMIGA_BLUE);
}



} // namespace widgets
