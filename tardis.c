#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096

static int callback(void *NotUsed, int argc, char **argv, char **az_col_name);

int
main(int argc, char **argv)
{
  sqlite3 *db;
  char *z_err_msg = 0;
  int result_code;
  char sql[BUFFER_LENGTH];

  if (argc < 2) {
    fprintf(stderr, "Usage: %s project_name [description]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *project = argv[1];
  char *action = "START";

  sprintf(sql, "insert into entries(project, description) values('%s','%s');", project, action);

  result_code = sqlite3_open("log.db", &db);
  if (result_code) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  result_code = sqlite3_exec(db, sql, callback, 0, &z_err_msg);
  if (result_code != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", z_err_msg);
    sqlite3_free(z_err_msg);
  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}


static int
callback(void *NotUsed, int argc, char **argv, char **az_col_name) {
  printf("\n");
  return 0;
}
