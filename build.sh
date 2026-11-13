CC=gcc CXX=g++ meson setup build --reconfigure -Dc_args="-std=c23 -ggdb"
meson compile -C build && mv build/gaming . && ./gaming
mv build/map_tool tools/
mv build/glb_tool tools/
