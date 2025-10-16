#include "eadk.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

// App metadata for EADK
const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Game of life";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

// Grid dimensions and pixel size
#define SIZE_GRID_X 320
#define SIZE_GRID_Y 240
uint8_t size_px = 1;

// Pattern identifiers
#define TETROMINO 1
#define GOSPER_GUN 2
#define ACORN 3
#define SIMKIN_GUN 4

// Bit-packed grid size: each cell is 1 bit
#define GRID_CELLS ((SIZE_GRID_X * SIZE_GRID_Y + 7) / 8)

// Two bit-packed buffers and changed bitmap
static uint8_t grid_buf1[GRID_CELLS];
static uint8_t grid_buf2[GRID_CELLS];
static uint8_t changed_cells[GRID_CELLS];

// Pointers to current (src) and next (dst) generation buffers
static uint8_t *src = grid_buf1;
static uint8_t *dst = grid_buf2;

// Bit manipulation helpers
static inline size_t cell_index(int x, int y) { return (size_t)y * SIZE_GRID_X + (size_t)x; }
static inline void bit_set(uint8_t *buf, size_t idx) { buf[idx >> 3] |= (uint8_t)(1u << (idx & 7));}
static inline void bit_clear(uint8_t *buf, size_t idx) { buf[idx >> 3] &= (uint8_t)~(1u << (idx & 7));}
static inline void bit_toggle(uint8_t *buf, size_t idx) { buf[idx >> 3] ^= (uint8_t)(1u << (idx & 7));}
static inline uint8_t bit_get(const uint8_t *buf, size_t idx) { return (buf[idx >> 3] >> (idx & 7)) & 1u;}

// Global variables
uint8_t live_neighbors;
uint8_t cursor[2] = {160, 120}; // Cursor starts at center
int8_t move_direction[] = {0, 0}; // Movement vector
uint8_t new_x, new_y;
bool running = false;        // auto-run flag, toggled by backspace


static const int tetromino[][2] = { {0,0},{1,0},{2,0},{2,-1},{1,1} };
static const int glider_gun[][2] = {
  {0,4},{0,5},{1,4},{1,5},
  {10,4},{10,5},{10,6},{11,3},{11,7},{12,2},{12,8},{13,2},{13,8},{14,5},
  {15,3},{15,7},{16,4},{16,5},{16,6},{17,5},
  {20,2},{20,3},{20,4},{21,2},{21,3},{21,4},{22,1},{22,5},{24,0},{24,1},{24,5},{24,6},
  {34,2},{34,3},{35,2},{35,3} };
static const int acorn[][2] = { {0,0},{1,0},{1,2},{3,1},{4,0},{5,0},{6,0} };
static const int simkin[][2] = {
  {0,0},{1,0},{2,0},{2,1},{1,2},{0,2},
  {10,0},{10,1},{11,0},{11,1},
  {20,0},{21,0},{22,0},{22,1},{21,2},{20,2},
  {30,0},{30,1},{31,0},{31,1},
  {15,5},{16,5},{17,5},{17,6},{16,7},{15,7},
  {25,5},{25,6},{26,5},{26,6},
  {35,5},{36,5},{37,5},{37,6},{36,7},{35,7} };

// Initialize grid to all dead cells
void init_grid(void) {
  memset(grid_buf1, 0, GRID_CELLS);
  memset(grid_buf2, 0, GRID_CELLS);
  memset(changed_cells, 0, GRID_CELLS);
  src = grid_buf1;
  dst = grid_buf2;
}

