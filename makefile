CC = mpicc
CFLAGS = -lm

SRCS = project2.c ring_broadcast.c readEvent.c haversine.c
OBJS = $(SRCS:.c=.o)
TARGET = project2

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(CFLAGS)

%.o: %.c
	$(CC) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)
