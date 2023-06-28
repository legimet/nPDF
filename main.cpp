// nPDF main function
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

#include <keys.h>
#include <libndls.h>
#include <string>
#include "Viewer.hpp"
#include "Screen.hpp"
#include "Timer.hpp"

// Actions that, when held down, result in a long delay initially and a short delay afterwards
enum ScrollAction {
	none = 0,
	down = 1,
	up = 2,
	right = 4,
	left = 8,
	pgdown = 16,
	pgup = 32,
	zoomout = 64,
	zoomin = 128,
	findnext = 256,
	findprev = 512,
	resetzoom = 1024,
};

ScrollAction operator|(ScrollAction a, ScrollAction b) {
	return static_cast<ScrollAction>(static_cast<int>(a) | static_cast<int>(b));
}

ScrollAction& operator|=(ScrollAction& a, ScrollAction b) {
	return (a = a | b);
}

const int longDelay = 400;
const int shortDelay = 40;

ScrollAction getScrollKey() {
	ScrollAction action=none;
	if (isKeyPressed(KEY_NSPIRE_DOWN) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN) || isKeyPressed(KEY_NSPIRE_DOWNLEFT) ||
			isKeyPressed(KEY_NSPIRE_2) || isKeyPressed(KEY_NSPIRE_3) || isKeyPressed(KEY_NSPIRE_1))
		action |= down;
	if (isKeyPressed(KEY_NSPIRE_UP) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_LEFTUP) ||
			isKeyPressed(KEY_NSPIRE_8) || isKeyPressed(KEY_NSPIRE_7) || isKeyPressed(KEY_NSPIRE_9))
		action |= up;
	if (isKeyPressed(KEY_NSPIRE_RIGHT) || isKeyPressed(KEY_NSPIRE_UPRIGHT) || isKeyPressed(KEY_NSPIRE_RIGHTDOWN) ||
			isKeyPressed(KEY_NSPIRE_6) || isKeyPressed(KEY_NSPIRE_9) || isKeyPressed(KEY_NSPIRE_3))
		action |= right;
	if (isKeyPressed(KEY_NSPIRE_LEFT) || isKeyPressed(KEY_NSPIRE_DOWNLEFT) || isKeyPressed(KEY_NSPIRE_LEFTUP) ||
			isKeyPressed(KEY_NSPIRE_4) || isKeyPressed(KEY_NSPIRE_7) || isKeyPressed(KEY_NSPIRE_1))
		action |= left;
	if (isKeyPressed(KEY_NSPIRE_PLUS))
		action |= pgdown;
	if (isKeyPressed(KEY_NSPIRE_MINUS))
		action |= pgup;
	if (isKeyPressed(KEY_NSPIRE_DIVIDE))
		action |= zoomout;
	if (isKeyPressed(KEY_NSPIRE_MULTIPLY))
		action |= zoomin;
	if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_G)) {
		if (isKeyPressed(KEY_NSPIRE_SHIFT))
			action |= findprev;
		else
			action |= findnext;
	}
	if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_R))
		action |= resetzoom;
	return action;
}

void handleDelays(ScrollAction key, ScrollAction& lastScrollKey) {
	if (lastScrollKey != key) {
		Timer::start(longDelay);
		lastScrollKey = key;
	} else if (Timer::done()) {
		Timer::start(shortDelay);
	}
}

void registerExtensions(char* executable) {
	std::string s = std::string(executable);
	size_t pos = s.find_last_of("/");
	if (pos != std::string::npos) {
		s.erase(0, s.find_last_of("/") + 1);
	}
	if (s.size() >= 4 && s.substr(s.size() - 4) == ".tns") {
		s.erase(s.size() - 4);
	}
	cfg_register_fileext("pdf", s.c_str());
	cfg_register_fileext("xps", s.c_str());
	cfg_register_fileext("cbz", s.c_str());
	cfg_register_fileext("xml", s.c_str());
	cfg_register_fileext("xhtml", s.c_str());
	cfg_register_fileext("html", s.c_str());
	cfg_register_fileext("htm", s.c_str());
	cfg_register_fileext("epub", s.c_str());
	show_msgbox("nPDF", "File extensions registered. You can now open a .pdf, .xps, .cbz, .xml, .xhtml, .html, .htm and .epub files from the Documents screen");
}

int main(int argc, char **argv) {
	if (argc == 0) {
		show_msgbox("nPDF", "Executable called with no arguments. Exiting.");
		return 1;
	} else if (argc == 1) {
		registerExtensions(argv[0]);
		return 0;
	}

	try {
		Viewer v{};
		v.openDoc(argv[1]);

		Screen screen{};
		Timer::init();
		v.drawPage();
		v.display();

		ScrollAction lastScrollKey = none;
		ScrollAction current = none;

		int page;
		bool toRefresh;
		while (true) {
			if ((current = getScrollKey())) {
				if (current != lastScrollKey || Timer::done()) {
					toRefresh = false;
					if (current & down) {
						v.scrollDown();
						toRefresh = 1;
					} else if (current & up) {
						v.scrollUp();
						toRefresh = 1;
					}
					if (current & right) {
						v.scrollRight();
						toRefresh = 1;
					} else if (current & left) {
						v.scrollLeft();
						toRefresh = 1;
					}
					if (current & pgdown) {
						v.next();
						toRefresh = 1;
					} else if (current & pgup) {
						v.prev();
						toRefresh = 1;
					}
					if (current & zoomout) {
						v.zoomOut();
						toRefresh = 1;
					} else if (current & zoomin) {
						v.zoomIn();
						toRefresh = 1;
					} else if (current & resetzoom) {
						v.setFitSize(true);
						toRefresh = 1;
					}
					if (current & findnext) {
						v.findNext(Direction::FORWARD);
						toRefresh = 1;
					} else if (current & findprev) {
						v.findNext(Direction::BACKWARD);
						toRefresh = 1;
					}
					if (toRefresh) {
						handleDelays(current, lastScrollKey);
						v.display();
					}
				}
			} else {
				lastScrollKey = none;
				Timer::stop();
				if (isKeyPressed(KEY_NSPIRE_ESC)) {
					break;
				}
				if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_TAB)) {
					wait_no_key_pressed();
					if (show_1numeric_input("Go to page", "", "Enter page number", &page, 1, v.getPages())) {
						if (page > 0){
							v.gotoPage(static_cast<unsigned int>(page - 1));
						}
					}
					v.display();
				}
				if (isKeyPressed(KEY_NSPIRE_CTRL) && isKeyPressed(KEY_NSPIRE_F)) {
					char *s = nullptr;
					wait_no_key_pressed();
					int len = show_msg_user_input("Find", "Enter search string", "", &s);
					if (len > 0) {
						v.find(s);
					}
					v.display();
				}
			}
			msleep(10);
		}
	} catch(const char *s) {
		show_msgbox("nPDF", s);
	}

	return 0;
}
