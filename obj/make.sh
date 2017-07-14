#!/bin/bash
rm ./*.o libh264.a
g++ -c ../src/*.c -I../inc/ -Wall -O -DBUILD_TESTS -w -fpermissive -g

ar r libh264.a common.o input.o in_file.o coretrans.o nal.o cavlc.o \
	params.o slicehdr.o slice.o mbmodes.o residual.o block.o \
	mode_pred.o intra_pred.o mocomp.o\
	main.o perf.o modules.o

rm ./*.o

g++ -c ../src/*.cpp -I../inc/ -I../orion3/ -I/usr/X11R6/include -w -fpermissive -g  
g++ -I../inc/ -I../orion3/ *.o libh264.a -o exe -L/usr/X11R6/lib -lm -lX11 -L../orion3 -lm -lpower `sdl-config --libs` -g
#./exe traffic: IPCORE ary_size: 3 injection_rate: 0.1 cycles: 10000 do_dvfs: 0 use_boost: 0 hist_window: 500 
