#include <stdio.h>
#include "sokol_app.h"

bool key_states[SAPP_MAX_KEYCODES] = {0};
struct mouse_state {
	bool lmb, rmb, mmb;
	float dx, dy, scroll; // dx dy are changes
};

void reset_mouse(struct mouse_state* mouse) {
	mouse->lmb = false;
	mouse->rmb = false;
	mouse->mmb = false;
	mouse->dx = 0.0f;
	mouse->dy = 0.0f;
	mouse->scroll = 0.0f;
}

void handle_input(const sapp_event* ev, struct mouse_state* mouse) {
	if(ev->type == SAPP_EVENTTYPE_KEY_DOWN) {
		key_states[ev->key_code] = true;
	}else if(ev->type == SAPP_EVENTTYPE_KEY_UP) {
		key_states[ev->key_code] = false;
	}else if(ev->type == SAPP_EVENTTYPE_MOUSE_DOWN) {
		if(ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
			mouse->lmb = true;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
			mouse->rmb = true;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_MIDDLE) {
			mouse->mmb = true;
		}
	}else if(ev->type == SAPP_EVENTTYPE_MOUSE_UP) {
		if(ev->mouse_button == SAPP_MOUSEBUTTON_LEFT) {
			mouse->lmb = false;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT) {
			mouse->rmb = false;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_MIDDLE) {
			mouse->mmb = false;
		}
	}else if(ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL) {
		mouse->scroll = ev->scroll_y;
	}

	if(ev->type == SAPP_EVENTTYPE_MOUSE_MOVE) {
		mouse->dx = ev->mouse_dx;
		mouse->dy = ev->mouse_dy;
	}
}

bool key_down(sapp_keycode code) {
	return key_states[code];
}
