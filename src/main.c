#include "eadk.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

const char eadk_app_name[] __attribute__((section(".rodata.eadk_app_name"))) = "Game of life";
const uint32_t eadk_api_level  __attribute__((section(".rodata.eadk_api_level"))) = 0;

#define SIZE_GRID_X 80
#define SIZE_GRID_Y 60
bool temp_grid[SIZE_GRID_X][SIZE_GRID_Y];
bool grid[SIZE_GRID_X][SIZE_GRID_Y];
uint8_t size_px = 4;
char str[20];
uint8_t live_neighbors;
uint8_t cursor[2] = {0, 0};
uint8_t move_direction[] = {0, 0};
uint8_t new_x, new_y;

void init_grid(void) {
  for (int i = 0; i < SIZE_GRID_X; i++) {
    for (int j = 0; j < SIZE_GRID_Y; j++) {
      grid[i][j] = 0;
    }
  }
}

void update_grid(void) {
  memcpy(temp_grid, grid, sizeof(grid));
  for (int i = 0; i < SIZE_GRID_X; i++) {
    for (int j = 0; j < SIZE_GRID_Y; j++) {
      int up = (i + SIZE_GRID_X-1) % SIZE_GRID_X;
      int down = (i + 1) % SIZE_GRID_X;
      int left = (j + SIZE_GRID_Y-1) % SIZE_GRID_Y;
      int right = (j + 1) % SIZE_GRID_Y;

      live_neighbors = temp_grid[up][left] + temp_grid[up][j] + temp_grid[up][right] +
                      temp_grid[i][left]               + temp_grid[i][right] +
                      temp_grid[down][left] + temp_grid[down][j] + temp_grid[down][right];

      if (live_neighbors == 3) {
        grid[i][j] = 1;
      } else if (live_neighbors == 2) {
        grid[i][j] = temp_grid[i][j];
      } else {
        grid[i][j] = 0;
      }
    }
  }
}

void draw_grid(void) {
  for (int i = 0; i < SIZE_GRID_X; i++) {
    for (int j = 0; j < SIZE_GRID_Y; j++) {
      if (grid[i][j] == 1) {
        eadk_display_push_rect_uniform((eadk_rect_t){i * size_px, j * size_px, size_px, size_px}, eadk_color_black);
      }
      else {
        eadk_display_push_rect_uniform((eadk_rect_t){i * size_px, j * size_px, size_px, size_px}, eadk_color_white);
      }
    }
  }
}

void click(void) {
  grid[cursor[0]][cursor[1]] = !grid[cursor[0]][cursor[1]];
}

void move(uint8_t direction[2]) {
  new_x = cursor[0] + direction[0];
  new_y = cursor[1] + direction[1];

  if (new_x >= 0 && new_x < SIZE_GRID_X) {
    cursor[0] = new_x;
  }
  if (new_y >= 0 && new_y < SIZE_GRID_Y) {
    cursor[1] = new_y;
  }

  eadk_display_push_rect_uniform(
    (eadk_rect_t){cursor[0] * size_px + 1, cursor[1] * size_px + 1, size_px - 2, size_px - 2},
    eadk_color_green
  );
}

int main(void) {
  eadk_display_push_rect_uniform((eadk_rect_t){0,0,320,240}, eadk_color_black);
  init_grid();
  draw_grid();
  move(move_direction);

  // Event loop: wait for events and exit on "ok" or "home"
  while (true) {
    int32_t timeout = 1000; // ms: adjust as needed
    eadk_event_t ev = eadk_event_get(&timeout);
    if (ev == eadk_event_back) {
      break;
    } else if (ev < 4) {
      move_direction[0] = 0;
      move_direction[1] = 0;

      if (ev == eadk_event_left) {move_direction[0] = -1;}
      if (ev == eadk_event_right) {move_direction[0] = 1;}
      if (ev == eadk_event_up) {move_direction[1] = -1;}
      if (ev == eadk_event_down) {move_direction[1] = 1;}

      draw_grid();
      move(move_direction);

    } else if (ev == eadk_event_ok) {
      click();
    }
      else if (ev == eadk_event_backspace) {
      update_grid();
      draw_grid();
    } 
    
  }

  return 0;
}