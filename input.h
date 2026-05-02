#pragma once

typedef struct sapp_event sapp_event;
typedef enum sapp_keycode sapp_keycode;

struct mouse_state{
	bool lmb, rmb, mmb;
	float dx, dy, scroll;
};

void reset_mouse(struct mouse_state* mouse);

void handle_input(const sapp_event* ev, struct mouse_state* mouse);

bool key_down(sapp_keycode code);

struct mouse_state get_mouse_state();
