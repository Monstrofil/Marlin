/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2023 MarlinFirmware
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "../../inc/MarlinConfigPre.h"
#include "../marlinui.h"
#include "ui_common.h"
#include "../../libs/numtostr.h"

#if ENABLED(TFT_COLOR_UI) && ENABLED(TOUCH_SCREEN)

template <typename Derived>
class QuickAccessButtonsBase {
  protected:
    static uint8_t page_index;
    static uint8_t page_count;
    static uint8_t last_drawn_page;

    static constexpr uint8_t columns() { return Derived::grid_columns; }
    static constexpr uint8_t rows()    { return Derived::grid_rows; }

    static constexpr uint16_t col_to_x(const uint8_t col) {
      return Derived::origin_x + uint16_t(col) * Derived::button_spacing_x;
    }
    static constexpr uint16_t row_to_y(const uint8_t row) {
      return Derived::origin_y + uint16_t(row) * Derived::button_spacing_y;
    }
    static constexpr uint16_t x_for_index(const uint8_t index) {
      return col_to_x((index % buttons_per_page()) % columns());
    }
    static constexpr uint16_t y_for_index(const uint8_t index) {
      return row_to_y((index % buttons_per_page()) / columns());
    }

    static constexpr uint8_t buttons_per_page() {
      return rows() * columns();
    }

    static void clear_buttons_area() {
      if (!columns() || !rows()) return;
      const uint16_t area_width  = columns() * Derived::button_spacing_x;
      const uint16_t area_height = rows()    * Derived::button_spacing_y;
      tft.canvas(Derived::origin_x, Derived::origin_y, area_width, area_height);
      tft.set_background(COLOR_BACKGROUND);
    }

    static void clamp_page() {
      static_assert(buttons_per_page() > 0, "Quick access grid must provide at least one button slot per page.");
      if (!page_count) page_count = 1;
      if (page_index >= page_count) page_index = page_count - 1;
    }

    static void set_page_count(const uint8_t new_pages) {
      page_count = new_pages ? new_pages : 1;
      clamp_page();
      last_drawn_page = 0xFF;
    }

    static uint8_t get_page_count() {
      if (!page_count) page_count = 1;
      return page_count;
    }

    static void draw_nav() {
      const bool enable = get_page_count() > 1;
      const uint8_t nav_row = rows();
      add_control(
        col_to_x(0), row_to_y(nav_row), BUTTON,
        (intptr_t)prev_page_touch, imgLeft,
        enable, COLOR_CONTROL_ENABLED, COLOR_CONTROL_DISABLED
      );
      add_control(
        col_to_x(columns()) - 64, row_to_y(nav_row), BUTTON,
        (intptr_t)next_page_touch, imgRight,
        enable, COLOR_CONTROL_ENABLED, COLOR_CONTROL_DISABLED
      );
    }

    static void draw_indicator() {
      const uint8_t indicator_row = rows();
      const uint8_t total_pages = get_page_count();
      const bool has_multiple_pages = total_pages > 1;

      tft.canvas(0, row_to_y(indicator_row), TFT_WIDTH, FONT_LINE_HEIGHT);
      tft.set_background(COLOR_BACKGROUND);

      if (has_multiple_pages) {
        tft_string.set(ui8tostr2(page_index + 1));
        tft_string.add('/');
        tft_string.add(ui8tostr2(total_pages));
        tft_string.trim();
        const uint16_t cursor_y = (FONT_LINE_HEIGHT > tft_string.font_height())
          ? (FONT_LINE_HEIGHT - tft_string.font_height()) / 2
          : 0;
        tft.add_text(
          tft_string.center(TFT_WIDTH),
          cursor_y,
          COLOR_LIGHT_BLUE,
          tft_string
        );
      }
    }

    static void next_page_impl() {
      const uint8_t total_pages = get_page_count();
      if (total_pages <= 1) return;
      page_index = (page_index + 1) % total_pages;
      ui.refresh();
    }

    static void prev_page_impl() {
      const uint8_t total_pages = get_page_count();
      if (total_pages <= 1) return;
      page_index = page_index ? page_index - 1 : total_pages - 1;
      ui.refresh();
    }

    static void next_page_touch() { next_page_impl(); }
    static void prev_page_touch() { prev_page_impl(); }

  public:
    template <typename... Args>
    static void draw(Args... args) {
      clamp_page();

      const bool page_changed = page_index != last_drawn_page;
      if (page_changed)
        clear_buttons_area();

      Derived::draw_page(page_index, args...);

      draw_indicator();
      draw_nav();

      last_drawn_page = page_index;
    }

    static void next_page() { next_page_impl(); }
    static void prev_page() { prev_page_impl(); }
    static void reset() {
      page_index = 0;
      last_drawn_page = 0xFF;
    }
};

template <typename Derived> uint8_t QuickAccessButtonsBase<Derived>::page_index = 0;
template <typename Derived> uint8_t QuickAccessButtonsBase<Derived>::page_count = 1;
template <typename Derived> uint8_t QuickAccessButtonsBase<Derived>::last_drawn_page = 0xFF;

#define QUICK_ACCESS_BEGIN(NAME, ORIGIN_X, ORIGIN_Y, SPACING_X, SPACING_Y, GRID_COLS, GRID_ROWS, ...) \
  struct NAME : QuickAccessButtonsBase<NAME> { \
    static constexpr uint16_t origin_x = ORIGIN_X; \
    static constexpr uint16_t origin_y = ORIGIN_Y; \
    static constexpr uint16_t button_spacing_x = SPACING_X; \
    static constexpr uint16_t button_spacing_y = SPACING_Y; \
    static constexpr uint8_t grid_columns = GRID_COLS; \
    static constexpr uint8_t grid_rows = GRID_ROWS; \
    static_assert(grid_columns > 0, "Quick access grid must have at least one column."); \
    static_assert(grid_rows > 0, "Quick access grid must have at least two rows (controls + navigation)."); \
    using Base = QuickAccessButtonsBase<NAME>; \
    using Base::draw; \
    static void draw_page(uint8_t page, ##__VA_ARGS__) { \
      constexpr uint8_t __qa_buttons_per_page = Base::buttons_per_page(); \
      static_assert(__qa_buttons_per_page > 0, "Quick access grid must provide at least one button slot per page."); \
      uint8_t __qa_button_index = 0;

#define QUICK_ACCESS_BUTTON_BEGIN() \
      do { \
        const uint8_t __qa_button_page = __qa_button_index / __qa_buttons_per_page; \
        if (__qa_button_page == page) { \
          const uint16_t BTN_X = Base::x_for_index(__qa_button_index); \
          const uint16_t BTN_Y = Base::y_for_index(__qa_button_index);

#define QUICK_ACCESS_BUTTON_END() \
        } \
        ++__qa_button_index; \
      } while (0)

#define QUICK_ACCESS_END(NAME) \
      const uint8_t __qa_total_buttons = __qa_button_index; \
      const uint8_t __qa_total_pages = __qa_buttons_per_page ? uint8_t((__qa_total_buttons + __qa_buttons_per_page - 1) / __qa_buttons_per_page) : 1; \
      Base::set_page_count(__qa_total_pages); \
    } \
  };

#endif // ENABLED(TFT_COLOR_UI) && ENABLED(TOUCH_SCREEN)

