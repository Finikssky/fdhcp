#include "timer.h"

void * timer(void * arg)
{
	struct timer t = *((struct timer *)arg);

	while(1)
	{
		sleep(t.timeout);
		t.fun(t.args);
	}
}



