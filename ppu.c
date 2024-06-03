#include "cpu.h"
#include "ppu.h"
#include <stdint.h>
#include <stdio.h>

void decode_tile(cpu *c, uint16_t addr, uint8_t tile[8][8]) {
    uint8_t encoded_tile[16];
    for (int i = 0; i < 16; i++) {
        encoded_tile[i] = c->memory[addr + i];
    }

    for (int y = 0; y < 8; y++) {
        uint8_t byte1 = encoded_tile[y * 2];
        uint8_t byte2 = encoded_tile[y * 2 + 1];
        for (int x = 0; x < 8; x++) {
            uint8_t bit1 = (byte1 >> (7-x)) & 1;
            uint8_t bit2 = (byte2 >> (7-x)) & 1;
            tile[y][x] = (bit2 << 1) | bit1;
        }
    }
}

void load_sprites(cpu *c, ppu *p) {
    for (int i = 0; i < 40; i++) {
        for (int j = 0; j < 4; j++) {
            p->sprites[i][j] = c->memory[0xfe00 + (4 * i) + j];
        }
    }
}

void load_background(cpu *c, ppu *p) {
    uint16_t bg_tiles_id[1024];
    if ((c->memory[0xff40] & 8) == 0)
        for (uint16_t i = 0; i < 1024; i++) {
            bg_tiles_id[i] = c->memory[0x9800 + i];
        }
    else
        for (uint16_t i = 0; i < 1024; i++) {
            bg_tiles_id[i] = c->memory[0x9c00 + i];
        }

    if ((c->memory[0xff40] & 16) != 0) {
        for (uint16_t i = 0; i < 1024; i++) {
            bg_tiles_id[i] = 0x8000 + (bg_tiles_id[i] << 4);
        }
    }
    else {
        for (uint16_t i = 0; i < 1024; i++) {
            if (bg_tiles_id[i] < 0x80)
                bg_tiles_id[i] = 0x9000 + (bg_tiles_id[i] << 4);
            else
                bg_tiles_id[i] = 0x8000 + (bg_tiles_id[i] << 4);
        }
    }

    uint8_t tile[8][8];
    for (int y = 0; y < 32; y++) {
        uint8_t display_y = y * 8;
        for (int x = 0; x < 32; x++) {
            uint8_t display_x = x * 8;
            decode_tile(c, bg_tiles_id[y * 32 + x], tile);
            for (int y1 = 0; y1 < 8; y1++) {
                for (int x1 = 0; x1 < 8; x1++) {
                    p->background[display_y + y1][display_x + x1] = tile[y1][x1];
                }
            }
        }
    }
}

void load_window(cpu *c, ppu *p) {
    uint16_t win_tiles_id[1024];
    if ((c->memory[0xff40] & 64) == 0)
        for (uint16_t i = 0; i < 1024; i++) {
            win_tiles_id[i] = c->memory[0x9800 + i];
        }
    else
        for (uint16_t i = 0; i < 1024; i++) {
            win_tiles_id[i] = c->memory[0x9c00 + i];
        }

    if ((c->memory[0xff40] & 16) != 0) {
        for (uint16_t i = 0; i < 1024; i++) {
            win_tiles_id[i] = 0x8000 + (win_tiles_id[i] << 4);
        }
    }
    else {
        for (uint16_t i = 0; i < 1024; i++) {
            if (win_tiles_id[i] < 0x80)
                win_tiles_id[i] = 0x9000 + (win_tiles_id[i] << 4);
            else
                win_tiles_id[i] = 0x8000 + (win_tiles_id[i] << 4);
        }
    }

    uint8_t tile[8][8];
    for (int y = 0; y < 32; y++) {
        uint8_t display_y = y * 8;
        for (int x = 0; x < 32; x++) {
            uint8_t display_x = x * 8;
            decode_tile(c, win_tiles_id[y * 32 + x], tile);
            for (int y1 = 0; y1 < 8; y1++) {
                for (int x1 = 0; x1 < 8; x1++) {
                    p->window[display_y + y1][display_x + x1] = tile[y1][x1];
                }
            }
        }
    }
}

void load_display(cpu *c, ppu *p) {
    load_background(c, p);
    load_window(c, p);
    if ((c->memory[0xff40] & 128) != 0) {
        uint8_t scx = c->memory[0xff43];
        uint8_t scy = c->memory[0xff42];
        uint8_t wy = c->memory[0xff4a];
        uint8_t wx = c->memory[0xff4b];
        for (uint8_t y = 0; y < 144; y++) {
            for (uint8_t x = 0; x < 160; x++) {
                p->display[y][x] = p->background[(uint8_t) (y + scy)][(uint8_t) (x + scx)];
            }
        }
        if ((c->memory[0xff40] & 2) != 0) {
            load_sprites(c, p);
            for (int i = 0; i < 40; i++) {
                if (((p->sprites[i][0] < 160) && (p->sprites[i][0] > 16) && (p->sprites[i][1] < 160) && (p->sprites[i][1] > 8))) {
                    uint8_t sprite_tile[8][8];
                    decode_tile(c, (0x8000 | (p->sprites[i][2] << 4)), sprite_tile);

                    for (int16_t y = 0; y < 8; y++) {
                        for (int16_t x = 0; x < 8; x++) {
                            if (sprite_tile[y][x] != 0) {
                                p->display[p->sprites[i][0] + y - 16][p->sprites[i][1] + x - 8] = sprite_tile[y][x];
                            }
                        }
                    }
                }

            }
        }

    }
}
