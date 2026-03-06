#pragma once

typedef struct mouse_state{
	bool lmb, rmb, mmb;
	float x, y, dx, dy, scroll;
} mouse_state;

void handle_input(const sapp_event* ev);

bool key_down(sapp_keycode code);

mouse_state get_mouse_state();
