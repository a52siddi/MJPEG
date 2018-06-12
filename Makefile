#Set up directories

OBJ_DIR = src
SRC_DIR = src
INC_DIR = include

# Define compilation flags

CC = gcc -g #-DINFO #-pg
INC = -I$(INC_DIR) -I/opt/local/include/ -I/usr/include/ -I/usr/local/cuda/include/
CFLAGS = $(INC) -Wall -std=c99 #-g
LDFLAGS = -L/opt/local/lib/ -L/usr/local/cuda/lib64/ -lSDL -lOpenCL#-g

# Define object files list

GENERATED_OBJ_FILES =  $(OBJ_DIR)/screen.o 
OBJ_FILES = $(GENERATED_OBJ_FILES) $(OBJ_DIR)/main.o $(OBJ_DIR)/idct.o  $(OBJ_DIR)/conv.o $(OBJ_DIR)/iqzz.o 
OBJ_FILES += $(OBJ_DIR)/upsampler.o $(OBJ_DIR)/skip_segment.o $(OBJ_DIR)/huffman.o $(OBJ_DIR)/unpack_block.o 
NEW_OBJ_FILES =

all : mjpeg mire

# Real compilation

mire : $(OBJ_DIR)/example.o $(OBJ_DIR)/screen.o
	$(CC) -o mire $(OBJ_DIR)/example.o $(OBJ_DIR)/screen.o $(LDFLAGS)
mjpeg : $(OBJ_FILES)
	$(CC) -o mjpeg $(OBJ_FILES) $(LDFLAGS)
$(OBJ_DIR)/screen.o : $(SRC_DIR)/screen.c $(INC_DIR)/screen.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/screen.c -o $(OBJ_DIR)/screen.o
$(OBJ_DIR)/example.o : example/example.c 
	$(CC) $(CFLAGS) -g -c example/example.c -o $@

# START CUT 
$(OBJ_DIR)/main.o : $(SRC_DIR)/main.c $(INC_DIR)/jpeg.h $(INC_DIR)/screen.h $(INC_DIR)/upsampler.h $(INC_DIR)/huffman.h $(INC_DIR)/skip_segment.h $(INC_DIR)/idct.h $(INC_DIR)/iqzz.h $(INC_DIR)/conv.h $(INC_DIR)/utils.h $(INC_DIR)/define_common.h $(INC_DIR)/unpack_block.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/main.c -o $@
$(OBJ_DIR)/idct.o : $(SRC_DIR)/idct.c $(INC_DIR)/idct.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/idct.c -o $@
$(OBJ_DIR)/conv.o : $(SRC_DIR)/conv.c $(INC_DIR)/conv.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/conv.c -o $@
$(OBJ_DIR)/iqzz.o : $(SRC_DIR)/iqzz.c $(INC_DIR)/iqzz.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/iqzz.c -o $@
$(OBJ_DIR)/upsampler.o : $(SRC_DIR)/upsampler.c $(INC_DIR)/upsampler.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/upsampler.c -o $@
$(OBJ_DIR)/skip_segment.o : $(SRC_DIR)/skip_segment.c $(INC_DIR)/skip_segment.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/skip_segment.c -o $@
$(OBJ_DIR)/huffman.o : $(SRC_DIR)/huffman.c $(INC_DIR)/huffman.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/huffman.c -o $@
$(OBJ_DIR)/unpack_block.o : $(SRC_DIR)/unpack_block.c $(INC_DIR)/unpack_block.h
	$(CC) $(CFLAGS) -c $(SRC_DIR)/unpack_block.c -o $@
# END CUT

clean:
	rm -f mjpeg $(GENERATED_OBJ_FILES) $(NEW_OBJ_FILES)
	rm -f mire $(OBJ_DIR)/example.o
# START CUT
	rm -f $(OBJ_FILES)
# END CUT
