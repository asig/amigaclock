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
 
#include "amiga_clock_ui.h"

#include <math.h>
#include <stdatomic.h>
#include <time.h>

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "lvgl.h"

#include "ble_services.h"
#include "board_pins.h"

static const char *TAG = "amiga_clock_ui";

// Amiga WB 1.2 Colors
#define AMIGA_BLUE   lv_color_hex(0x0055aa) 
#define AMIGA_BLACK  lv_color_hex(0x000000)
#define AMIGA_WHITE  lv_color_hex(0xffffff)
#define AMIGA_ORANGE lv_color_hex(0xff8800) 

// Canvas dimensions
#define CANVAS_W LCD_H_RES
#define CANVAS_H LCD_V_RES

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

struct clock_dimensions {
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

static lv_obj_t *s_canvas;
static lv_color_t *s_canvas_buf;
static lv_timer_t *s_timer;

static struct clock_dimensions clock_dims_without_date;
static struct clock_dimensions clock_dims_with_date;

static struct clock_dimensions *current_clock_dims;
static bool show_date;
static bool show_seconds;

// Initialize clock dimensions based on the actual clock size.
static void init_clock_dimensions(struct clock_dimensions *dim, lv_coord_t clock_width) {
    dim->center.x = CANVAS_W / 2;
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
        pt.x + offset.x,
        pt.y + offset.y
    };
}

static void draw_filled_poly(const lv_point_t *points, uint16_t point_count, lv_color_t color) {
    lv_draw_rect_dsc_t poly_dsc;
    lv_draw_rect_dsc_init(&poly_dsc);

    poly_dsc.bg_color = color;
    poly_dsc.bg_opa = LV_OPA_COVER;

    lv_canvas_draw_polygon(s_canvas, points, point_count, &poly_dsc);
}

static void draw_line(lv_point_t *points, int count, lv_color_t color, lv_coord_t width) {
    lv_draw_line_dsc_t line_dsc;
    lv_draw_line_dsc_init(&line_dsc);
    line_dsc.color = color;
    line_dsc.width = width;
    line_dsc.round_start = false;
    line_dsc.round_end = false;

    lv_canvas_draw_line(s_canvas, points, count, &line_dsc);
}

static void draw_tick(double_t angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    int top = -current_clock_dims->radius + current_clock_dims->border_w + current_clock_dims->border_space;
    lv_point_t line_points[2];
    line_points[0] = mv(rotate_point((lv_point_t){0,top}, rad), current_clock_dims->center);
    line_points[1] = mv(rotate_point((lv_point_t){0,top+current_clock_dims->tick_small_h}, rad), current_clock_dims->center);
    draw_line(line_points, 2, AMIGA_BLACK, 2);
}

static void draw_big_tick(double_t angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    int top = -current_clock_dims->radius + current_clock_dims->border_w + current_clock_dims->border_space;
    lv_point_t poly_points[] = {
        {0,                   top},
        {current_clock_dims->tick_big_w/2,  top + current_clock_dims->tick_big_h/2},
        {0,                   top + current_clock_dims->tick_big_h},
        {-current_clock_dims->tick_big_w/2, top + current_clock_dims->tick_big_h/2}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), current_clock_dims->center);
    }

    draw_filled_poly(poly_points, 4, AMIGA_BLACK);
}

static void draw_minute_hand(double_t angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    int top = -current_clock_dims->hand_min_top - current_clock_dims->hand_min_bottom;
    lv_point_t poly_points[] = {
        {0,                   top},
        {current_clock_dims->hand_min_width/2,  top + current_clock_dims->hand_min_top},
        {0,                   top + current_clock_dims->hand_min_top + current_clock_dims->hand_min_bottom},
        {-current_clock_dims->hand_min_width/2, top + current_clock_dims->hand_min_top}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), current_clock_dims->center);
    }
    draw_filled_poly(poly_points, 4, AMIGA_BLACK);
}

static void draw_hour_hand(double_t angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    // poly points with offsets to center, so that we can just rotate them
    int top = -current_clock_dims->hand_hour_top - current_clock_dims->hand_hour_bottom;
    lv_point_t poly_points[] = {
        {0,                   top},
        {current_clock_dims->hand_hour_width/2,  top + current_clock_dims->hand_hour_top},
        {0,                   top + current_clock_dims->hand_hour_top + current_clock_dims->hand_hour_bottom},
        {-current_clock_dims->hand_hour_width/2, top + current_clock_dims->hand_hour_top}
    };
    for (int i = 0; i < 4; i++) {
        poly_points[i] = mv(rotate_point(poly_points[i], rad), current_clock_dims->center);
    }
    draw_filled_poly(poly_points, 4, AMIGA_BLACK);
}

