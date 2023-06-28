// Screen routines for nPDF
// Copyright (C) 2014-2016  Legimet
//
// This file is part of nPDF.
//
// nPDF is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// nPDF is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with nPDF.  If not, see <http://www.gnu.org/licenses/>.

#include <algorithm>
#include <cstdint>
#include <libndls.h>
#include "Screen.hpp"

bool Screen::initialized;
unsigned int Screen::size;
scr_type_t Screen::type;
uint8_t *Screen::screen;

Screen::Screen() {
	if (initialized) throw "Screen is already initialized.";
	type = lcd_type() == SCR_320x240_4 ? SCR_320x240_4 : SCR_320x240_565;
	if (!lcd_init(type)) throw "Could not init lcd.";
	size = (SCREEN_WIDTH*SCREEN_HEIGHT/2) * (type == SCR_320x240_4 ? 1 : 4);
	screen = new uint8_t[size];
	initialized = true;
}

Screen::~Screen() {
	if (initialized) {
		delete[] screen;
		lcd_init(SCR_TYPE_INVALID);
		initialized = false;
	}
}

void Screen::display() {
	lcd_blit(screen, type);
}

void Screen::setPixel(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y) {
	// On color models, each pixel is represented in 16-bit high color
	// On classic models, each pixel is 4 bits grayscale, 0 is black and 15 is white
	if (x < SCREEN_WIDTH && y < SCREEN_HEIGHT) {
		unsigned int pos = y * SCREEN_WIDTH + x;
		if (type == SCR_320x240_565) {
			reinterpret_cast<uint16_t*>(screen)[pos] = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
		} else if (pos % 2 == 0) {
			screen[pos / 2] = (screen[pos / 2] & 0x0F) | (((30 * r + 59 * g + 11 * b) / 100) & 0xF0);
		} else {
			screen[pos / 2] = (screen[pos / 2] & 0xF0) | (((30 * r + 59 * g + 11 * b) / 100) >> 4);
		}
	}
}

void Screen::setPixel(uint8_t c, unsigned int x, unsigned int y) {
	setPixel(c, c, c, x, y);
}

// The RGBA and GrayA functions display pixmaps with premultiplied alpha. The alpha component is
// ignored since we aren't compositing

void Screen::showImgRGB(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
		unsigned int w, unsigned int h, unsigned int wImg) {
	unsigned int pos;
	for (unsigned int i = y1; i < y1 + h; i++) {
		for (unsigned int j = x1; j < x1 + w; j++) {
			pos = 3 * (wImg * i + j);
			setPixel(img[pos], img[pos + 1], img[pos + 2], x0 - x1 + j, y0 - y1 + i);
		}
	}
}

void Screen::showImgRGBA(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
		unsigned int w, unsigned int h, unsigned int wImg) {
	unsigned int pos;
	for (unsigned int i = y1; i < y1 + h; i++) {
		for (unsigned int j = x1; j < x1 + w; j++) {
			pos = 4 * (wImg * i + j);
			setPixel(img[pos], img[pos + 1], img[pos + 2], x0 - x1 + j, y0 - y1 + i);
		}
	}
}

void Screen::showImgGray(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
		unsigned int w, unsigned int h, unsigned int wImg) {
	unsigned int pos;
	for (unsigned int i = y1; i < y1 + h; i++) {
		for (unsigned int j = x1; j < x1 + w; j++) {
			pos = wImg * i + j;
			setPixel(img[pos], x0 - x1 + j, y0 - y1 + i);
		}
	}
}

void Screen::showImgGrayA(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
		unsigned int w, unsigned int h, unsigned int wImg) {
	unsigned int pos;
	for (unsigned int i = y1; i < y1 + h; i++) {
		for (unsigned int j = x1; j < x1 + w; j++) {
			pos = 2 * (wImg * i + j);
			setPixel(img[pos], x0 - x1 + j, y0 - y1 + i);
		}
	}
}

void Screen::fillScreen(uint8_t r, uint8_t g, uint8_t b) {
	uint16_t color;
	if (type == SCR_320x240_565) {
		color = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
	} else {
		color = (30 * r + 59 * g + 11 * b) / 100;
		color = (color >> 4) * 0x1111;
	}

	std::fill(reinterpret_cast<volatile uint16_t*>(screen), reinterpret_cast<volatile uint16_t*>(screen + size), color);
}

void Screen::fillScreen(uint8_t c) {
	fillScreen(c, c, c);
}

void Screen::fillRect(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int w,
		unsigned int h) {
	for (unsigned int i = x; i < x + w; i++) {
		for (unsigned int j = y; j < y + h; j++) {
			setPixel(r, g, b, i, j);
		}
	}
}

void Screen::fillRect(uint8_t c, unsigned int x, unsigned int y, unsigned int w, unsigned int h) {
	fillRect(c, c, c, x, y, w, h);
}

void Screen::drawVert(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int h) {
	for (unsigned int j = y; j < y + h; j++) {
		setPixel(r, g, b, x, j);
	}
}

void Screen::drawHoriz(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int w) {
	for (unsigned int i = x; i < x + w; i++) {
		setPixel(r, g, b, i, y);
	}
}
