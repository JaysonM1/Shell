/* file: test-1+2.c
   tiny code that prints something to stdout and stderr 
   for testing purposes.
*/
#include <stdio.h>

int main()
{
  fprintf(stdout, "This is to standard output\n");
  fprintf(stderr, "This is to standard error\n");

  return 0;

