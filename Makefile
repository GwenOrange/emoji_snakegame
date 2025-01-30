# Snake Notcurses Makefile
SNAKE_VERSION = 1.00
MAJOR_VERSION = 1
MINOR_VERSION = 0
PATCH_VERSION = 0

SRC_DIR = src
BUILD_DIR = build
SOUNDS_SRC_DIR = $(SRC_DIR)/snake_sounds
HEADER_FILE = $(SRC_DIR)/notcurses-snake.h

LIB_NAME = notcurses-snake
STATIC_LIB = lib$(LIB_NAME).a
DYNAMIC_LIB = lib$(LIB_NAME).so.$(MAJOR_VERSION).$(MINOR_VERSION).$(PATCH_VERSION)

PACKAGE_MANAGER := $(shell which dnf 2>/dev/null || which apt-get 2>/dev/null || echo "unknown")

ifeq ($(findstring apt-get,$(PACKAGE_MANAGER)),apt-get)
  PREFIX = /usr
  LIB_INSTALL_PATH = $(PREFIX)/lib/x86_64-linux-gnu
else
  PREFIX = /usr/local
  LIB_INSTALL_PATH = $(PREFIX)/lib64
endif

HEADER_INSTALL_PATH = $(PREFIX)/include
SOUNDS_INSTALL_PATH = $(PREFIX)/share/notcurses-snake-sounds
LD_CONF_DIR = /etc/ld.so.conf.d
LD_CONF_FILE = $(LD_CONF_DIR)/notcurses-snake.conf

CC = gcc
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -g \
         -D_XOPEN_SOURCE=600 -D_GNU_SOURCE \
         -O3 -ffast-math -finline-functions -funroll-loops \
         -mtune=native -ftree-vectorize -fPIC \
         -fopenmp -flto -fvisibility=default \
         -DSNAKE_VERSION=$(SNAKE_VERSION)f

LDFLAGS_BASE = -lnotcurses -lnotcurses-core -ldeflate -lunistring \
               -lgpm -lpthread -lm -lavcodec -lavformat -lavutil \
               -lswscale -lavdevice -lSDL2 -lSDL2_mixer

ifeq ($(findstring apt-get,$(PACKAGE_MANAGER)),apt-get)
  LDFLAGS = $(LDFLAGS_BASE) -lncursesw -ltinfo
else
  LDFLAGS = $(LDFLAGS_BASE) -lncursesw
endif

LIB_SOURCES = $(filter-out $(SRC_DIR)/main.c, $(wildcard $(SRC_DIR)/*.c))
OBJECTS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/obj/%.o,$(LIB_SOURCES))

.PHONY: all
all: directories static dynamic snakegame

.PHONY: directories
directories:
	@mkdir -p $(BUILD_DIR)/obj $(BUILD_DIR)/lib $(BUILD_DIR)/bin

$(BUILD_DIR)/obj/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -I$(SRC_DIR) -c $< -o $@

.PHONY: static
static: $(BUILD_DIR)/lib/$(STATIC_LIB)

$(BUILD_DIR)/lib/$(STATIC_LIB): $(OBJECTS)
	ar rcs --plugin=$(shell gcc --print-file-name=liblto_plugin.so) $@ $^
	ranlib $@

.PHONY: dynamic
dynamic: $(BUILD_DIR)/lib/$(DYNAMIC_LIB)

$(BUILD_DIR)/lib/$(DYNAMIC_LIB): $(OBJECTS)
	$(CC) -shared $(CFLAGS) -Wl,-soname,lib$(LIB_NAME).so.$(MAJOR_VERSION) \
	-o $@ $^ $(LDFLAGS) -Wl,--no-undefined -Wl,--as-needed -Wl,-rpath=$(LIB_INSTALL_PATH)
	ln -sf $(DYNAMIC_LIB) $(BUILD_DIR)/lib/lib$(LIB_NAME).so.$(MAJOR_VERSION)
	ln -sf lib$(LIB_NAME).so.$(MAJOR_VERSION) $(BUILD_DIR)/lib/lib$(LIB_NAME).so

# 关键修复：调整链接顺序和参数
snakegame: $(SRC_DIR)/main.c static
	$(CC) $(CFLAGS) -I$(SRC_DIR) -L$(BUILD_DIR)/lib \
	$< \
	-Wl,--whole-archive -l:$(STATIC_LIB) -Wl,--no-whole-archive \
	$(LDFLAGS) \
	-o $(BUILD_DIR)/bin/emoji_snakegame

.PHONY: check-ld-config
check-ld-config:
	@if [ ! -f $(LD_CONF_FILE) ]; then \
		echo "$(LIB_INSTALL_PATH)" > $(LD_CONF_FILE); \
		ldconfig; \
	else \
		grep -q "^$(LIB_INSTALL_PATH)$$" $(LD_CONF_FILE) || { \
			echo "$(LIB_INSTALL_PATH)" >> $(LD_CONF_FILE); \
			ldconfig; \
		}; \
	fi

.PHONY: install
install: static dynamic snakegame
	install -d $(LIB_INSTALL_PATH) $(HEADER_INSTALL_PATH) $(SOUNDS_INSTALL_PATH)
	install -m 644 $(BUILD_DIR)/lib/$(STATIC_LIB) $(LIB_INSTALL_PATH)/$(STATIC_LIB)
	install -m 755 $(BUILD_DIR)/lib/$(DYNAMIC_LIB) $(LIB_INSTALL_PATH)
	cd $(LIB_INSTALL_PATH) && ln -sf $(DYNAMIC_LIB) lib$(LIB_NAME).so.$(MAJOR_VERSION) \
	&& ln -sf lib$(LIB_NAME).so.$(MAJOR_VERSION) lib$(LIB_NAME).so
	install -m 644 $(HEADER_FILE) $(HEADER_INSTALL_PATH)
	install -m 644 $(SOUNDS_SRC_DIR)/* $(SOUNDS_INSTALL_PATH)
	install -m 755 $(BUILD_DIR)/bin/emoji_snakegame /usr/bin/emoji_snakegame
	@if [ $$(id -u) -eq 0 ]; then $(MAKE) check-ld-config; fi

.PHONY: remove-ld-config
remove-ld-config:
	@rm -f $(LD_CONF_FILE) 2>/dev/null && ldconfig || true

.PHONY: uninstall
uninstall:
	rm -f $(LIB_INSTALL_PATH)/$(STATIC_LIB) \
	$(LIB_INSTALL_PATH)/lib$(LIB_NAME).so* \
	$(LIB_INSTALL_PATH)/$(DYNAMIC_LIB)
	rm -f $(HEADER_INSTALL_PATH)/$(notdir $(HEADER_FILE))
	rm -rf $(SOUNDS_INSTALL_PATH)
	rm -f /usr/bin/emoji_snakegame
	@if [ $$(id -u) -eq 0 ]; then $(MAKE) remove-ld-config; fi

.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)
	find . -name "*.o" -delete

.PHONY: debug
debug: CFLAGS += -DDEBUG -g
debug: all

.PHONY: check-install
check-install:
	@ls -l $(LIB_INSTALL_PATH)/lib$(LIB_NAME).* || true
	@ldconfig -p | grep $(LIB_NAME) || true
	@[ -f $(LD_CONF_FILE) ] && cat $(LD_CONF_FILE) || echo "No linker config"