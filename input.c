#include "sokol_app.h"

bool key_states[SAPP_MAX_KEYCODES] = {0};
struct mouse_state {
	bool lmb, rmb, mmb;
	float x, y, dx, dy, scroll; // dx dy are changes
};


void handle_input(const sapp_event* ev, struct mouse_state* mouse){
	if(ev->type == SAPP_EVENTTYPE_KEY_DOWN){
		key_states[ev->key_code] = true;
	}else if(ev->type == SAPP_EVENTTYPE_KEY_UP){
		key_states[ev->key_code] = false;
	}else if(ev->type == SAPP_EVENTTYPE_MOUSE_DOWN){
		if(ev->mouse_button == SAPP_MOUSEBUTTON_LEFT){
			mouse->lmb = true;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT){
			mouse->rmb = true;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_MIDDLE){
			mouse->mmb = true;
		}
	}else if(ev->type == SAPP_EVENTTYPE_MOUSE_UP){
		if(ev->mouse_button == SAPP_MOUSEBUTTON_LEFT){
			mouse->lmb = false;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_RIGHT){
			mouse->rmb = false;
		}else if(ev->mouse_button == SAPP_MOUSEBUTTON_MIDDLE){
			mouse->mmb = false;
		}
	}else if(ev->type == SAPP_EVENTTYPE_MOUSE_SCROLL){
		mouse->scroll = ev->scroll_y;
	}

	mouse->dx = ev->mouse_dx;
	mouse->x = ev->mouse_x;
	mouse->dy = ev->mouse_dy;
	mouse->y = ev->mouse_y;
}

bool key_down(sapp_keycode code){
	return key_states[code];
}
