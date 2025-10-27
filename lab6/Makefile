CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
TARGET_CLIENT = client
TARGET_SERVER = server
SRC_DIR = src
OBJ_DIR = obj

SOURCES_CLIENT = $(SRC_DIR)/client.c
SOURCES_SERVER = $(SRC_DIR)/server.c
SOURCES_COMMON = $(SRC_DIR)/common.c

OBJECTS_CLIENT = $(SOURCES_CLIENT:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJECTS_SERVER = $(SOURCES_SERVER:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
OBJECTS_COMMON = $(SOURCES_COMMON:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

all: $(TARGET_CLIENT) $(TARGET_SERVER)

$(TARGET_CLIENT): $(OBJECTS_CLIENT) $(OBJECTS_COMMON)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

$(TARGET_SERVER): $(OBJECTS_SERVER) $(OBJECTS_COMMON)
	$(CC) $(CFLAGS) -o $@ $^ -pthread

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

clean:
	rm -rf $(OBJ_DIR) $(TARGET_CLIENT) $(TARGET_SERVER)

.PHONY: all clean