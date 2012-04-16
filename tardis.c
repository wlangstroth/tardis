#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096

static int callback(void *, int, char **, char **);
static int post_update(void *, int, char **, char **);
static int post_insert(void *, int, char **, char **);

int
main(int argc, char **argv)
{
  sqlite3 *db;
  char *z_err_msg = 0;
  int result_code;
  char update_sql[BUFFER_LENGTH];
  char insert_sql[BUFFER_LENGTH];

  if (argc < 2) {
    fprintf(stderr, "Usage: %s project_name [description]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *project = argv[1];
  char *action = "START";

  sprintf(insert_sql, "insert into entries(start, project, description) values('%s','%s');", project, action);

  result_code = sqlite3_open("log.db", &db);
  if (result_code) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  result_code = sqlite3_exec(db, insert_sql, callback, 0, &z_err_msg);
  if (result_code != SQLITE_OK) {
    fprintf(stderr, "SQL error: %s\n", z_err_msg);
    sqlite3_free(z_err_msg);
  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}


static int
callback(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("\n");
  return 0;
}

static int
select_display(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("select statement\n");
  return 0;
}

static int
post_update(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  return 0;
}

static int
post_insert(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  return 0;
}
