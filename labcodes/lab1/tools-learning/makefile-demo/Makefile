SLASH := /
OBJDIR := obj
BINDIR := bin
OBJPREFIX := __objs_

.SECONDEXPANSION:

include tools/function.mk

SRC := $(call listf,src,c)
TMP := tmp
OBJ := $(call toobj,$(SRC),$(TMP))
DEP := $(call todep,$(SRC),$(TMP))
ORDER := $(OBJDIR)$(SLASH)$(TMP)$(SLASH)src/

# $(eval $(call do_create_target,main,,obj/tmp/src/main.o obj/tmp/src/init.o obj/tmp/src/common.o,gcc,-g))
# $(info $(call do_add_files_to_packet,$(SRC),gcc,-g,HELLO,$(TMP)))

add_files_cc := $(call add_files,$(1),gcc,-g,$(3),$(2),$(4))
$(call add_files_cc,$(SRC),,)

all2:
	echo $(add_files_cc)

all: $(DEP) $(OBJ)
	@echo "        ALLOBJS: $(ALLOBJS)"
	@echo "__temp_packet__: $(__temp_packet__)"
	@echo "  __temp_objs__: $(__temp_objs__)"
	@echo "   __objs_HELLO: $(__objs_HELLO)"


$(ORDER) $(BINDIR)$(SLASH):
	mkdir -p $@


.PHONY: clean
clean:
	rm -rf obj bin
