#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

int
main (int args, char *argv[])
{
  FILE *fp;
  time_t now;

  time(&now);

  fp = fopen("log.txt","a");

  fprintf(fp, "%s - %s", argv[1], ctime(&now));

  fclose(fp);

  exit(EXIT_SUCCESS);
}
