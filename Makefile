CC = mpicc
all: bubble_sort.c bubble_sort_no_delta.c
	$(CC) -o bubble_sort bubble_sort.c -lm
	$(CC) -o bubble_sort_no_delta bubble_sort_no_delta.c -lm

clean:
	rm -f bubble_sort bubble_sort_no_delta

.PHONY: all clean

