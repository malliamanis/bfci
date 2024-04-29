NC     = \033[0m
BLUE   = \033[1;34m
CYAN   = \033[1;36m
GREEN  = \033[1;32m
YELLOW = \033[1;33m

CC = clang
LD = clang

CFLAGS = -std=c2x -pedantic -Wall -Wextra -Wfloat-equal -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wunreachable-code -Isrc

CFLAGS_DEB = -O0 -ggdb -gdwarf-4
CFLAGS_REL = -O3

LDFLAGS =

rwildcard = $(foreach d, $(wildcard $1*), $(call rwildcard, $d/, $2) $(filter $(subst *, %, $2), $d))

BUILD_DIR = build

DEB_DIR = $(BUILD_DIR)/debug
REL_DIR = $(BUILD_DIR)/release

OBJ_DEB_DIR = $(DEB_DIR)/obj
OBJ_REL_DIR = $(REL_DIR)/obj
SRC         = $(call rwildcard, src, *.c)
OBJ_DEB     = $(patsubst src/%.c, $(OBJ_DEB_DIR)/%.o,   $(SRC))
OBJ_REL     = $(patsubst src/%.c, $(OBJ_REL_DIR)/%.o,   $(SRC))

EXE_DEB = $(DEB_DIR)/bfci
EXE_REL = $(REL_DIR)/bfci

.PHONY: debug release clean

debug: $(OBJ_DEB)
	@ echo -e "$(GREEN)LINKING DEBUG EXECUTABLE$(NC) $(EXE_DEB)"
	@ $(LD) $(OBJ_DEB) -o $(EXE_DEB) $(LDFLAGS)

release: $(OBJ_REL)
	@ echo -e "$(GREEN)LINKING RELEASE EXECUTABLE$(NC) $(EXE_REL)"
	@ $(LD) $(OBJ_REL) -o $(EXE_REL) $(LDFLAGS)

$(OBJ_DEB_DIR)/%.o: src/%.c
	@ mkdir -p $(@D)
	@ echo -e "$(GREEN)COMPILING OBJECT$(NC) $@"
	@ $(CC) $(CFLAGS) $(CFLAGS_DEB) -c $< -o $@

$(OBJ_REL_DIR)/%.o: src/%.c
	@ mkdir -p $(@D)
	@ echo -e "$(GREEN)COMPILING OBJECT$(NC) $@"
	@ $(CC) $(CFLAGS) $(CFLAGS_REL) -c $< -o $@

clean:
	@ echo -e "$(YELLOW)CLEANING PROJECT$(NC)"
	@ rm -rf $(BUILD_DIR)