// Apply Game of Life rules to update the grid using src->dst and swap
void update_grid(void) {
  memset(changed_cells, 0, GRID_CELLS); // Clear change tracker

  for (int y = 0; y < SIZE_GRID_Y; ++y) {
    int y_up = (y == 0) ? (SIZE_GRID_Y - 1) : (y - 1);
    int y_down = (y == SIZE_GRID_Y - 1) ? 0 : (y + 1);

    for (int x = 0; x < SIZE_GRID_X; ++x) {
      int x_left = (x == 0) ? (SIZE_GRID_X - 1) : (x - 1);
      int x_right = (x == SIZE_GRID_X - 1) ? 0 : (x + 1);

      size_t idx        = cell_index(x, y);
      size_t idx_up_left    = cell_index(x_left, y_up);
      size_t idx_up        = cell_index(x, y_up);
      size_t idx_up_right  = cell_index(x_right, y_up);
      size_t idx_left      = cell_index(x_left, y);
      size_t idx_right     = cell_index(x_right, y);
      size_t idx_down_left = cell_index(x_left, y_down);
      size_t idx_down      = cell_index(x, y_down);
      size_t idx_down_right= cell_index(x_right, y_down);

      uint8_t neighbors =
        bit_get(src, idx_up_left) +
        bit_get(src, idx_up) +
        bit_get(src, idx_up_right) +
        bit_get(src, idx_left) +
        bit_get(src, idx_right) +
        bit_get(src, idx_down_left) +
        bit_get(src, idx_down) +
        bit_get(src, idx_down_right);

      uint8_t cur = bit_get(src, idx);
      uint8_t next = 0;

      if (neighbors == 3) {
        next = 1;
      } else if (neighbors == 2 && cur) {
        next = 1;
      }

      if (next) {
        bit_set(dst, idx);
      } else {
        bit_clear(dst, idx);
      }

      if (next != cur) {
        bit_set(changed_cells, idx); // Mark as changed
      }
    }
  }

  // swap src and dst pointers instead of copying buffers
  uint8_t *tmp = src;
  src = dst;
  dst = tmp;
}

// Draw the entire grid to the screen (reads from src)
void draw_grid(void) {
  for (int i = 0; i < SIZE_GRID_X; i++) {
    for (int j = 0; j < SIZE_GRID_Y; j++) {
      size_t idx = cell_index(i, j);
      eadk_color_t color = bit_get(src, idx) ? eadk_color_black : eadk_color_white;
      eadk_display_push_rect_uniform((eadk_rect_t){i * size_px, j * size_px, size_px, size_px}, color);
    }
  }
}

// Draw only changed cells (reads from src)
void draw_changed_cells(void) {
  // Coalesce horizontal runs per row to reduce draw calls
  for (int y = 0; y < SIZE_GRID_Y; ++y) {
    int x = 0;
    while (x < SIZE_GRID_X) {
      size_t idx = cell_index(x, y);
      if (!bit_get(changed_cells, idx)) {
        ++x;
        continue;
      }

      // start of run
      int run_start = x;
      // extend run while changed and same row
      do {
        ++x;
        if (x >= SIZE_GRID_X) break;
        idx = cell_index(x, y);
      } while (bit_get(changed_cells, idx));

      int run_end = x - 1;
      // draw run from run_start..run_end using src as current state
      int px = run_start * size_px;
      int py = y * size_px;
      int w = (run_end - run_start + 1) * size_px;
      int h = size_px;

      // Check if all cells in run share same state to use single rectangle
      uint8_t first_state = bit_get(src, cell_index(run_start, y));
      bool uniform = true;
      for (int sx = run_start + 1; sx <= run_end; ++sx) {
        if (bit_get(src, cell_index(sx, y)) != first_state) { uniform = false; break; }
      }

      if (uniform) {
        eadk_color_t color = first_state ? eadk_color_black : eadk_color_white;
        eadk_display_push_rect_uniform((eadk_rect_t){px, py, w, h}, color);
      } else {
        // fallback: draw per pixel inside run
        for (int sx = run_start; sx <= run_end; ++sx) {
          size_t sidx = cell_index(sx, y);
          eadk_color_t color = bit_get(src, sidx) ? eadk_color_black : eadk_color_white;
          eadk_display_push_rect_uniform((eadk_rect_t){sx * size_px, y * size_px, size_px, size_px}, color);
        }
      }
    }
  }
}

// Toggle cell state at cursor position (updates both src and dst to stay consistent)
void click(void) {
  size_t idx = cell_index(cursor[0], cursor[1]);
  bit_toggle(src, idx);
  bit_toggle(dst, idx); // keep dst in sync so next update uses consistent state
}

