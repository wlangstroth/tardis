#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096
#define DATE_LENGTH     20

static int report(void *, int, char **, char **);
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
  char *project;
  char *description;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s mode [options]\n", argv[0]);
    fprintf(stderr, "(More usage information)\n");
    exit(EXIT_FAILURE);
  }

  char *mode = argv[1];

  // This will create a new ~/.tardis.db file if one does not exist..
  // TODO: fix this hardcoded value
  // result_code = sqlite3_open("/Users/will/.tardis.db", &db);
  result_code = sqlite3_open("tardis.db", &db);
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

// -----------------------------------------------------------------------------
// Start Mode
// -----------------------------------------------------------------------------
  if (strcmp(mode, "s") == 0 || strcmp(mode, "start") == 0) {

    if (argc < 3) {
      fprintf(stderr, "Usage: %s s[tart] project_name [description]\n", argv[0]);
      exit(EXIT_FAILURE);
    }
    project = argv[2];
    description = argv[3];

    static char  *update_template = "update entries set end='%s' where start = (select max(start) from entries)";
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    static char  *insert_template = "insert into entries(project, description) values('%s','%s')";
    sprintf(insert_sql, insert_template, project, description);
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  }

// -----------------------------------------------------------------------------
// Export Mode
// -----------------------------------------------------------------------------
  if (strcmp(mode, "export") == 0) {
    printf("Export feature coming soon.\n");
  }

// -----------------------------------------------------------------------------
// Report Mode
// -----------------------------------------------------------------------------
  if (strcmp(mode, "report") == 0) {

    printf("+----------------+----------------+\n");
    printf("| project        | time           |\n");
    printf("+----------------+----------------+\n");

    static char *report_sql = "select project, sum(strftime('%s',end) - strftime('%s',start)) as seconds from entries group by project";
    result_code = sqlite3_exec(db, report_sql, report, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    printf("+----------------+----------------+\n");
  }

// -----------------------------------------------------------------------------
// Stop Mode
// -----------------------------------------------------------------------------
  if (strcmp(mode, "stop") == 0) {

    static char  *update_template = "update entries set end='%s' where start = (select max(start) from entries)";
    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}

static int
report(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("| %-14s | %14s |\n", argv[0], argv[1] ? argv[1] : "NULL");
  return 0;
}

static int
sink(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  return 0;
}
