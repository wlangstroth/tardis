#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096
#define DATE_LENGTH    128

// Forward declarations for sqlite3_exec() callbacks
static int post_create(void *, int, char **, char **);
static int post_update(void *, int, char **, char **);
static int post_insert(void *, int, char **, char **);

int
main(int argc, char *argv[])
{
  sqlite3      *db;
  char         *error_message = 0;
  int           result_code;
  char          update_sql[BUFFER_LENGTH];
  char          insert_sql[BUFFER_LENGTH];
  char          date_buffer[DATE_LENGTH];
  time_t        rawtime;
  struct tm    *timeinfo;

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s project_name [description]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *project = argv[1];
  char *description = argv[2];


  // This will create a new log.db file if one does not exist, but will not
  // populate it with an entries table, so we have to do that.
  result_code = sqlite3_open("log.db", &db);
  if (result_code == SQLITE_CANTOPEN) {
    fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  static char  *create_sql = "create table if not exists entries(id integer primary key autoincrement, start datetime default current_timestamp, project text, description text, end datetime)";
  result_code = sqlite3_exec(db, create_sql, post_create, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  static char  *update_template = "update entries set end='%s' where id = (select max(id) from entries);";
  static char  *time_format = "%Y-%m-%d %H:%M:%S";
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

  sprintf(update_sql, update_template, date_buffer);
  result_code = sqlite3_exec(db, update_sql, post_update, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  static char  *insert_template = "insert into entries(project, description) values('%s','%s');";
  sprintf(insert_sql, insert_template, project, description);
  result_code = sqlite3_exec(db, insert_sql, post_insert, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}

static int
post_create(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("table created\n");
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
