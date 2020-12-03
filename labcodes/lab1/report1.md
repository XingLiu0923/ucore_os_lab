# report 1

## 练习1

1. 操作系统镜像文件ucore.img是如何一步一步生成的？(需要比较详细地解释Makefile中每一条相关命令和命令参数的含义，以及说明命令导致的结果)

参考：https://www.it610.com/article/1294049168892960768.htm

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
# -march说明编译后的代码在x86的i686结构上使用; -fno-builtin不适用编译器优化后的库函数（即需要手动include）,只接受以“__builtin_”开头的名称的内建函数; ;-fno-PIC: https://gcc.gnu.org/onlinedocs/gcc/Code-Gen-Options.html; -ggdb与-g类似，-g生成debug原始信息，-ggdb生成信息更易于gdb适用; -m32表示编译的是32位程序; -stabs此选项以stabs格式声称调试信息，但是不包括gdb调试信息; -nostdinc不在标准系统目录中搜索头文件,只在-I指定的目录中搜索; $(DEFS)未定义，用于扩展变量
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
# -nostdlib不连接系统标准库文件
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

# 列出目录下所有的.c .S文件
listf_cc = $(call listf,$(1),$(CTYPE))

# for cc
# 编译产生.o文件，add_files_cc相当于一个包装的新函数 (files, packet, flags, dir)
add_files_cc = $(call add_files,$(1),$(CC),$(CFLAGS) $(3),$(2),$(4))
# 链接产生可执行文件
create_target_cc = $(call create_target,$(1),$(2),$(3),$(CC),$(CFLAGS))

# for hostcc
add_files_host = $(call add_files,$(1),$(HOSTCC),$(HOSTCFLAGS),$(2),$(3))
create_target_host = $(call create_target,$(1),$(2),$(3),$(HOSTCC),$(HOSTCFLAGS))

# cgtype 将$(1)中的$(2)后缀替换成$(3)后缀
# patsubst用例，$(patsubst %.c,%.o, a.c b.c)
cgtype = $(patsubst %.$(2),%.$(3),$(1))
objfile = $(call toobj,$(1))
# 将.o后缀分别改为.asm .out .sym
asmfile = $(call cgtype,$(call toobj,$(1)),o,asm)
outfile = $(call cgtype,$(call toobj,$(1)),o,out)
symfile = $(call cgtype,$(call toobj,$(1)),o,sym)

# for match pattern
# awk tutorial: https://zhuanlan.zhihu.com/p/68188159
# NF是column#，match(string,regexp,array)
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

下面是用到的重要的库函数`function.mk`的相关理解，**其中：**

基本注释参考如下：https://blog.csdn.net/dglxlcl/article/details/100842011

makefile-demo来源如下：https://blog.csdn.net/Anhui_Chen/article/details/106892975

```makefile
OBJPREFIX	:= __objs_

.SECONDEXPANSION:
# -------------------- function begin --------------------

# list all files in some directories: (#directories, #types)
# 拆分后: fiter(%.c %.s, derectory/*)
listf = $(filter $(if $(2),$(addprefix %.,$(2)),%),\
		  $(wildcard $(addsuffix $(SLASH)*,$(1))))

# get .o obj files: (#files[, packet])
# 给出文件名列表files,和软件包名称packet，返回相应文件的目标文件名称：/objdir/packet/file.o
toobj = $(addprefix $(OBJDIR)$(SLASH)$(if $(2),$(2)$(SLASH)),\
		$(addsuffix .o,$(basename $(1))))

# get .d dependency files: (#files[, packet])
# .d文件是包含了依赖文件关系，具体描述：https://www.cnblogs.com/sky-heaven/p/9735177.html
todep = $(patsubst %.o,%.d,$(call toobj,$(1),$(2)))

totarget = $(addprefix $(BINDIR)$(SLASH),$(1))

# change $(name) to $(OBJPREFIX)$(name): (#names)
# packetname = __objs_$(1)
packetname = $(if $(1),$(addprefix $(OBJPREFIX),$(1)),$(OBJPREFIX))

# cc compile template, generate rule for dep, obj: (file, cc[, flags, dir])
# 为文件生成编译后的目标文件 dep和obj
define cc_template
# 生成目标文件的依赖文件，要被eval两次，最后此外还有make时的一次，因此会出现4次$
# "|"号表示 后面的依赖目标是order-only Prerequisites，执行某个或某些规则，但不会引起生成目标被重新生成
# $$$$(dir $$$$@)：是一个order-only Prerequisites,其内容是是%.d文件的路径
# -I：添加包含目录; -MM: 生成文件的依赖关系，但不包含标准库的头文件; -MT: 在生成的依赖文件中,指定依赖规则中的目标
# $<:第一个依赖文件。-MT "$$(patsubst %.d,%.o,$$@) $$@"：在规则中的目标文件添加%.o (这样的话，目标文件由%o %d组成)
# > $$@：将依赖规则信息输出到目标文件（%.d）中，其实也可以用-MF标记来做
# gcc -Idir -flags -MM $< -MT %.d %.o > %.d
$$(call todep,$(1),$(4)): $(1) | $$$$(dir $$$$@)
	@$(2) -I$$(dir $(1)) $(3) -MM $$< -MT "$$(patsubst %.d,%.o,$$@) $$@"> $$@
# 生成目标文件
# gcc -Idir -flags -c file -o file.o
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
# 此模板就是真正在makefile中用来编译所有的目表文件，并生成makefile规则的模板。
define do_add_files_to_packet
# __temp_packet__ = __objs_$(4)
__temp_packet__ := $(call packetname,$(4))
# 如果$$(__temp_packet__)的值未定义，则定义为空
ifeq ($$(origin $$(__temp_packet__)),undefined)
$$(__temp_packet__) :=
endif
# __temp_objs__=objdir/dir/file.o
__temp_objs__ := $(call toobj,$(1),$(5))
# 对于files中的各个file进行编译
$$(foreach f,$(1),$$(eval $$(call cc_template,$$(f),$(2),$(3),$(5))))
## 此时可以获得最终的$$(__temp_packet__)变量，为各个.o函数的集合，在 do_create_target 函数中会用到
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

# add packets and objs to target (target, #packets, #objs[, cc, flags])
# (sign, sign, , gcc, -g -Wall -O2)
define do_create_target
# __temp_target__ = /bin/$(target)
# __temp_target__ = /bin/bootblock
__temp_target__ = $(call totarget,$(1))
# 读取__objs_$(2)中的文件序列，再在最后加上$(3)返回给__temp_objs__(为什么要加$(3)?，实际上(3)为空白)
# $(call packetname,$(2)) = __objs_sign,
# __temp_objs__ = /objs/sign/tools/sign.o
# 结合do_add_files_to_packet看，本质上是提取了已经编译后的.o文件
__temp_objs__ = $$(foreach p,$(call packetname,$(2)),$$($$(p))) $(3)
# TARGETS = bin/kernel bin/bootblock
TARGETS += $$(__temp_target__)
ifneq ($(4),)
# gcc -g -Wall -O2 obj/sign/tools/sign.o -o bin/sign
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

