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

$(eval $(call do_create_target,main,,obj/tmp/src/main.o obj/tmp/src/init.o obj/tmp/src/common.o,gcc,-g))
all: $(DEP) $(OBJ)
	@echo "        ALLOBJS: $(ALLOBJS)"
	@echo "__temp_packet__: $(__temp_packet__)"
	@echo "  __temp_objs__: $(__temp_objs__)"
	@echo "   __objs_HELLO: $(__objs_HELLO)"
$(eval $(call do_add_files_to_packet,$(SRC),gcc,-g,HELLO,$(TMP)))


$(ORDER):
	mkdir -p $@

.PHONY: clean
clean:
	rm -rf obj bin/*
