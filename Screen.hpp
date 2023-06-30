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

#ifndef SCREEN_HPP
#define SCREEN_HPP

#include <cstdint>


class Screen {
	public:
		Screen();

		~Screen();

		static void display();

		static void setPixel(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y);

		static void setPixel(uint8_t c, unsigned int x, unsigned int y);

		static void showImgRGB(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
			unsigned int w, unsigned int h, unsigned int wTotal);

		static void showImgRGBA(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
			unsigned int w, unsigned int h, unsigned int wTotal);

		static void showImgGray(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
			unsigned int w, unsigned int h, unsigned int wTotal);

		static void showImgGrayA(uint8_t *img, unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1,
			unsigned int w, unsigned int h, unsigned int wTotal);

		static void fillScreen(uint8_t r, uint8_t g, uint8_t b);

		static void fillScreen(uint8_t c);

		static void fillRect(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int w,
			unsigned int h);

		static void fillRect(uint8_t c, unsigned int x, unsigned int y, unsigned int w, unsigned int h);

		static void drawVert(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int h);

		static void drawHoriz(uint8_t r, uint8_t g, uint8_t b, unsigned int x, unsigned int y, unsigned int w);

	private:
		static scr_type_t type;
		static unsigned int size;
		static uint8_t *screen;
		static bool initialized;
};

#endif
