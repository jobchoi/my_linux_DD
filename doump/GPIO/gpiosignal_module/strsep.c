#include <stdio.h>
#include <string.h>

int main( void)
{
	char *sep = ":";
	char *cmd, *str;
	char *pidstr;


   str = strdup( "write:www.google.com");
   printf( "%s\n", str);

   cmd = strsep(&str, sep);
   pidstr = strsep(&str, sep);

   printf("Command : %s, Pid : %s\n", cmd, pidstr);

   return 0;
}
