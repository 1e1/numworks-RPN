# Self-contained build for the NumWorks RPN external app.
#
#   make            build the device app  -> output/rpn.nwa
#   make test       build & run the host engine tests
#   make install    build and upload to a plugged-in calculator
#   make clean
#
# Requirements: arm-none-eabi-gcc (device build) and node/npx (nwlink).

APP_NAME = rpn
APP_ICON = src/icon.png
OUTPUT_DIR = output

NWLINK = npx --yes -- nwlink

CC = arm-none-eabi-gcc
CXX = arm-none-eabi-g++

SOURCES = $(wildcard src/*.cpp)
OBJS = $(patsubst src/%.cpp,$(OUTPUT_DIR)/%.o,$(SOURCES))

# Flags provided by nwlink for the calculator target (Cortex-M7 + EADK headers).
EADK_CFLAGS := $(shell $(NWLINK) eadk-cflags-device)
EADK_LDFLAGS := $(shell $(NWLINK) eadk-ldflags-device)

CXXFLAGS = $(EADK_CFLAGS) -std=c++11 -fno-exceptions -Wall -Os -MMD -MP
CXXFLAGS += -fdata-sections -ffunction-sections
CXXFLAGS += -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal

LDFLAGS = $(EADK_LDFLAGS) --specs=nano.specs
LDFLAGS += -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
LDFLAGS += -Wl,--gc-sections -flinker-output=nolto-rel
# Pull float support into newlib-nano's printf so decimals render (%g).
LDFLAGS += -Wl,-u,_printf_float

.PHONY: all
all: $(OUTPUT_DIR)/$(APP_NAME).nwa

$(OUTPUT_DIR)/$(APP_NAME).nwa: $(OBJS) $(OUTPUT_DIR)/icon.o
	@echo "LD      $@"
	$(CC) $(CXXFLAGS) $(LDFLAGS) $^ -o $@

$(OUTPUT_DIR)/%.o: src/%.cpp | $(OUTPUT_DIR)
	@echo "CXX     $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OUTPUT_DIR)/icon.o: $(APP_ICON) | $(OUTPUT_DIR)
	@echo "ICON    $<"
	$(NWLINK) png-icon-o $< $@

$(OUTPUT_DIR):
	mkdir -p $(OUTPUT_DIR)

.PHONY: install
install: $(OUTPUT_DIR)/$(APP_NAME).nwa
	$(NWLINK) install-nwa $<

.PHONY: test
test:
	$(MAKE) -C tests run

.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR)
	$(MAKE) -C tests clean

-include $(OBJS:.o=.d)
