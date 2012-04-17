#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096
#define DATE_LENGTH     20

static int sink(void *, int, char **, char **);

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
  static char  *time_format = "%Y-%m-%d %H:%M:%S";

  if (argc < 2 || argc > 3) {
    fprintf(stderr, "Usage: %s project_name [description]\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  char *project = argv[1];
  char *description = argv[2];


  // This will create a new ~/.tardis.db file if one does not exist, but will not
  // populate it with an entries table, so we have to do that.
  // TODO: fix this hardcoded value
  result_code = sqlite3_open("/Users/will/.tardis.db", &db);
  if (result_code) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  static char  *create_sql = "create table if not exists entries(id integer primary key autoincrement, start datetime default current_timestamp, project text, description text, end datetime)";
  result_code = sqlite3_exec(db, create_sql, sink, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  static char  *update_template = "update entries set end='%s' where id = (select max(id) from entries);";
  time(&rawtime);
  timeinfo = gmtime(&rawtime);
  strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

  sprintf(update_sql, update_template, date_buffer);
  result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  if (strcmp(project, "stop")) {
    static char  *insert_template = "insert into entries(project, description) values('%s','%s');";
    sprintf(insert_sql, insert_template, project, description);
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }
  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}

static int
sink(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  return 0;
}
