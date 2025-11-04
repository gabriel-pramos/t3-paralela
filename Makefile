CC = mpicc
TARGET = bubble_sort
SRC = bubble_sort.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

run: $(TARGET)
	mpirun -np 8 ./$(TARGET) 200000 1000

.PHONY: all clean run

