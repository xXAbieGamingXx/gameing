#!/bin/bash
set -e

cd "$(dirname "$0")"

mapfile -t files < <(cd textures && ls *.png 2>/dev/null | LC_ALL=C sort | tr -d '\r')
count=${#files[@]}
if [ "$count" -eq 0 ]; then
	echo "gen_textures.sh: no .png in textures/"
	exit 1
fi

{
	echo "#pragma once"
	echo "#define TEXTURE_COUNT $count"
	echo "static const char* const texture_paths[TEXTURE_COUNT] = {"
	for f in "${files[@]}"; do
		echo "	\"textures/$f\","
	done
	echo "};"
	echo "static const char* const texture_names[TEXTURE_COUNT] = {"
	for f in "${files[@]}"; do
		name="${f%.*}"
		echo "	\"$name\","
	done
	echo "};"
} > textures.h

bindings=""
cases=""
for i in "${!files[@]}"; do
	bindings+="layout(binding=$i) uniform texture2D tex$i;"$'\n'
	cases+="		case $i: frag_color = texture(sampler2D(tex$i, smp), uv); break;"$'\n'
done

awk -v b="$bindings" -v c="$cases" '
{
	gsub(/@SAMPLER_BINDINGS@/, b)
	gsub(/@SAMPLER_SWITCH@/, c)
	print
}
' triangle.glsl.in > triangle.glsl
