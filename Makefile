# The sensorfw sources built with GNU Make.   make [PLATFORM=host|m4] [V=1] [all|test|size|run|clean]
PLATFORM ?= host
BUILD    ?= build/make-$(PLATFORM)
VERSION  := 0.1.0

ifeq ($(PLATFORM),m4)
CC        := arm-none-eabi-gcc
AR        := arm-none-eabi-ar
SIZE      := arm-none-eabi-size
CPU       := -mcpu=cortex-m4 -mthumb -mfloat-abi=soft
CFLAGS    := $(CPU) -ffunction-sections -fdata-sections -ffile-prefix-map=$(CURDIR)=. -Os -DNDEBUG
LDFLAGS   := $(CPU) -Wl,--gc-sections --specs=nano.specs -nostartfiles -Tplatform/arm-m4/link.ld
PLAT_SRC  := platform/arm-m4/platform_semihost.c
STARTUP   := platform/arm-m4/startup.c
RUN       := qemu-system-arm -machine mps2-an386 -nographic -semihosting-config enable=on,target=native -kernel
else ifeq ($(PLATFORM),host)
CC        := cc
AR        := ar
CFLAGS    := -g
PLAT_SRC  := platform/host/platform_host.c
RUN       :=
else
$(error PLATFORM must be host or m4, not "$(PLATFORM)")
endif

WARN     := -Wall -Wextra -Wshadow -Wconversion
CPPFLAGS := -DRINGBUF_CAPACITY=16u -Ilib/include -Ilib/filters/include -Iplatform/include
DEPFLAGS := -MMD -MP

ifeq ($(V),1)
Q    :=
ECHO := @:
else
Q    := @
ECHO := @echo
endif

LIB_SRC := lib/src/ringbuf.c lib/src/stats.c lib/filters/src/median.c
LIB_OBJ := $(LIB_SRC:%.c=$(BUILD)/obj/%.o) $(BUILD)/version.o
PLAT_OBJ := $(PLAT_SRC:%.c=$(BUILD)/obj/%.o)
START_OBJ := $(STARTUP:%.c=$(BUILD)/obj/%.o)
OBJS := $(LIB_OBJ) $(PLAT_OBJ) $(START_OBJ) $(BUILD)/obj/app/main.o $(BUILD)/obj/test/test_sensor.o

.PHONY: all test size run clean FORCE
all: $(BUILD)/sensor_demo $(BUILD)/test_sensor

# Compile. The stamp makes every object depend on the flags, so a changed flag rebuilds.
$(BUILD)/obj/%.o: %.c $(BUILD)/flags.txt
	@mkdir -p $(dir $@)
	$(ECHO) "  CC    $<"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(OBJ_FLAGS) $(WARN) $(DEPFLAGS) -c $< -o $@

# A target specific variable: only the startup object gets this option (see platform/CMakeLists.txt).
$(START_OBJ): OBJ_FLAGS := -fno-tree-loop-distribute-patterns

$(BUILD)/version.o: $(BUILD)/version.c $(BUILD)/flags.txt
	$(ECHO) "  CC    $<"
	$(Q)$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(WARN) $(DEPFLAGS) -c $< -o $@

# FORCE makes these two rules run on every build. Each one writes its file only when the content changes.
$(BUILD)/flags.txt: FORCE
	@mkdir -p $(BUILD)
	$(Q)echo '$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(WARN)' | cmp -s - $@ || echo '$(CC) $(CPPFLAGS) $(CFLAGS) $(EXTRA_CFLAGS) $(WARN)' > $@

$(BUILD)/version.c: lib/src/version.c.in FORCE
	@mkdir -p $(BUILD)
	$(ECHO) "  GEN   $@"
	$(Q)d=$$(git describe --tags --always --dirty 2>/dev/null || echo no-git); \
	    sed -e 's/@PROJECT_VERSION@/$(VERSION)/' -e "s/@GIT_DESCRIBE@/$$d/" $< > $@.tmp; \
	    cmp -s $@.tmp $@ && rm $@.tmp || mv $@.tmp $@

# Archives, then programs. The archives come after the objects that use them.
$(BUILD)/libsensor.a: $(LIB_OBJ)
	$(ECHO) "  AR    $@"
	$(Q)rm -f $@ && $(AR) rcs $@ $^

$(BUILD)/libplatform.a: $(PLAT_OBJ)
	$(ECHO) "  AR    $@"
	$(Q)rm -f $@ && $(AR) rcs $@ $^

$(BUILD)/sensor_demo: $(START_OBJ) $(BUILD)/obj/app/main.o $(BUILD)/libsensor.a $(BUILD)/libplatform.a
	$(ECHO) "  LD    $@"
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

$(BUILD)/test_sensor: $(START_OBJ) $(BUILD)/obj/test/test_sensor.o $(BUILD)/libsensor.a $(BUILD)/libplatform.a
	$(ECHO) "  LD    $@"
	$(Q)$(CC) $(LDFLAGS) $^ -o $@

test: all
	$(Q)$(RUN) $(BUILD)/test_sensor 2>&1
	$(Q)$(RUN) $(BUILD)/sensor_demo 2>&1 | grep -q "mean ok" && echo "sensor_demo: mean ok"

size: all
	$(Q)$(SIZE) $(BUILD)/sensor_demo $(BUILD)/test_sensor

run: $(BUILD)/sensor_demo
	$(Q)$(RUN) $(BUILD)/sensor_demo 2>&1

clean:
	$(Q)rm -rf $(BUILD)

-include $(OBJS:.o=.d)
