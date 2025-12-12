sort:
	@echo " Compile sort_main ...";
	gcc -g -I ./include/ -L ./lib/ -Wl,-rpath,./lib/ ./examples/sort_main.c ./src/record.c ./src/sort.c ./src/merge.c ./src/chunk.c -lbf lib/libbf.so -lhp_file lib/libhp_file.so -o ./build/sort_main -O2
	@echo "BUILD_OUTPUT=build/sort_main"