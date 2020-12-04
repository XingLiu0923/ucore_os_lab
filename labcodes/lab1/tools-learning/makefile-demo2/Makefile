define func1
a.o: a.c
	gcc -c a.c -o a.o 
endef

func2=$(eval $(call func1))

$(call func2)

all:
	echo hello

.PHONY = clean
clean:
	rm -f *.o
