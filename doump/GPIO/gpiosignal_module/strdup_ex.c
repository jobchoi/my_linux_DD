#include <stdio.h>
#include <string.h>

int main( void)
{
   char *ptr;
  
   ptr = strdup( "www.google.com");
   printf( "%s\n", ptr);

   return 0;
}