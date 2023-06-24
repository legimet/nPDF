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

extern "C" {
#include <mupdf/fitz.h>
}
#include <algorithm>
#include <libndls.h>
#include "Viewer.hpp"
#include "Screen.hpp"

const int Viewer::scroll = 20;
const float Viewer::zoom = 1.142857;
const unsigned char Viewer::bgColor = 103;
const float Viewer::maxScale = 2.0;
const float Viewer::minScale = 0.1;


PageIterator::PageIterator(int start, int nPages, Direction dir)
	: start{start}
	, nPages{nPages}
	, dir{dir}
	, current{start}
	{}

int PageIterator::next() {
	int newCurrent = (nPages + current + dir) % nPages;
	if (newCurrent != start) {
		current = newCurrent;
		return current;
	}
	return -1;
}

Document::Document(fz_context *ctx)
	: ctx{ctx}
	{
}

Document::~Document(){
	if (matchingFor != nullptr ){
		free(matchingFor);
		matchingFor = nullptr;
	}
	if (page != nullptr) {
		fz_drop_page(ctx, page);
		page = nullptr;
	}
	fz_drop_document(ctx, doc);
}

bool Document::open(const char *path) {
	fz_try(ctx) {
		doc = fz_open_document(ctx, path);
		if (fz_needs_password(ctx, doc)) {
			int okay = 0;
			char *password;
			char defaultValue = '\0';
			while (!okay) {
				if (show_msg_user_input("nPDF", "This document requires a password:", &defaultValue, &password) == -1 ) {
					break;
				}
				okay = fz_authenticate_password(ctx, doc, password);
			}
			if (!okay) {
				fz_throw(ctx, FZ_ERROR_GENERIC, "password not provided");
			}
		}
	} fz_catch(ctx) {
		return false;
	}
	return true;
}

unsigned int Document::getPages() {
	int pages = fz_count_pages(ctx, doc);
	if (pages >= 0) return static_cast<unsigned int>(pages);
	return 0;
}

fz_page* Document::ensureCurrentPageLoaded() {
	if (currentlyLoadedPageNo != pageNo) {
		if (page != nullptr) fz_drop_page(ctx, page);
		page = fz_load_page(ctx, doc, pageNo);
		fz_bound_page(ctx, page, &bounds);
		currentlyLoadedPageNo = pageNo;
	}
	return page;
}

bool Document::next() {
	return gotoPage(pageNo + 1);
}

bool Document::prev() {
	return gotoPage(pageNo - 1);
}

const fz_rect& Document::getBounds() {
	return bounds;
}

bool Document::gotoPage(unsigned int page){
	printf("gotoPage(%i)\n", page);
	if (page < getPages()) {
		resetFind();
		pageNo = page;
		ensureCurrentPageLoaded();
		return true;
	}
	return false;
}

const fz_rect* Document::getCurrentMatch() {
	if (matchesCount > 0) {
		return &matches[matchIdx];
	}
	return nullptr;
}

void Document::resetFind() {
	matchesCount = 0;
	matchIdx = 0;
}

const fz_rect* Document::find(char *s) {
	printf("find\n");
	if (matchingFor != nullptr && matchingFor != s){
		// free(matchingFor);
		matchingFor = nullptr;
	}
	matchingFor = s;

	resetFind();
	auto iter = PageIterator(pageNo, getPages(), Direction::FORWARD);
	return gotoNextPageWithOccurrence(iter);
}

int Document::scanPages(PageIterator& iter, int* outPage){
	int cnt = 0;
	int page = iter.current;
	while(page >= 0){
		cnt = fz_search_page_number(ctx, doc, page, matchingFor, matches, MATCH_LIMIT);
		if (cnt > 0) {
			*outPage = page;
			return cnt;
		}
		page = iter.next();
	};
	return 0;
}

fz_rect* Document::gotoNextPageWithOccurrence(PageIterator& iter) {
	int foundOnPage{0};
	int cnt = scanPages(iter, &foundOnPage);
	if (cnt > 0) {
		gotoPage(foundOnPage);
		matchesCount = cnt;
		if (iter.dir == Direction::BACKWARD) matchIdx = matchesCount - 1;
		else matchIdx = 0;

		// Sort for more intuitive ordering
		std::sort(matches, matches + matchesCount,
			[](const fz_rect& a, const fz_rect& b) -> bool {
				return a.y0 < b.y0 || (a.y0 == b.y0 && a.x0 < b.x0); // prioritize y over x
			}
		);
		return &matches[matchIdx];
	}
	return nullptr;
}

