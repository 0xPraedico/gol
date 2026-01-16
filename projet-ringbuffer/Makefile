CC ?= cc

PROJECT := life
BENCH := life_bench

SRC_DIR := src
INC_DIR := include
BUILD_DIR := build
BIN_DIR := bin

SDL_CFLAGS := $(shell pkg-config --cflags sdl2 2>/dev/null)
SDL_LIBS := $(shell pkg-config --libs sdl2 2>/dev/null)
ifeq ($(strip $(SDL_CFLAGS)$(SDL_LIBS)),)
SDL_CFLAGS := $(shell sdl2-config --cflags 2>/dev/null)
SDL_LIBS := $(shell sdl2-config --libs 2>/dev/null)
endif
ifeq ($(strip $(SDL_CFLAGS)$(SDL_LIBS)),)
$(error SDL2 non trouvé. Installez libsdl2-dev (Debian/Ubuntu) ou SDL2 via votre gestionnaire de paquets.)
endif

CSTD := -std=c11
WARN := -Wall -Wextra -Wpedantic

APP_OPT ?= -O2
BENCH_OPT ?= -O3

APP_CFLAGS ?= $(CSTD) $(WARN) $(APP_OPT) -I$(INC_DIR) $(SDL_CFLAGS)
BENCH_CFLAGS ?= $(CSTD) $(WARN) $(BENCH_OPT) -DNDEBUG -I$(INC_DIR)

LDFLAGS ?=
APP_LDLIBS ?= $(SDL_LIBS)
BENCH_LDLIBS ?=

COMMON_SRCS := \
	$(SRC_DIR)/grid.c \
	$(SRC_DIR)/life.c \
	$(SRC_DIR)/io.c \
	$(SRC_DIR)/history.c

APP_SRCS := $(COMMON_SRCS) $(SRC_DIR)/ui_sdl.c $(SRC_DIR)/main.c
BENCH_SRCS := $(COMMON_SRCS) $(SRC_DIR)/bench_main.c

APP_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/app/%.o,$(APP_SRCS))
BENCH_OBJS := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/bench/%.o,$(BENCH_SRCS))

APP_DEPS := $(APP_OBJS:.o=.d)
BENCH_DEPS := $(BENCH_OBJS:.o=.d)

APP_TARGET := $(BIN_DIR)/$(PROJECT)
BENCH_TARGET := $(BIN_DIR)/$(BENCH)

.PHONY: all bench clean dirs run

all: dirs $(APP_TARGET)

bench: dirs $(BENCH_TARGET)

dirs:
	@mkdir -p "$(BUILD_DIR)/app" "$(BUILD_DIR)/bench" "$(BIN_DIR)"

$(APP_TARGET): $(APP_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(APP_LDLIBS)

$(BENCH_TARGET): $(BENCH_OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(BENCH_LDLIBS)

$(BUILD_DIR)/app/%.o: $(SRC_DIR)/%.c
	$(CC) $(APP_CFLAGS) -MMD -MP -c -o $@ $<

$(BUILD_DIR)/bench/%.o: $(SRC_DIR)/%.c
	$(CC) $(BENCH_CFLAGS) -MMD -MP -c -o $@ $<

-include $(APP_DEPS) $(BENCH_DEPS)

run: all
	./$(APP_TARGET)

clean:
	@rm -rf "$(BUILD_DIR)" "$(BIN_DIR)"
