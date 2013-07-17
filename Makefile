CC=gcc -g -I./include
 
LDFLAGS += -lreadline -lusb-1.0  -lcurses -lpthread

TARGET = ejtag

all: $(TARGET)

OBJS += main.o mips_ejtag.o command.o misc.o gdb_server.o mips_gdb.o

OBJS += nand_flash.o nand_ids.o

$(TARGET): $(OBJS) 
	$(CC)  -o $@ $^ -lc $(LDFLAGS) 
clean:
	rm -f *.o $(TARGET) ${OBJS} 


