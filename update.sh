#!/bin/bash
base_url="https://raw.githubusercontent.com/floooh/sokol/refs/heads/master/" # this has the actual contents of the file

files=("sokol_app.h" "sokol_args.h" "sokol_audio.h" "sokol_fetch.h" "sokol_gfx.h" "sokol_glue.h" "sokol_log.h" "sokol_time.h")


for file in "${files[@]}"
do
	rm -rf $file
	wget "$base_url$file"
done
