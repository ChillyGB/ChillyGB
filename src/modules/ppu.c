#include "../includes/cpu.h"
#include "../includes/ppu.h"
#include "../includes/timer.h"
#include <stdint.h>
#include <string.h>

ppu video;

void oam_scan(cpu *c) {
    video.buffer_size = 0;
    uint8_t sprite_height = 8;
    if (video.obj_size)
        sprite_height = 16;
    for (int i = 0; (i < 40) && (video.buffer_size < 10); i++) {
        uint8_t sprite_y = c->memory[0xfe00 | (i << 2)];
        if ((video.scan_line + 16) >= sprite_y && (video.scan_line + 16) < sprite_y + sprite_height) {
            video.oam_buffer[video.buffer_size] = i;
            video.buffer_size++;
        }
    }
}

void fetch_bg_to_fifo(cpu *c) {
    uint8_t bgX = video.scx + (video.current_pixel + video.fifo.pixel_count);
    uint8_t bgY = video.scy + video.scan_line;
    uint16_t tile_id = c->memory[(video.bg_tilemap ? 0x9c00 : 0x9800) | (((uint16_t) bgY & 0x00f8) << 2) | (bgX >> 3)];
    uint16_t tile_addr = (0x8000 | (tile_id << 4) | (video.bg_tiles || tile_id > 0x7f ? 0x0000 : 0x1000)) + ((bgY << 1) & 0x0f);
    uint8_t lowerTileData = c->memory[tile_addr];
    uint8_t upperTileData = c->memory[tile_addr + 1];

    for (int i = 0; i < 8; i++) {
        video.fifo.pixels[video.fifo.pixel_count + i].value = (lowerTileData >> (7 - i)) & 1;
        video.fifo.pixels[video.fifo.pixel_count + i].value |= (upperTileData >> (7 - i) & 1) << 1;
        video.fifo.pixels[video.fifo.pixel_count + i].palette = 0;
    }

    video.fifo.pixel_count += 8;
}

void fetch_win_to_fifo(cpu *c) {
    uint8_t bgX = ((video.current_pixel + video.fifo.pixel_count) - video.wx) + 7;
    uint8_t bgY = video.window_internal_line;
    uint16_t tile_id = c->memory[(video.window_tilemap ? 0x9c00 : 0x9800) | (((uint16_t) bgY & 0x00f8) << 2) | (bgX >> 3)];
    uint16_t tile_addr = (0x8000 | (tile_id << 4) | (video.bg_tiles || tile_id > 0x7f ? 0x0000 : 0x1000)) + ((bgY << 1) & 0x0f);
    uint8_t lowerTileData = c->memory[tile_addr];
    uint8_t upperTileData = c->memory[tile_addr + 1];

    for (int i = 0; i < 8; i++) {
        video.fifo.pixels[video.fifo.pixel_count + i].value = (lowerTileData >> (7 - i)) & 1;
        video.fifo.pixels[video.fifo.pixel_count + i].value |= (upperTileData >> (7 - i) & 1) << 1;
        video.fifo.pixels[video.fifo.pixel_count + i].palette = 0;
    }

    video.fifo.pixel_count += 8;
    if (video.fifo.window_wx7) {
        video.fifo.window_wx7 = false;
        for (int i = 0; i < video.fifo.pixel_count-1; i++) {
            video.fifo.pixels[i].value = video.fifo.pixels[i+(7-video.wx)].value;
            video.fifo.pixels[i].palette = video.fifo.pixels[i+(7-video.wx)].palette;
        }
        video.fifo.pixel_count -= (7-video.wx);
    }
}

void fetch_sprite_to_fifo(cpu *c) {
    uint16_t buffer[10];
    uint8_t buffer_size = 0;

    for (int i = 0; i < video.buffer_size; i++) {
        uint16_t sprite_addr = 0xfe00 | (video.oam_buffer[i] << 2);
        if (c->memory[sprite_addr | 1] == (video.current_pixel + 8)) {
            buffer[buffer_size] = sprite_addr;
            buffer_size++;
        }
    }
    if (buffer_size > 0) {
        for (int j = 0; j < buffer_size; j++) {
            uint16_t sprite_addr = buffer[j];
            uint8_t palette = ((c->memory[sprite_addr | 3] & 0x10) >> 4) + 1;
            bool flip_X = ((c->memory[sprite_addr | 3] & 0x20) != 0);
            bool flip_Y = ((c->memory[sprite_addr | 3] & 0x40) != 0);
            bool priority = ((c->memory[sprite_addr | 3] & 0x80) != 0);

            uint8_t tile_id = (!video.obj_size) ? c->memory[sprite_addr | 2] : c->memory[sprite_addr | 2] & 0xfe;
            uint8_t tile_y = (video.scan_line + 16) - c->memory[sprite_addr];
            if (flip_Y) {
                tile_y = (~tile_y) & ((video.obj_size) ? 0xf : 0x7);
            }
            uint16_t tile_addr = (0x8000 | (tile_id << 4) + ((tile_y << 1) & 0x1f));
            uint8_t lowerTileData = c->memory[tile_addr];
            uint8_t upperTileData = c->memory[tile_addr + 1];

            pixel sprite[8];
            for (int i = 0; i < 8; i++) {
                if (!flip_X) {
                    sprite[i].value = (lowerTileData >> (7 - i)) & 1;
                    sprite[i].value |= (upperTileData >> (7 - i) & 1) << 1;
                } else {
                    sprite[i].value = (lowerTileData >> i) & 1;
                    sprite[i].value |= (upperTileData >> i & 1) << 1;
                }
            }
            for (int i = 0; i < 8; i++) {
                if (sprite[i].value != 0 && video.fifo.pixels[i].palette == 0) {
                    if (!priority || video.fifo.pixels[i].value == 0) {
                        video.fifo.pixels[i].value = sprite[i].value;
                        video.fifo.pixels[i].palette = palette;
                    }
                }
            }
        }
    }
}