// Place a predefined pattern at cursor (writes to both src and dst)
void pattern(uint8_t shape) {
  int x = cursor[0];
  int y = cursor[1];

  /* optional outer guard; per-point bounds check remains */
  if (x <= 3 || y <= 3 || x >= SIZE_GRID_X - 40 || y >= SIZE_GRID_Y - 20) {
    return;
  }

  const int (*pattern_data)[2] = NULL;
  size_t pattern_size = 0;

  if (shape == TETROMINO) {
    pattern_data = tetromino;
    pattern_size = sizeof tetromino / sizeof tetromino[0];
  } else if (shape == GOSPER_GUN) {
    pattern_data = glider_gun;
    pattern_size = sizeof glider_gun / sizeof glider_gun[0];
  } else if (shape == ACORN) {
    pattern_data = acorn;
    pattern_size = sizeof acorn / sizeof acorn[0];
  } else if (shape == SIMKIN_GUN) {
    pattern_data = simkin;
    pattern_size = sizeof simkin / sizeof simkin[0];
  }

  if (!pattern_data) return;

  for (size_t i = 0; i < pattern_size; ++i) {
    int px = x + pattern_data[i][0];
    int py = y + pattern_data[i][1];
    if (px >= 0 && px < SIZE_GRID_X && py >= 0 && py < SIZE_GRID_Y) {
      size_t idx = cell_index(px, py);
      bit_set(src, idx);
      bit_set(dst, idx); // keep dst in sync
    }
  }
}

// Move the cursor in the given direction
void move(int8_t direction[2]) {
  int nx = (int)cursor[0] + (int)direction[0];
  int ny = (int)cursor[1] + (int)direction[1];

  // Clamp to grid bounds
  if (nx >= 0 && nx < SIZE_GRID_X) {
    cursor[0] = (uint8_t)nx;
  }
  if (ny >= 0 && ny < SIZE_GRID_Y) {
    cursor[1] = (uint8_t)ny;
  }

  draw_grid();
  // Draw cursor highlight
  eadk_display_push_rect_uniform(
    (eadk_rect_t){cursor[0] * size_px + 1, cursor[1] * size_px + 1, (size_px > 2) ? (size_px - 2) : 1, (size_px > 2) ? (size_px - 2) : 1},
    eadk_color_green
  );
}

// Main loop
int main(void) {
  // Clear screen
  eadk_display_push_rect_uniform((eadk_rect_t){0,0,320,240}, eadk_color_black);

  init_grid();       // Start with empty grid
  draw_grid();       // Draw initial state
  move(move_direction); // Draw cursor

  // main loop
  int32_t timeout = 10;

while (true) {
  int32_t waited = timeout;
  eadk_event_t ev = eadk_event_get(&waited); // waited updated to actual ms (ignored here)

  if (ev == eadk_event_back) {
    break; // Exit app
  } else if (ev < 4) {
    // Arrow keys: move cursor
    move_direction[0] = 0;
    move_direction[1] = 0;

    if (ev == eadk_event_left)  { move_direction[0] = -1; }
    if (ev == eadk_event_right) { move_direction[0] = 1; }
    if (ev == eadk_event_up)    { move_direction[1] = -1; }
    if (ev == eadk_event_down)  { move_direction[1] = 1; }

    // optional: avoid heavy full redraw here if costly
    // draw_grid();
    move(move_direction);

  } else if (ev == eadk_event_ok) {
    click();
    size_t idx = cell_index(cursor[0], cursor[1]);
    eadk_color_t color = bit_get(src, idx) ? eadk_color_black : eadk_color_white;
    eadk_display_push_rect_uniform((eadk_rect_t){cursor[0] * size_px, cursor[1] * size_px, size_px, size_px}, color);

  } else if (ev == eadk_event_backspace) {
    running = !running;
    if (!running) {
      // ensure final frame is visible when stopping
      draw_changed_cells();
    } else {
      // optional immediate step when starting
      update_grid();
      draw_changed_cells();
    }

  } else if (ev == eadk_event_one) {
    pattern(TETROMINO); draw_grid();
  } else if (ev == eadk_event_two) {
    pattern(GOSPER_GUN); draw_grid();
  } else if (ev == eadk_event_three) {
    pattern(ACORN); draw_grid();
  } else if (ev == eadk_event_four) {
    pattern(SIMKIN_GUN); draw_grid();
  }

  if (running) {
    update_grid();
    draw_changed_cells();
  }
}

  return 0;
}