#include "init.h"
#include "common.h"

static void mm_init(void)
{
	prinfo("memory init...");
}

static void rest_init(void)
{
	prinfo("rest init...");
}

int start_kernel(void)
{
	prinfo("start kernel...");
	mm_init();
	rest_init();
}

