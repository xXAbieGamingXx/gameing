rm -rf build/unittest
CC=gcc CXX=g++ meson setup build --reconfigure -Dc_args="-std=c23 -g"
meson compile -C build
./build/unittest
