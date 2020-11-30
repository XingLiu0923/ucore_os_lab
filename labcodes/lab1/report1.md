# report 1

## 练习1

1. 操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)

```makefile
# 定义变量，注意space的定义方法，makefile会自动忽略句首的空格
PROJ	:= challenge
EMPTY	:=
SPACE	:= $(EMPTY) $(EMPTY)
SLASH	:= /

V       := @
#need llvm/cang-3.5+
#USELLVM := 1
# try to infer the correct GCCPREFX
ifndef GCCPREFIX
GCCPREFIX := $(shell \
	# 0->stdin 1->stdout 2->stderr，而/dev/null相当于垃圾桶，信息重定向到这里直接丢弃
	# 看命令'i386-elf-objdump -i'是否存在，存在则输出information(-i)，进行了若干重定向
	# 最后赋予GCCPREFIX的值是then后echo出来的值
	# GCCPREFIX=''
	if i386-elf-objdump -i 2>&1 | grep '^elf32-i386$$' >/dev/null 2>&1; \
	then echo 'i386-elf-'; \
	# 重新尝试命令'objdump'
	elif objdump -i 2>&1 | grep 'elf32-i386' >/dev/null 2>&1; \
	then echo ''; \
	else echo "***" 1>&2; \
	echo "*** Error: Couldn't find an i386-elf version of GCC/binutils." 1>&2; \
	echo "*** Is the directory with i386-elf-gcc in your PATH?" 1>&2; \
	echo "*** If your i386-elf toolchain is installed with a command" 1>&2; \
	echo "*** prefix other than 'i386-elf-', set your GCCPREFIX" 1>&2; \
	echo "*** environment variable to that prefix and run 'make' again." 1>&2; \
	echo "*** To turn off this error, run 'gmake GCCPREFIX= ...'." 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# try to infer the correct QEMU
ifndef QEMU
QEMU := $(shell if which qemu-system-i386 > /dev/null; \
	then echo 'qemu-system-i386'; exit; \
	elif which i386-elf-qemu > /dev/null; \
	then echo 'i386-elf-qemu'; exit; \
	elif which qemu > /dev/null; \
	then echo 'qemu'; exit; \
	else \
	echo "***" 1>&2; \
	echo "*** Error: Couldn't find a working QEMU executable." 1>&2; \
	echo "*** Is the directory containing the qemu binary in your PATH" 1>&2; \
	echo "***" 1>&2; exit 1; fi)
endif

# eliminate default suffix rules
.SUFFIXES: .c .S .h

# delete target files if there is an error (or make is interrupted)
.DELETE_ON_ERROR:

# define compiler and flags
ifndef  USELLVM
# 用gcc编译
HOSTCC		:= gcc
# -g调试时保留代码文字信息，便于调试；-Wall生成所有警告信息；-O2优化生成的代码
HOSTCFLAGS	:= -g -Wall -O2
CC		:= $(GCCPREFIX)gcc
# -march说明编译后的代码在x86的i686结构上使用; -fno-builtin不适用编译器优化后的库函数（即需要手动include）;-fno-PIC: https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html; -ggdb与-g类似，-g生成debug原始信息，-ggdb生成信息更易于gdb适用; -m32表示编译的是32位程序; -stabs此选项以stabs格式声称调试信息，但是不包括gdb调试信息; -nostdinc不在标准系统目录中搜索头文件,只在-I指定的目录中搜索; $(DEFS)未定义，用于扩展变量
CFLAGS	:= -march=i686 -fno-builtin -fno-PIC -Wall -ggdb -m32 -gstabs -nostdinc $(DEFS)
# $(shell)可以输出shell指令，-fno-stack-protector禁用堆栈保护，-E仅预处理不进行编译汇编链接可以提高速度，-x c指明c语言
# /dev/null指定目标文件，>/dev/null 2>&1标准错误重定向到标准输出，&&先运行前一句若成功再运行后一句（原来如此。。）
# 意为只预处理，所有出错全部作为垃圾(/dev/null类似垃圾文件)测试能否开启-fno-stack-protector，若能则CFLAGS += -fno-stack-protector
# CFLAGS = -march=i686 -fno-builtin -fno-PIC -Wall -ggdb -m32 -gstabs -nostdinc -fno-stack-protector
CFLAGS	+= $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
# 适用clang做上述类似的处理
else
HOSTCC		:= clang
HOSTCFLAGS	:= -g -Wall -O2
CC		:= clang
CFLAGS	:= -march=i686 -fno-builtin -fno-PIC -Wall -g -m32 -nostdinc $(DEFS)
CFLAGS	+= $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
endif

# 源文件类型
CTYPE	:= c S

# ld命令，GNU链接器，与gcc链接是一样的
LD      := $(GCCPREFIX)ld
# ld -V 输出链接器支持的版本
LDFLAGS	:= -m $(shell $(LD) -V | grep elf_i386 2>/dev/null | head -n 1)
##-nostdlib不连接系统标准库文件
# LDFLAGS = -m elf_i386 -nostdlib
LDFLAGS	+= -nostdlib

OBJCOPY := $(GCCPREFIX)objcopy
OBJDUMP := $(GCCPREFIX)objdump

COPY	:= cp
MKDIR   := mkdir -p
MV		:= mv
RM		:= rm -f
AWK		:= awk
SED		:= sed
SH		:= sh
TR		:= tr
TOUCH	:= touch -c

OBJDIR	:= obj
BINDIR	:= bin

ALLOBJS	:=
ALLDEPS	:=
TARGETS	:=

# function.mk中定义了有用的makefile函数
include tools/function.mk

# 列出目录下所有的c S文件
# listf函数利用了filter来过滤出指定目录下的特定后缀文件
listf_cc = $(call listf,$(1),$(CTYPE))

# for cc

add_files_cc = $(call add_files,$(1),$(CC),$(CFLAGS) $(3),$(2),$(4))
create_target_cc = $(call create_target,$(1),$(2),$(3),$(CC),$(CFLAGS))

# for hostcc
add_files_host = $(call add_files,$(1),$(HOSTCC),$(HOSTCFLAGS),$(2),$(3))
create_target_host = $(call create_target,$(1),$(2),$(3),$(HOSTCC),$(HOSTCFLAGS))

cgtype = $(patsubst %.$(2),%.$(3),$(1))
objfile = $(call toobj,$(1))
asmfile = $(call cgtype,$(call toobj,$(1)),o,asm)
outfile = $(call cgtype,$(call toobj,$(1)),o,out)
symfile = $(call cgtype,$(call toobj,$(1)),o,sym)

# for match pattern
match = $(shell echo $(2) | $(AWK) '{for(i=1;i<=NF;i++){if(match("$(1)","^"$$(i)"$$")){exit 1;}}}'; echo $$?)

# >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
# include kernel/user

INCLUDE	+= libs/

CFLAGS	+= $(addprefix -I,$(INCLUDE))

LIBDIR	+= libs

$(call add_files_cc,$(call listf_cc,$(LIBDIR)),libs,)

# -------------------------------------------------------------------
# kernel

KINCLUDE	+= kern/debug/ \
			   kern/driver/ \
			   kern/trap/ \
			   kern/mm/

KSRCDIR		+= kern/init \
			   kern/libs \
			   kern/debug \
			   kern/driver \
			   kern/trap \
			   kern/mm

KCFLAGS		+= $(addprefix -I,$(KINCLUDE))

$(call add_files_cc,$(call listf_cc,$(KSRCDIR)),kernel,$(KCFLAGS))

KOBJS	= $(call read_packet,kernel libs)

# create kernel target
kernel = $(call totarget,kernel)

$(kernel): tools/kernel.ld

$(kernel): $(KOBJS)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -T tools/kernel.ld -o $@ $(KOBJS)
	@$(OBJDUMP) -S $@ > $(call asmfile,kernel)
	@$(OBJDUMP) -t $@ | $(SED) '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $(call symfile,kernel)

$(call create_target,kernel)

# -------------------------------------------------------------------

# create bootblock
bootfiles = $(call listf_cc,boot)
$(foreach f,$(bootfiles),$(call cc_compile,$(f),$(CC),$(CFLAGS) -Os -nostdinc))

bootblock = $(call totarget,bootblock)

$(bootblock): $(call toobj,$(bootfiles)) | $(call totarget,sign)
	@echo + ld $@
	$(V)$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 $^ -o $(call toobj,bootblock)
	@$(OBJDUMP) -S $(call objfile,bootblock) > $(call asmfile,bootblock)
	@$(OBJCOPY) -S -O binary $(call objfile,bootblock) $(call outfile,bootblock)
	@$(call totarget,sign) $(call outfile,bootblock) $(bootblock)

$(call create_target,bootblock)

# -------------------------------------------------------------------

# create 'sign' tools
$(call add_files_host,tools/sign.c,sign,sign)
$(call create_target_host,sign,sign)

# -------------------------------------------------------------------

# create ucore.img
UCOREIMG	:= $(call totarget,ucore.img)

$(UCOREIMG): $(kernel) $(bootblock)
	$(V)dd if=/dev/zero of=$@ count=10000
	$(V)dd if=$(bootblock) of=$@ conv=notrunc
	$(V)dd if=$(kernel) of=$@ seek=1 conv=notrunc

$(call create_target,ucore.img)

# >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

$(call finish_all)

IGNORE_ALLDEPS	= clean \
				  dist-clean \
				  grade \
				  touch \
				  print-.+ \
				  handin

ifeq ($(call match,$(MAKECMDGOALS),$(IGNORE_ALLDEPS)),0)
-include $(ALLDEPS)
endif

# files for grade script

TARGETS: $(TARGETS)

.DEFAULT_GOAL := TARGETS

.PHONY: qemu qemu-nox debug debug-nox
qemu-mon: $(UCOREIMG)
	$(V)$(QEMU)  -no-reboot -monitor stdio -hda $< -serial null
qemu: $(UCOREIMG)
	$(V)$(QEMU) -no-reboot -parallel stdio -hda $< -serial null
log: $(UCOREIMG)
	$(V)$(QEMU) -no-reboot -d int,cpu_reset  -D q.log -parallel stdio -hda $< -serial null
qemu-nox: $(UCOREIMG)
	$(V)$(QEMU)   -no-reboot -serial mon:stdio -hda $< -nographic
TERMINAL        :=gnome-terminal
debug: $(UCOREIMG)
	$(V)$(QEMU) -S -s -parallel stdio -hda $< -serial null &
	$(V)sleep 2
	$(V)$(TERMINAL) -e "gdb -q -tui -x tools/gdbinit"
	
debug-nox: $(UCOREIMG)
	$(V)$(QEMU) -S -s -serial mon:stdio -hda $< -nographic &
	$(V)sleep 2
	$(V)$(TERMINAL) -e "gdb -q -x tools/gdbinit"

.PHONY: grade touch

GRADE_GDB_IN	:= .gdb.in
GRADE_QEMU_OUT	:= .qemu.out
HANDIN			:= proj$(PROJ)-handin.tar.gz

TOUCH_FILES		:= kern/trap/trap.c

MAKEOPTS		:= --quiet --no-print-directory

grade:
	$(V)$(MAKE) $(MAKEOPTS) clean
	$(V)$(SH) tools/grade.sh

touch:
	$(V)$(foreach f,$(TOUCH_FILES),$(TOUCH) $(f))

print-%:
	@echo $($(shell echo $(patsubst print-%,%,$@) | $(TR) [a-z] [A-Z]))

.PHONY: clean dist-clean handin packall tags
clean:
	$(V)$(RM) $(GRADE_GDB_IN) $(GRADE_QEMU_OUT) cscope* tags
	-$(RM) -r $(OBJDIR) $(BINDIR)

dist-clean: clean
	-$(RM) $(HANDIN)

handin: packall
	@echo Please visit http://learn.tsinghua.edu.cn and upload $(HANDIN). Thanks!

packall: clean
	@$(RM) -f $(HANDIN)
	@tar -czf $(HANDIN) `find . -type f -o -type d | grep -v '^\.*$$' | grep -vF '$(HANDIN)'`

tags:
	@echo TAGS ALL
	$(V)rm -f cscope.files cscope.in.out cscope.out cscope.po.out tags
	$(V)find . -type f -name "*.[chS]" >cscope.files
	$(V)cscope -bq 
	$(V)ctags -L cscope.files

```

