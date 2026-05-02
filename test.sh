rm -rf builddir/unittest.exe
CC=gcc CXX=g++ meson setup builddir --reconfigure -Dc_args="-std=c23 -g"
meson compile -C builddir
./builddir/unittest
