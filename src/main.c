/******************************************************************************
*
*    Copyright (C) 2025 David Latham
*
*    This library is free software; you can redistribute it and/or
*    modify it under the terms of the GNU Lesser General Public
*    License as published by the Free Software Foundation; either
*    version 2.1 of the License, or (at your option) any later version.
*
*    This library is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    Lesser General Public License for more details.
*
*    You should have received a copy of the GNU Lesser General Public
*    License along with this library; if not, write to the Free Software
*    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301
*    USA
*
* https://github.com/linuxplayground/z80-breakout
*
*****************************************************************************
*/
#include "assets.h"
#include <cpm.h>
#include <joy.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tms.h>

#ifndef RETRO
#include <ay-3-8910.h>
#include <ay-notes.h>
#endif

#define PAD_MAX_X 255 - 32
#define PAD_MIN_X 0
#define PAD_Y 191 - 5 - 2
#define PAD_SPEED 4

#if defined NOUVEAU
#define DECAY 900
#elif defined NABU
#define DECAY 600
#else
#define DECAY 600
#endif

// globals
size_t i;
char k;
uint8_t jnext = 0;
int pad_x = 0;
bool should_exit = false;
int ballx = 256 / 2 - 2;
int bally = PAD_Y - 4;
int ball_vx = 0;
int ball_vy = 0;
int ball_dx = 0;
int ball_dy = 0;
int x, y, o;
uint16_t frames = 0;
int16_t score = 0;
int16_t highscore = 0;
uint8_t hitbrick = 0;
bool launching = true;
uint8_t lives = 3;
char num_tb[4] = {0};
uint8_t level;

void printstats() {
  tms_puts_xy(0, 0, "LIVES:");
  itoa(lives, num_tb, 16);
  tms_puts_xy(7, 0, num_tb);

  tms_puts_xy(9, 0, "LVL:");
  itoa(level, num_tb, 10);
  tms_puts_xy(13, 0, num_tb);

  tms_puts_xy(15, 0, "TOP:");
  itoa(highscore, num_tb, 10);
  tms_puts_xy(19, 0, num_tb);

  tms_puts_xy(23, 0, "SCORE:");
  itoa(score, num_tb, 10);
  tms_puts_xy(29, 0, num_tb);
}

void newlevel() {
  char *p = tms_buf + (32 * 3);
  memset(tms_buf, 0x20, 768);

  printstats();
  jnext = 0xFF;
  for (i = 0; i < 32 * 6; ++i) {
    *p++ = bricks[i];
  }
  ballx = 256 / 2 - 2;
  bally = PAD_Y - 4;
  ball_vx = 1;
  ball_vy = 2;
  ball_dx = 1;
  ball_dy = -1;
  pad_x = 255 / 2 - 16;
}

bool menu() {
  char *p = tms_buf + (32 * 3);

  for (i = 0; i < 3; ++i)
    sprites[i].y = 192;
  tms_flush_sprites();

  jnext = 0xFF;
  should_exit = false;
  memset(tms_buf, 0x20, 768);
  for (i = 0; i < 32 * 6; ++i) {
    *p++ = bricks[i];
  }
  printstats();
  tms_puts_xy(3, 18, "Press button to start");
#if defined NABU
  tms_puts_xy(3, 11, "NABU Breakout");
#elif defined NOUVEAU
  tms_puts_xy(3, 11, "Z80-Nouveau Breakout");
#elif defined RETRO
  tms_puts_xy(3, 11, "Z80-Retro Breakout");
#endif
  tms_puts_xy(3, 12, "Release 1.0.0");
  tms_puts_xy(3, 13, "(c) Productiondave - 2025");
  tms_puts_xy(3, 19, "Press ESC to exit");
  tms_wait();
  tms_g1flush(tms_buf);
  tms_wait();
  tms_g1flush(tms_buf);
  tms_wait();
  tms_g1flush(tms_buf);

  for (;;) {
    if ((joy(0) & JOY_MAP_BUTTON) == 0)
      return true;
    k = cpm_dc_in();
    if (k == ' ') {
      return true;
    } else if (k == 0x1b)
      return false;
  }
}