const fz_rect* Document::findNext(Direction dir) {
	if (matchingFor){
		if (
			(matchIdx < matchesCount - 1 && dir == Direction::FORWARD)
			|| 	(matchIdx > 0 && dir == Direction::BACKWARD)
		) {
			// Page has already been searched and more matches are available
			matchIdx += dir;
			return &matches[matchIdx];
		}
		// Look for matches on other pages.
		auto iter = PageIterator(pageNo, getPages(), dir);
		iter.next(); // Skip current page
		return gotoNextPageWithOccurrence(iter);
	}
	return nullptr;
}


// We have a separate initialization method for the error handling
Viewer::Viewer()
	: width{SCREEN_WIDTH}
	, height{SCREEN_HEIGHT}
	, doc{nullptr}
	{
	ctx = fz_new_context(nullptr, nullptr, 16 << 20);
	if (ctx) {
		fz_register_document_handlers(ctx);
	} else {
		throw "Could not allocate MuPDF context";
	}
	pix = nullptr;
	scale = 1.0f;
	xPos = 0;
	yPos = 0;
	fitWidth = true;
}

Viewer::~Viewer() {
	this->doc.reset(); // TODO: Needed as doc depends on ctx for shutdown
	fz_drop_pixmap(ctx, pix);
	fz_drop_context(ctx);
}

void Viewer::invertPixels(const fz_rect *rect) {
	fz_irect b;
	fz_rect r = *rect;
	fz_round_rect(&b, fz_transform_rect(&r, &transform));
	fz_invert_pixmap_rect(ctx, pix, &b);
}

void Viewer::ensureInView(const fz_rect *rect) {
	if (xPos > rect->x0 || xPos + width < rect->x1) xPos = (rect->x0 + rect->x1) / 2 - width / 2;
	if (yPos > rect->y0 || yPos + height < rect->y1) yPos = (rect->y0 + rect->y1) / 2 - height / 2;
}

bool Viewer::find(char *s) {
	const fz_rect* match = doc->getCurrentMatch();
	if (match) invertPixels(match);

	match = doc->find(s);
	if (match) {
		ensureInView(match);
		drawPage();
		return true;
	}
	return false;
}

bool Viewer::findNext(Direction dir) {
	const fz_rect* match = doc->getCurrentMatch();
	if (match) invertPixels(match);

	match = doc->findNext(dir);
	if (match) {
		ensureInView(match);
		drawPage();
		return true;
	}
	return false;
}

void Viewer::openDoc(const char *path) {
	auto d = std::make_unique<Document>(ctx);
	bool success = d->open(path);
	doc = std::move(d);
	if (!success) {
		show_msgbox("nPDF", "Can't open document");
	}
}

int Viewer::getPages() {
	return doc->getPages();
}

void Viewer::fixBounds() {
	// Make sure we don't go out of bounds
	const int boundsWidth = static_cast<int>(bounds.x1 - bounds.x0);
	const int boundsHeight = static_cast<int>(bounds.y1 - bounds.y0);
	const int maxAllowedWidth = boundsWidth - std::min(width, boundsWidth);
	const int maxAllowedHeight = boundsHeight - std::min(height, boundsHeight);
	if (xPos < 0 || boundsWidth <= width) xPos = 0;
	else xPos = std::min(xPos, maxAllowedWidth);

	if (yPos < 0 || boundsHeight <= height) yPos = 0;
	else yPos = std::min(yPos, maxAllowedHeight);
}

void Viewer::drawPage() {
	fz_drop_pixmap(ctx, pix);

	pix = nullptr;

	fz_page* page = doc->ensureCurrentPageLoaded();
	bounds = doc->getBounds();

	if (fitWidth) {
		scale = width / (bounds.x1 - bounds.x0);
	}

	fz_scale(&transform, scale, scale);
	fz_transform_rect(&bounds, &transform);

	fz_irect bbox;
	fz_round_rect(&bbox, &bounds);

	fixBounds();

	if (has_colors) {
		pix = fz_new_pixmap_with_bbox(ctx, fz_device_rgb(ctx), &bbox, nullptr, 1);
	} else {
		pix = fz_new_pixmap_with_bbox(ctx, fz_device_gray(ctx), &bbox, nullptr, 1);
	}
	fz_clear_pixmap_with_value(ctx, pix, 0xff);

	fz_device *dev = fz_new_draw_device(ctx, nullptr, pix);
	fz_run_page(ctx, page, dev, &transform, nullptr);
	fz_close_device(ctx, dev);
	fz_drop_device(ctx, dev);
	dev = nullptr;

	const fz_rect* match = doc->getCurrentMatch();
	if (match) invertPixels(match);
}

