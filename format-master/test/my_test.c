//#include <stdio.h>
//#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include "format.h"
#include "format_config.h"

static void * outfunc( void * op, const char * buf, size_t n )
{
    while ( n-- )
        putchar( *buf++ );

    return (void *)( !NULL );
}

static int test_printf ( const char *fmt, ... )
{
    va_list arg;
    int done;
    
    va_start ( arg, fmt );
    done = format( outfunc, NULL, fmt, arg );
    va_end ( arg );
    
    return done;
}

// static void * bufwrite( void * memptr, const char * buf, size_t n )
// {
//     return ( (char *)memcpy( memptr, buf, n ) + n );
// }

// static int test_sprintf( char *buf, const char *fmt, ... )
// {
//     va_list arg;
//     int done;

//     va_start ( arg, fmt );
//     done = format( bufwrite, buf, fmt, arg );
//     if ( 0 <= done )
//         buf[done] = '\0';
//     va_end ( arg );

//     return done;
// }


int main(void)
{
    //char s[100];
    int num_int = 234;

    test_printf("%d\n", num_int);

    //test_sprintf(s, "%d", num_int);

    //printf("%s\n", s);
    return 0;
}
