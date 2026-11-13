files=("sokol_app.h" "sokol_args.h" "sokol_audio.h" "sokol_fetch.h" "sokol_gfx.h" "sokol_glue.h" "sokol_log.h" "sokol_time.h")
rm -rf vecmath.h
rm -rf stb_image.h

cmds=("floooh/sokol sokol_app.h sokol_app.h" "floooh/sokol sokol_args.h sokol_args.h" "floooh/sokol sokol_audio.h sokol_audio.h" "floooh/sokol sokol_fetch.h sokol_fetch.h" \
	"floooh/sokol sokol_gfx.h sokol_gfx.h" "floooh/sokol sokol_glue.h sokol_glue.h" "floooh/sokol sokol_log.h sokol_log.h" "floooh/sokol sokol_time.h sokol_time.h" \
	"mattiasgustavsson/libs vecmath.h util/vecmath.h" "nothings/stb stb_image.h util/stb/stb_image.h" "jkuhlmann/cgltf cgltf.h util/cgltf.h")

for cmd in "${cmds[@]}"
do
	./tools/git-file.sh $cmd
done

sed -i "1i #pragma GCC system_header" util/vecmath.h