// Set up new game
void newgame() {

  should_exit = false;
  score = 0;
  lives = 3;
  level = 0;
  // paddle left
  sprites[0].x = pad_x;
  sprites[0].y = PAD_Y;
  sprites[0].pattern = 0;
  sprites[0].color = CYAN;
  // paddle right
  sprites[1].x = pad_x + 16;
  sprites[1].y = PAD_Y;
  sprites[1].pattern = 0;
  sprites[1].color = CYAN;
  // ball
  sprites[2].x = ballx;
  sprites[2].y = bally;
  sprites[2].pattern = 4;
  sprites[2].color = WHITE;

  sprites[3].y = 0xD0;
  newlevel();
}

// sfx unless RETRO
void sfx_wall() {
#ifndef RETRO
  ay_play_note_delay(0x0F, AY_CHANNEL_A, DECAY);
#endif
}

void sfx_paddle() {
#ifndef RETRO
  ay_play_note_delay(0x0F + 12, AY_CHANNEL_A, DECAY);
#endif
}

void sfx_brick() {
#ifndef RETRO
  ay_play_note_delay(0x0F + 24, AY_CHANNEL_A, DECAY);
#endif
}

void sfx_life() {
#ifndef RETRO
  ay_play_note_delay(0x0F, AY_CHANNEL_A, 12000);
#endif
}

// draw the two sprites that make up the paddle
void draw_paddle() {
  sprites[0].x = pad_x;
  sprites[1].x = pad_x + 16;
}

// Move the ball by the direction vector * the ball speed
void move_ball() {
  ballx += ball_dx * ball_vx;
  bally += ball_dy * ball_vy;
  sprites[2].x = ballx;
  sprites[2].y = bally;
}

void score_brick(uint8_t brick) {
  switch (hitbrick) {
  case 0x18:
  case 0x1a:
    score += 4;
    itoa(score, num_tb, 10);
    break;
  case 0x10:
  case 0x12:
    score += 3;
    itoa(score, num_tb, 10);
    break;
  case 0x08:
  case 0x0a:
    score += 2;
    itoa(score, num_tb, 10);
    break;
  case 0x00:
  case 0x02:
    score += 1;
    itoa(score, num_tb, 10);
    break;
  default:
    break;
  }
  tms_puts_xy(29, 0, num_tb);
}

bool is_brick_hit(uint16_t o) {
  if (tms_buf[o] < ' ') {
    x = x & 0xFE;
    o = y * 32 + x;
    hitbrick = tms_buf[o];
    if (level == 1) {
      switch (hitbrick) {
      case 0x18:
      case 0x10:
      case 0x08:
      case 0x00:
        tms_buf[o] = hitbrick + 2;
        tms_buf[o + 1] = hitbrick + 3;
        break;
      default:
        tms_buf[o] = ' ';
        tms_buf[o + 1] = ' ';
      }
    } else {
      tms_buf[o] = ' ';
      tms_buf[o + 1] = ' ';
    }
    score_brick(hitbrick);
    return true;
  } else {
    return false;
  }
}

void collide_bricks() {
  // Brick
  // new x position.
  x = (ballx + (ball_dx * ball_vx)) / 8;
  y = bally / 8;
  o = y * 32 + x;
  if (is_brick_hit(o)) {
    ball_dx *= -1;
    sfx_brick();
    return;
  }
  // new y position
  x = ballx / 8;
  y = (bally + (ball_dy * ball_vy)) / 8;
  o = y * 32 + x;
  if (is_brick_hit(o)) {
    ball_dy *= -1;
    sfx_brick();
    return;
  }
}

void collide_walls() {
  // Walls left and right
  if (ballx > 255 - 4) {
    ball_dx *= -1;
    ballx = 255 - 4;
    sfx_wall();
    return;
  } else if (ballx < 1) {
    ball_dx *= -1;
    ballx = 0;
    sfx_wall();
    return;
  }

  // Walls top and bottom
  if (bally > (191 - 4)) {
    launching = true;
    bally = PAD_Y - 4;
    sprites[2].y = bally;
    sfx_life();
    if (--lives == 0)
      should_exit = true;
    itoa(lives, num_tb, 16);
    tms_puts_xy(7, 0, num_tb);
    return;
  } else if (bally < 1) {
    ball_dy *= -1;
    bally = 0;
    sfx_wall();
  }
}

