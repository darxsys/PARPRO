CC=mpicc
LD=mpicc

NAME=connect4

CC_FLAGS = -c -Wall
LD_FLAGS = 

SRC_DIR=src
OBJ_DIR=obj

SRC=$(shell find $(SRC_DIR) -type f -regex ".*\.c")
OBJ=$(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

EXEC=$(NAME)

all: $(EXEC)

$(EXEC): $(OBJ)
	@echo [LD] $@
	@mkdir -p $(dir $@)
	$(LD) -o $@ $^ $(LD_FLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo [CC]
	@mkdir -p $(dir $@)
	$(CC) -o $@ $< $(CC_FLAGS)

clean:
	rm $(EXEC) $(OBJ_DIR) -rf