void fetch_sprite_to_fifo_minus_8(cpu *c) {
    for (int i = 1; i < 8; i++) {
        uint16_t buffer[10];
        uint8_t buffer_size = 0;

        for (int j = 0; j < video.buffer_size; j++) {
            uint16_t sprite_addr = 0xfe00 | (video.oam_buffer[j] << 2);
            if (c->memory[sprite_addr | 1] == i) {
                buffer[buffer_size] = sprite_addr;
                buffer_size++;
            }
        }
        if (buffer_size > 0) {
            for (int j = 0; j < buffer_size; j++) {
                uint16_t sprite_addr = buffer[j];
                uint8_t palette = ((c->memory[sprite_addr | 3] & 0x10) >> 4) + 1;
                bool flip_X = ((c->memory[sprite_addr | 3] & 0x20) != 0);
                bool flip_Y = ((c->memory[sprite_addr | 3] & 0x40) != 0);
                bool priority = ((c->memory[sprite_addr | 3] & 0x80) != 0);

                uint8_t tile_id = (!video.obj_size) ? c->memory[sprite_addr | 2] : c->memory[sprite_addr | 2] & 0xfe;
                uint8_t tile_y = (video.scan_line + 16) - c->memory[sprite_addr];
                if (flip_Y) {
                    tile_y = (~tile_y) & ((video.obj_size) ? 0xf : 0x7);
                }
                uint16_t tile_addr = (0x8000 | (tile_id << 4) + ((tile_y << 1) & 0x1f));
                uint8_t lowerTileData = c->memory[tile_addr];
                uint8_t upperTileData = c->memory[tile_addr + 1];

                pixel sprite[8];
                for (int k = 0; k < 8; k++) {
                    if (!flip_X) {
                        sprite[k].value = (lowerTileData >> (7 - k)) & 1;
                        sprite[k].value |= (upperTileData >> (7 - k) & 1) << 1;
                    } else {
                        sprite[k].value = (lowerTileData >> k) & 1;
                        sprite[k].value |= (upperTileData >> k & 1) << 1;
                    }
                }
                for (int k = 0; k < i; k++) {
                    if (sprite[8-i+k].value != 0 && video.fifo.pixels[k].palette == 0) {
                        if (!priority || video.fifo.pixels[k].value == 0) {
                            video.fifo.pixels[k].value = sprite[8-i+k].value;
                            video.fifo.pixels[k].palette = palette;
                        }
                    }
                }
            }
        }
    }
}

void push_pixel() {
    // Copy pixel to display
    if (video.bg_enable && video.fifo.pixels[0].palette == 0) {
        video.line[video.current_pixel] = video.bgp[video.fifo.pixels[0].value];
    }
    else if (video.fifo.pixels[0].palette != 0) {
        video.line[video.current_pixel] = video.obp[video.fifo.pixels[0].palette - 1][video.fifo.pixels[0].value];
    }
    else {
        video.line[video.current_pixel] = video.bgp[0];
    }

    // Shift FIFO
    for (int i = 0; i < video.fifo.pixel_count-1; i++) {
        video.fifo.pixels[i].value = video.fifo.pixels[i+1].value;
        video.fifo.pixels[i].palette = video.fifo.pixels[i+1].palette;
    }
    video.fifo.pixel_count--;

    // Advance current pixel index
    if (video.fifo.scx_init > 0 && !video.in_window) {
        video.fifo.scx_init--;
    }
    else {
        video.current_pixel++;
    }
}

void operate_fifo(cpu *c) {
    if (video.fifo.init_timer > 0) {
        video.fifo.init_timer--;
        if (video.fifo.init_timer == 0) {
            fetch_bg_to_fifo(c);
            if (video.obj_enable) {
                fetch_sprite_to_fifo_minus_8(c);
            }
            video.fifo.scx_init = video.scx % 8;
        }
        return;
    }
    if (video.current_pixel < 160) {
        if (!video.in_window && video.window_enable && video.wy_trigger && video.current_pixel + 8 > video.wx) {
            video.in_window = true;
            video.fifo.win_timer = 6;
            video.fifo.pixel_count = 0;
            if (video.wx < 7)
                video.fifo.window_wx7 = true;
        }
        if (!video.in_window) {
            while (video.fifo.pixel_count <= 8)
                fetch_bg_to_fifo(c);
            if (video.obj_enable) {
                fetch_sprite_to_fifo(c);
            }
            push_pixel();
        }
        else {
            while (video.fifo.pixel_count <= 8)
                fetch_win_to_fifo(c);
            if (video.fifo.win_timer > 0) {
                video.fifo.win_timer--;
            }
            else {
                if (video.obj_enable) {
                    fetch_sprite_to_fifo(c);
                }
                push_pixel();
            }
        }
    }
    else {
        video.fifo.pixel_count = 0;
        if (video.in_window)
            video.window_internal_line++;
        video.in_window = false;
        video.mode = 0;
    }
}

void load_line() {
    if (video.is_on) {
        for (int i = 0; i < 160; i++) {
            video.display[video.scan_line][i] = video.line[i];
        }
    }
    else {
        memset(video.display, 0, 160*144*sizeof(uint8_t));
    }
}