void Viewer::display() {
	fixBounds();

	// Center it
	int x = 0, y = 0;
	if (pix->w < width) {
		x = (width - pix->w) / 2;
		Screen::fillRect(bgColor, 0, 0, x, height);
		Screen::fillRect(bgColor, x + pix->w, 0, width - (x + pix->w), height);
	}
	if (pix->h < height) {
		y = (height - pix->h) / 2;
		Screen::fillRect(bgColor, 0, 0, width, y);
		Screen::fillRect(bgColor, y + pix->h, 0, width, height - (y + pix->h));
	}
	if (has_colors) {
		Screen::showImgRGBA(pix->samples, x, y, xPos, yPos, std::min(width, pix->w), std::min(height, pix->h), pix->w);
	} else {
		Screen::showImgGrayA(pix->samples, x, y, xPos, yPos, std::min(width, pix->w), std::min(height, pix->h), pix->w);
	}

	if ((bounds.y1-bounds.y0)>height) {
		Screen::drawVert(0,0,0,width-1,0,height-4);
		Screen::drawVert(0,0,0,width-5,0,height-4);
		Screen::drawHoriz(0,0,0,width-4,0,3);
		Screen::drawHoriz(0,0,0,width-4,height-5,3);
		Screen::fillRect(255,255,255,width-4, 1, 3, height-6);
		Screen::drawVert(0,0,0,width-3,2+yPos*(height-8)/(bounds.y1-bounds.y0),height*(height-7)/(bounds.y1-bounds.y0));
	}

	if ((bounds.x1-bounds.x0)>width) {
		Screen::drawHoriz(0,0,0,0,height-1,width-4);
		Screen::drawHoriz(0,0,0,0,height-5,width-4);
		Screen::drawVert(0,0,0,0,height-4,3);
		Screen::drawVert(0,0,0,width-5,height-4,3);
		Screen::fillRect(255,255,255,1,height-4, width-6, 3);
		Screen::drawHoriz(0,0,0,2+xPos*(width-8)/(bounds.x1-bounds.x0),height-3,width*(width-7)/(bounds.x1-bounds.x0));
	}

	Screen::display();
}

void Viewer::next() {
	if (doc->next()){
		yPos = 0;
		drawPage();
	}
}

void Viewer::prev() {
	if (doc->prev()){
		yPos = 0;
		drawPage();
	}
}

void Viewer::scrollUp() {
	if (yPos > 0) {
		yPos -= scroll;
		yPos = (yPos<0)?0:yPos;
	}
}

void Viewer::scrollDown() {
	if (yPos < (bounds.y1 - bounds.y0) - height) {
		yPos += scroll;
		yPos = (yPos > (bounds.y1 - bounds.y0) - height) ? (bounds.y1 - bounds.y0) - height : yPos;
	}
}

void Viewer::scrollLeft() {
	if (xPos > 0) {
		xPos -= scroll;
		xPos = (xPos<0)?0:xPos;
	}
}

void Viewer::scrollRight() {
	if (xPos < (bounds.x1 - bounds.x0) - width ) {
		xPos += scroll;
		xPos = (xPos > (bounds.x1 - bounds.x0) - width) ? (bounds.x1 - bounds.x0) - width : xPos;
	}
}

void Viewer::setFitWidth() {
	fitWidth = true;
	drawPage();
}

void Viewer::unsetFitWidth() {
	fitWidth = false;
}

void Viewer::zoomIn() {
	// Try to zoom in on the center
	if (scale * zoom <= maxScale) {
		fitWidth = false;
		xPos = (xPos + std::min(width, static_cast<int>(bounds.x1 - bounds.x0)) / 2) * zoom;
		xPos -= std::min(width, static_cast<int>((bounds.x1 - bounds.x0) * zoom)) / 2;
		yPos = (yPos + std::min(height, static_cast<int>(bounds.y1 - bounds.y0)) / 2) * zoom;
		yPos -= std::min(height, static_cast<int>((bounds.y1 - bounds.y0) * zoom)) / 2;
		scale *= zoom;
		drawPage();
	}
}

void Viewer::zoomOut() {
	// Try to zoom out from the center
	if (scale / zoom >= minScale) {
		fitWidth = false;
		xPos = (xPos + std::min(width, static_cast<int>(bounds.x1 - bounds.x0)) / 2) / zoom;
		xPos -= std::min(width, static_cast<int>((bounds.x1 - bounds.x0) / zoom)) / 2;
		yPos = (yPos + std::min(height, static_cast<int>(bounds.y1 - bounds.y0)) / 2) / zoom;
		yPos -= std::min(height, static_cast<int>((bounds.y1 - bounds.y0) / zoom)) / 2;
		scale /= zoom;
		drawPage();
	}
}

void Viewer::gotoPage(unsigned int page) {
	if (doc->gotoPage(page)){
		yPos = 0;
		drawPage();
	}
}