void collide_paddle() {
  // paddle
  if (tms_status & 0x20) {
    if (ball_dy > 0 && ball_dy <= PAD_Y) { // You can not hit the side of the
                                           // paddle.
      if (ballx < (pad_x + 4)) {
        ball_dx = -1;
        ball_dy = -1;
        ball_vx = 2;
        ball_vy = 1;
        sfx_paddle();
        return;
      } else if (ballx < (pad_x + 12)) {
        ball_dx = -1;
        ball_dy = -1;
        ball_vx = 1;
        ball_vy = 2;
        sfx_paddle();
        return;
      } else if (ballx < (pad_x + 20)) {
        ball_dy = -1;
        sfx_paddle();
        return;
      } else if (ballx < (pad_x + 28)) {
        ball_dx = 1;
        ball_dy = -1;
        ball_vx = 1;
        ball_vy = 2;
        sfx_paddle();
        return;
      } else {
        ball_dx = 1;
        ball_dy = -1;
        ball_vx = 2;
        ball_vy = 1;
        sfx_paddle();
        return;
      }
    }
  }
}
void play() {
  while (!should_exit) {
    k = cpm_dc_in();
    if (k == 0x1b)
      should_exit = true;
    if (k == '1') {
      score = 208;
      level = 1;
    }
    if (k == '0')
      level = 0;
#ifndef KEYBOARD
    jnext = joy(0); // read the first joystick

    if ((jnext & JOY_MAP_LEFT) == 0) {
      pad_x -= PAD_SPEED;
    }

    if ((jnext & JOY_MAP_RIGHT) == 0) {
      pad_x += PAD_SPEED;
    }

    if ((jnext & JOY_MAP_BUTTON) == 0) {
      launching = false;
    }

#else
    switch (k) {
    case 'a':
      pad_x -= PAD_SPEED;
      break;
    case 'd':
      pad_x += PAD_SPEED;
      break;
    case ' ':
      launching = false;
      break;
    default:
      break;
    }
#endif

    // clamp to walls
    if (pad_x < PAD_MIN_X)
      pad_x = PAD_MIN_X;
    if (pad_x > PAD_MAX_X)
      pad_x = PAD_MAX_X;

    draw_paddle();
    if (launching) {
      ballx = pad_x + 16 - 2;
      sprites[2].x = ballx;
    } else {
      move_ball();
      collide_walls();
      collide_bricks();
      collide_paddle();
    }
    tms_wait();
    tms_g1flush(tms_buf);
    tms_flush_sprites();
    frames++;
    if (level == 0) {
      if (score == 208) {
        level = 1;
        launching = true;
        lives++;
        newlevel();
      }
    } else {
      if (score == 208 * 3)
        should_exit = true;
    }
  }
}

void delay(uint16_t count) {
  while (count-- > 0) {
    tms_wait();
  }
}
/*
 * Main entry point of game.
 * Initialise the graphics and assets.
 * Reset the game
 * Game loop
 *   check for player input and move paddle
 *   move the ball
 *   check for collisions
 *   flush the frame buffer and sprites to the VDP
 */
void main() {
  tms_init_g1(BLACK, DARK_YELLOW, true, false);
  tms_load_col(colors, 32);
  tms_load_pat(tms_patterns, 0x400);
  tms_load_spr(sprite_patterns, 32 * 2);
#ifndef RETRO
  ay_write(7, 0x7E); // enable channel A tone only 0b01111110
  ay_write(8, 0x1F); // set channel A volume to 12
#endif
  while (menu()) {
    newgame();
    play();
    if (score > highscore)
      highscore = score;
    itoa(highscore, num_tb, 10);

    tms_puts_xy(0, 0, "GAME OVER");
    tms_wait();
    tms_g1flush(tms_buf);
    delay(60);
  }
}
