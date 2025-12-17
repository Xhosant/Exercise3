.PHONY: sort clean

sort:
	@mkdir -p build
	gcc -g -I ./include -L ./lib -Wl,-rpath,./lib \
	./examples/sort_main.c ./src/record.c ./src/sort.c ./src/chunk.c ./src/merge.c \
	-lbf -lhp_file -lm \
	-o ./build/sort_main

clean:
	rm -rf build
experiment:
	@mkdir -p build
	gcc -g -I ./include -L ./lib -Wl,-rpath,./lib \
	./examples/experiment_main.c ./src/record.c ./src/sort.c ./src/chunk.c ./src/merge.c \
	-lbf -lhp_file -lm \
	-o ./build/experiment_main
matrix:
	@mkdir -p build
	gcc -g -I ./include -L ./lib -Wl,-rpath,./lib \
	./examples/experiment_matrix_main.c ./src/record.c ./src/sort.c ./src/chunk.c ./src/merge.c \
	-lbf -lhp_file -lm \
	-o ./build/matrix_main

scaling:
	@mkdir -p build
	gcc -g -I ./include -L ./lib -Wl,-rpath,./lib \
	./examples/experiment_scaling_main.c ./src/record.c ./src/sort.c ./src/chunk.c ./src/merge.c \
	-lbf -lhp_file -lm \
	-o ./build/scaling_main

verify:
	@mkdir -p build
	gcc -g -I ./include -L ./lib -Wl,-rpath,./lib \
	./examples/verify_sorted_main.c ./src/record.c ./src/sort.c ./src/chunk.c ./src/merge.c \
	-lbf -lhp_file -lm \
	-o ./build/verify_main
