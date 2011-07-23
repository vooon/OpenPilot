#include "stdio.h"
#include "pios_sim_priv.h"
#include "sim_model.h"

int main() 
{
	int retval = sim_model_init();
	printf( "Returned %u\n", retval );
	return 0;
}
