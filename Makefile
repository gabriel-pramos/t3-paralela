CC = mpicc
all: bubble_sort.c bubble_sort_no_delta.c bubble_sort2.c
	$(CC) -o bubble_sort bubble_sort.c -lm
	$(CC) -o bubble_sort_no_delta bubble_sort_no_delta.c -lm
	$(CC) -o bubble_sort2 bubble_sort2.c -lm

clean:
	rm -f bubble_sort bubble_sort_no_delta bubble_sort2

.PHONY: all clean

