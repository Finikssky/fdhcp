#include "libtimer/timer.h"
#include <unistd.h>

void * timer(void * arg)
{
	struct timer t = *((struct timer *)arg);

	while(1)
	{
		usleep(1000000 * t.timeout);
		t.fun(t.args);
	}
}