其中用到的重要的库函数`function.mk`注释如下

```makefile
OBJPREFIX	:= __objs_

.SECONDEXPANSION:
# -------------------- function begin --------------------

# list all files in some directories: (#directories, #types)
# 拆分后: fiter(%.c %.s, derectory/*)
listf = $(filter $(if $(2),$(addprefix %.,$(2)),%),\
		  $(wildcard $(addsuffix $(SLASH)*,$(1))))

# get .o obj files: (#files[, packet])
toobj = $(addprefix $(OBJDIR)$(SLASH)$(if $(2),$(2)$(SLASH)),\
		$(addsuffix .o,$(basename $(1))))

# get .d dependency files: (#files[, packet])
todep = $(patsubst %.o,%.d,$(call toobj,$(1),$(2)))

totarget = $(addprefix $(BINDIR)$(SLASH),$(1))

# change $(name) to $(OBJPREFIX)$(name): (#names)
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# cc compile template, generate rule for dep, obj: (file, cc[, flags, dir])
define cc_template
$$(call todep,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@$(2) -I$$(dir $(1)) $(3) -MM $$< -MT "$$(patsubst %.d,%.o,$$@) $$@"> $$@
$$(call toobj,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@echo + cc $$<
	$(V)$(2) -I$$(dir $(1)) $(3) -c $$< -o $$@
ALLOBJS += $$(call toobj,$(1),$(4))
endef

# compile file: (#files, cc[, flags, dir])
define do_cc_compile
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(4))))
endef

# add files to packet: (#files, cc[, flags, packet, dir])
define do_add_files_to_packet
__temp_packet__ := $(call packetname,$(4))
ifeq ($$(origin $$(__temp_packet__)),undefined)
$$(__temp_packet__) :=
endif
__temp_objs__ := $(call toobj,$(1),$(5))
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(5))))
$$(__temp_packet__) += $$(__temp_objs__)
endef

# add objs to packet: (#objs, packet)
define do_add_objs_to_packet
__temp_packet__ := $(call packetname,$(2))
ifeq ($$(origin $$(__temp_packet__)),undefined)
$$(__temp_packet__) :=
endif
$$(__temp_packet__) += $(1)
endef

# add packets and objs to target (target, #packes, #objs[, cc, flags])
define do_create_target
__temp_target__ = $(call totarget,$(1))
__temp_objs__ = $$(foreach p,$(call packetname,$(2)),$$($$(p))) $(3)
TARGETS += $$(__temp_target__)
ifneq ($(4),)
$$(__temp_target__): $$(__temp_objs__) | $$$$(dir $$$$@)
	$(V)$(4) $(5) $$^ -o $$@
else
$$(__temp_target__): $$(__temp_objs__) | $$$$(dir $$$$@)
endif
endef

# finish all
define do_finish_all
ALLDEPS = $$(ALLOBJS:.o=.d)
$$(sort $$(dir $$(ALLOBJS)) $(BINDIR)$(SLASH) $(OBJDIR)$(SLASH)):
	@$(MKDIR) $$@
endef

# --------------------  function end  --------------------
# compile file: (#files, cc[, flags, dir])
cc_compile = $(eval $(call do_cc_compile,$(1),$(2),$(3),$(4)))

# add files to packet: (#files, cc[, flags, packet, dir])
add_files = $(eval $(call do_add_files_to_packet,$(1),$(2),$(3),$(4),$(5)))

# add objs to packet: (#objs, packet)
add_objs = $(eval $(call do_add_objs_to_packet,$(1),$(2)))

# add packets and objs to target (target, #packes, #objs, cc, [, flags])
create_target = $(eval $(call do_create_target,$(1),$(2),$(3),$(4),$(5)))

read_packet = $(foreach p,$(call packetname,$(1)),$($(p)))

add_dependency = $(eval $(1): $(2))

finish_all = $(eval $(call do_finish_all))
```

