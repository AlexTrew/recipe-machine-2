CC=aarch64-linux-gnu-gcc
CFLAGS=`aarch64-linux-gnu-pkg-config --cflags gtk+-3.0`
LDFLAGS=`aarch64-linux-gnu-pkg-config --libs gtk+-3.0`
TARGET=recipe-machine-2-aarch64
SRC=main.c
ARCH=arm64

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET)
