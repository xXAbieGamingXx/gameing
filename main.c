#define SOKOL_IMPL
#define SOKOL_GLCORE
#define SOKOL_WWIN32_FORCE_WINMAIN
#include "sokol_gfx.h"
#include "sokol_app.h"
#include "sokol_log.h"
#include "sokol_glue.h"

        // - with MINGW/MSYS2 gcc:
            // - compile with '-mwin32' so that _WIN32 is defined
            // - link with the following libs: -lkernel32 -luser32 -lshell32
            // - additionally with the GL backend: -lgdi32
            // - additionally with the D3D11 backend: -ld3d11 -ldxgi

sg_pass_action pass_action;

void init(){
	sg_setup(&(sg_desc){
		.environment = sglue_environment(),
		.logger.func = slog_func,
	});
	pass_action = (sg_pass_action){
		.colors[0] = {
			.load_action = SG_LOADACTION_CLEAR,
			.clear_value = {1.0f, .0f, .0f, 1.0f}
		}
	};
}

void frame(){
	sg_begin_pass(&(sg_pass){
		.action = pass_action,
		.swapchain = sglue_swapchain(),
	});
}

void cleanup(){
	sg_shutdown();
}

sapp_desc sokol_main(int argc, char* argv[]){
	(void)argc; (void)argv; 
	return (sapp_desc) {
		.width = 640,
		.height = 480,
		.init_cb = init,
		.frame_cb = frame,
		.cleanup_cb = cleanup,
		.logger.func = slog_func,
		.window_title = "gmaing",
	};
}
