#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main ( int argc, char *argv[] )
{
	if ( argc != 2 ) {
		fprintf( stderr, "insufficient arguments\n");
		exit(EXIT_FAILURE);
	}

	int keylength = 0;
	int arglen = strlen(argv[1]);
	int i = 0;
	for (i = 0; i < arglen; i++)
	{
		if ( argv[1][i] < 48 || argv[1][i] > 57)
		{
			fprintf( stderr, "invalid integer\n");
			exit(EXIT_FAILURE);
		}
		keylength = (keylength * 10) + ( argv[1][i] - '0' ) ;	
	}
	
	time_t t;
	srand((unsigned) time(&t));

	for ( i = 0; i < keylength; i++ )
	{
		int rnd = ( rand() %  27 );
		if (rnd == 0) {
			printf(" ");
		} else {
			rnd += 64;
			printf("%c", rnd); 
		}
	}
	printf("\n");
	
	return 0;
}