static void draw_second_hand(double_t angle_deg) {
    double rad = angle_deg * M_PI / 180.0;

    int top = -current_clock_dims->hand_min_top - current_clock_dims->hand_min_bottom;
    lv_point_t line_points[2];
    line_points[0] = mv(rotate_point((lv_point_t){0,top}, rad), current_clock_dims->center);
    line_points[1] = mv(rotate_point((lv_point_t){0,0}, rad), current_clock_dims->center);
    draw_line(line_points, 2, AMIGA_ORANGE, 2);
}

static void draw_clock() {
    // Blue background
    lv_canvas_fill_bg(s_canvas, AMIGA_BLUE, LV_OPA_COVER);

    // Clock face
    lv_draw_rect_dsc_t dsc;
    lv_draw_rect_dsc_init(&dsc);
    dsc.radius = LV_RADIUS_CIRCLE;
    dsc.bg_color = AMIGA_WHITE;
    dsc.bg_opa = LV_OPA_COVER;
    dsc.border_color = AMIGA_BLACK;
    dsc.border_width = current_clock_dims->border_w;
    dsc.border_opa = LV_OPA_COVER;
    int x = current_clock_dims->center.x - current_clock_dims->radius;
    int y = current_clock_dims->center.y - current_clock_dims->radius;
    int w = current_clock_dims->radius * 2;
    lv_canvas_draw_rect(s_canvas, x, y, w, w, &dsc);

    // Draw min markers
    for (int i = 0; i < 60; i++) {
        if (i % 5 == 0) {
            draw_big_tick(i*6); // Draw the 5-minute marker
        } else {
            draw_tick(i*6); // Each minute marker is 6 degrees apart
        }    
    }

    time_t now = time(NULL);
    struct tm t;
    localtime_r(&now, &t);
    
    // Draw clock hands
    // Minute hand moves every 10 secs
    // Hour hand moves every 2 mins
    draw_hour_hand((t.tm_hour % 12) * 30 + t.tm_min/2);
    draw_minute_hand(t.tm_min * 6 + t.tm_sec/10);
    if (show_seconds) {
        draw_second_hand(t.tm_sec * 6);
    }
}

static void clock_timer_cb(lv_timer_t *) {
    draw_clock();
    lv_obj_invalidate(s_canvas);
}

void amiga_clock_ui_create() {
    ESP_LOGI(TAG, "Creating Amiga clock UI.");

    ESP_LOGI(TAG, "Configuring clock dimenstions.");
    init_clock_dimensions(&clock_dims_without_date, CANVAS_W-CANVAS_BORDER*2);
    init_clock_dimensions(&clock_dims_with_date, CANVAS_W-CANVAS_BORDER*2-CLOCK_DATE_LINE_H);

    amiga_clock_ui_configure(false /*show_date*/, true /*show_seconds*/);

    ESP_LOGI(TAG, "Allocating canvas buffer.");
    size_t buf_size = CANVAS_W * CANVAS_H * sizeof(lv_color_t);
    s_canvas_buf = heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM);

    ESP_LOGI(TAG, "Creating canvas.");
    s_canvas = lv_canvas_create(lv_scr_act());
    lv_canvas_set_buffer(s_canvas, s_canvas_buf, CANVAS_W, CANVAS_H, LV_IMG_CF_TRUE_COLOR);
    lv_obj_center(s_canvas);

    draw_clock();
    ESP_LOGI(TAG, "Initial clock drawn.");

    ESP_LOGI(TAG, "Starting clock timer.");
    s_timer = lv_timer_create(clock_timer_cb, 1000, NULL);
    ESP_LOGI(TAG, "Clock timer started.");
}

void amiga_clock_ui_configure(bool show_date_param, bool show_seconds_param) {
    show_date = show_date_param;
    show_seconds = show_seconds_param;
    current_clock_dims = show_date ? &clock_dims_with_date : &clock_dims_without_date;
}

void amiga_clock_ui_get_config(bool *show_date_param, bool *show_seconds_param) {
    if (show_date_param != NULL) {
        *show_date_param = show_date;
    }
    if (show_seconds_param != NULL) {
        *show_seconds_param = show_seconds;
    }
}
