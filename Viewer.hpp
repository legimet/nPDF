// nPDF viewer class
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

#ifndef VIEWER_HPP
#define VIEWER_HPP

extern "C" {
#include <mupdf/fitz.h>
}
#include <memory>

#define MATCH_LIMIT 512

enum Direction{
	FORWARD = 1,
	BACKWARD = -1,
};

struct PageIterator {
	int start;
	int nPages;
	Direction dir;

	int current;

	PageIterator(int start, int nPages, Direction dir);
	int next();
};


class Document {
	private:
		// Essentials
		fz_context *ctx;
		fz_document *doc = nullptr;
		fz_page *page = nullptr;
		fz_rect bounds;

		// Page state
		unsigned int currentlyLoadedPageNo = -1; // Currently loaded page. May deviate from the one to be displayed
		unsigned int pageNo = 0; // Current page to be displayed

		// Find related
		fz_rect matches[MATCH_LIMIT]; // Matches on current page, ordered by y and x
		int matchesCount = 0; // Number of matches on current page
		int matchIdx = -1; // Current match to be displayed
		char* matchingFor = nullptr; // Search string

		// Find and go to first page which contains this token. Assumption: current iterator page is a fresh one.
		fz_rect* gotoNextPageWithOccurrence(PageIterator& iter);
		// Loop over iterator until page with given text is found or iterator is exhausted
		int scanPages(PageIterator& iter, int* outPage);

	public:
		// Init
		Document(fz_context *ctx);
		~Document();
		bool open(const char *path);

		// Page manipulation
		fz_page* ensureCurrentPageLoaded();
		unsigned int getPages();
		bool next();
		bool prev();
		// Go to page with bounds check. Page is loaded.
		bool gotoPage(unsigned int page);
		// Get bounds of current page
		const fz_rect& getBounds();

		// Find related
		const fz_rect* getCurrentMatch();
		const fz_rect* find(char *s);
		const fz_rect* findNext(Direction dir);
		void resetFind();
};


class Viewer {
	private:
		static const int scroll;
		static const float zoom;
		static const unsigned char bgColor;
		static const float maxScale;
		static const float minScale;
		fz_context *ctx;
		fz_pixmap *pix;
		fz_rect bounds;
		fz_matrix transform;
		float scale;
		int xPos;
		int yPos;
		bool fitWidth;
		const int width;
		const int height;

		std::unique_ptr<Document> doc;

	public:
		Viewer();
		~Viewer();
		void invertPixels(const fz_rect *rect);
		bool find(char *s);
		bool findNext(Direction dir);
		void openDoc(const char *path);
		int getPages();
		void drawPage();
		void display();
		void next();
		void prev();
		void scrollUp();
		void scrollDown();
		void scrollLeft();
		void scrollRight();
		void setFitWidth();
		void unsetFitWidth();
		void zoomIn();
		void zoomOut();
		void gotoPage(unsigned int page);
		void fixBounds();
};

#endif
