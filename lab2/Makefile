CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -Wpedantic
BUILD = build
SRC = src/revert_string

.PHONY: all check clean
all: $(BUILD)/swap $(BUILD)/revert_direct $(BUILD)/revert_static $(BUILD)/revert_dynamic $(BUILD)/tests

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/swap: src/swap/main.c src/swap/swap.c src/swap/swap.h | $(BUILD)
	$(CC) $(CFLAGS) src/swap/main.c src/swap/swap.c -o $@

$(BUILD)/revert_direct: $(SRC)/main.c $(SRC)/revert_string.c $(SRC)/revert_string.h | $(BUILD)
	$(CC) $(CFLAGS) $(SRC)/main.c $(SRC)/revert_string.c -o $@

$(BUILD)/revert_string.o: $(SRC)/revert_string.c $(SRC)/revert_string.h | $(BUILD)
	$(CC) $(CFLAGS) -c $(SRC)/revert_string.c -o $@

$(BUILD)/librevert.a: $(BUILD)/revert_string.o
	ar rcs $@ $<

$(BUILD)/revert_static: $(SRC)/main.c $(SRC)/revert_string.h $(BUILD)/librevert.a
	$(CC) $(CFLAGS) -I$(SRC) $(SRC)/main.c $(BUILD)/librevert.a -o $@

$(BUILD)/revert_string.pic.o: $(SRC)/revert_string.c $(SRC)/revert_string.h | $(BUILD)
	$(CC) $(CFLAGS) -fPIC -c $(SRC)/revert_string.c -o $@

$(BUILD)/librevert.so: $(BUILD)/revert_string.pic.o
	$(CC) -shared $< -o $@

$(BUILD)/revert_dynamic: $(SRC)/main.c $(SRC)/revert_string.h $(BUILD)/librevert.so
	$(CC) $(CFLAGS) -I$(SRC) $(SRC)/main.c -L$(BUILD) -lrevert -o $@

$(BUILD)/tests: src/tests/tests.c $(SRC)/revert_string.h $(BUILD)/librevert.so
	$(CC) $(CFLAGS) -I$(SRC) src/tests/tests.c -L$(BUILD) -lrevert -lcunit -o $@

check: all
	bash verify.sh

clean:
	rm -rf $(BUILD)
