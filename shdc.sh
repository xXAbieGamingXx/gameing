input="$1"
suffix=".h"
new_name="${input}${suffix}"
rm -rf new_name # make sure it gets regenerated
tools/sokol-shdc --input "${1}" --output "$new_name" --slang glsl430:hlsl5:metal_macos:spirv_vk
