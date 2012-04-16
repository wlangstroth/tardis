#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096
#define DATE_LENGTH    128

static int post_update(void *, int, char **, char **);
static int post_insert(void *, int, char **, char **);
static int post_select(void *, int, char **, char **);

int
main(int argc, char *argv[])
{
  sqlite3      *db;
  char         *error_message = 0;
  int           result_code;
  char          update_sql[BUFFER_LENGTH];
  char          insert_sql[BUFFER_LENGTH];
  char          date_buffer[DATE_LENGTH];
  static char  *update_template = "update entries set end='%s' where id = (select max(id) from entries);";
  static char  *insert_template = "insert into entries(start, project, description) values('%s','%s');";
  static char  *select_sql = "select * from entries";
  time_t        rawtime;
  struct tm    *timeinfo;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s project_name [description]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *project = argv[1];
  char *description = argv[2];

  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(date_buffer, DATE_LENGTH, "%Y-%m-%d %H:%M:%S", timeinfo);
  sprintf(update_sql, update_template, date_buffer);
  sprintf(insert_sql, insert_template, project, description);

  // This will create a new log.db file if one does not exist, but will not
  // populate it with an entries table
  result_code = sqlite3_open("log.db", &db);
  if (result_code == SQLITE_CANTOPEN) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  result_code = sqlite3_exec(db, update_sql, post_update, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  result_code = sqlite3_exec(db, insert_sql, post_insert, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  result_code = sqlite3_exec(db, select_sql, post_select, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}

static int
post_select(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  int i;

  for (i = 0; i < argc; i++) {
    printf("%s = %s\n", az_col_name[i], argv[i] ? argv[i] : "NULL");
  }

  printf("\n");
  return 0;
}

static int
post_update(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("update last entry\n");
  return 0;
}

static int
post_insert(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("insert entry\n");
  return 0;
}
