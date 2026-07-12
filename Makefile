# Build the NumWorks RPN external app.
#
#   make                     device app   -> output/rpn.nwa   (arm-none-eabi)
#   make PLATFORM=web        web app      -> output/rpn.nwb   (emscripten)
#   make install             upload to a plugged-in calculator
#   make test                build & run the host engine tests
#   make clean
#
# Requirements: node/npx (nwlink); arm-none-eabi-gcc for device; emcc for web.

APP_NAME = rpn
APP_ICON = src/icon.png
OUTPUT_DIR = output
PLATFORM ?= device

NWLINK = npx --yes -- nwlink

SOURCES = $(wildcard src/*.cpp)
OBJDIR = $(OUTPUT_DIR)/$(PLATFORM)
OBJS = $(patsubst src/%.cpp,$(OBJDIR)/%.o,$(SOURCES))

ifeq ($(PLATFORM),device)
  CC = arm-none-eabi-gcc
  CXX = arm-none-eabi-g++
  STRIP = arm-none-eabi-strip
  EADK_CFLAGS := $(shell $(NWLINK) eadk-cflags-device)
  CXXFLAGS = $(EADK_CFLAGS) -std=c++11 -fno-exceptions -Wall -Os -MMD -MP
  CXXFLAGS += -fdata-sections -ffunction-sections
  CXXFLAGS += -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal
  LDFLAGS = $(shell $(NWLINK) eadk-ldflags-device) --specs=nano.specs
  LDFLAGS += -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level
  LDFLAGS += -Wl,--gc-sections -flinker-output=nolto-rel -Wl,-u,_printf_float
  APP = $(OUTPUT_DIR)/$(APP_NAME).nwa
  # The PNG icon object is an ARM ELF; only the device app links it.
  ICON_OBJ = $(OBJDIR)/icon.o
else ifeq ($(PLATFORM),web)
  CC = emcc
  CXX = em++
  EADK_CFLAGS := $(shell $(NWLINK) eadk-cflags-web)
  CXXFLAGS = $(EADK_CFLAGS) -std=c++11 -fno-exceptions -Wall -O0 -g -MMD -MP
  LDFLAGS = -sSIDE_MODULE=2 -sEXPORTED_FUNCTIONS=_main -sASYNCIFY=1
  LDFLAGS += -sASYNCIFY_IMPORTS=eadk_event_get,_eadk_keyboard_scan_do_scan,eadk_timing_msleep,eadk_display_wait_for_vblank
  LDFLAGS += -lc
  APP = $(OUTPUT_DIR)/$(APP_NAME).nwb
  ICON_OBJ =
else
  $(error Unknown PLATFORM '$(PLATFORM)' (use device or web))
endif

.PHONY: all
all: $(APP)

$(APP): $(OBJS) $(ICON_OBJ)
	@echo "LD      $@"
	$(CC) $(CXXFLAGS) $(LDFLAGS) $^ -o $@
ifeq ($(PLATFORM),device)
# Drop DWARF/.comment only; keep .symtab + .rodata.eadk_* so nwlink can still
# relink the relocatable .nwa at install time. A full strip would break install.
	@echo "STRIP   $@"
	$(STRIP) --strip-debug --remove-section=.comment $@
endif

$(OBJDIR)/%.o: src/%.cpp | $(OBJDIR)
	@echo "CXX     $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(OBJDIR)/icon.o: $(APP_ICON) | $(OBJDIR)
	@echo "ICON    $<"
	$(NWLINK) png-icon-o $< $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

.PHONY: install
install: $(OUTPUT_DIR)/$(APP_NAME).nwa
	$(NWLINK) install-nwa $<

.PHONY: test
test:
	$(MAKE) -C tests run

# Render the app in the Epsilon Linux simulator -> capture/render.png (Docker).
.PHONY: screenshot
screenshot:
	docker/screenshot.sh

.PHONY: clean
clean:
	rm -rf $(OUTPUT_DIR)
	$(MAKE) -C tests clean

-include $(OBJS:.o=.d)
