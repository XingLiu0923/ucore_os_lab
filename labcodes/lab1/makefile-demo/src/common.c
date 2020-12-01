#include <stdio.h>
#include "common.h"

void prinfo(const char *s)
{
	if (!s) return ;
	printf(INFO "%s\n", s);
}
