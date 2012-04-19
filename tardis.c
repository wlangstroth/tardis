// -----------------------------------------------------------------------------
//
// tardis.c
//
// Time tracking for Timelords!
//
// Or people who spend most of their time on the command line, and always forget
// to open that browser window, sign in, bugger about with the mouse until
// some drop-down gives you the correct time, click on the project, find the
// activity, select the ... oh, screw it! I'm writing my own!
//
// Author:  Will Langstroth
// License: MIT
//
// -----------------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096
#define DATE_LENGTH     20
#define TIME_LENGTH     32

static int report_row(void *, int, char **, char **);
static int all_row(void *, int, char **, char **);
static int sink(void *, int, char **, char **);
char *seconds_to_time_string(int);
char *escape(char *);
char *str_replace(char *, char *, char *);
char *str_replace_all(const char *, const char *, const char *);

int
main(int argc, char *argv[])
{
  sqlite3      *db;
  char         *error_message = 0;
  int           result_code;
  char          update_sql[BUFFER_LENGTH];
  char          insert_sql[BUFFER_LENGTH];
  char          add_sql[BUFFER_LENGTH];
  char          date_buffer[DATE_LENGTH];
  char          home_db[BUFFER_LENGTH];
  time_t        rawtime;
  struct tm    *timeinfo;
  static char  *time_format = "%Y-%m-%d %H:%M:%S";
  char *project;
  char *description;

  static char *insert_template =
    "insert into entries(project, description) values('%s','%s')";
  static char *update_template =
    "update entries \
      set end='%s'  \
      where start = (select max(start) from entries)";

  if (argc < 2) {
    fprintf(stderr, "Usage: %s mode [options]\n", argv[0]);
    fprintf(stderr, "(More usage information)\n");
    exit(EXIT_FAILURE);
  }

  char *mode = argv[1];

  sprintf(home_db, "%s/.tardis.db", getenv("HOME"));

  // This will create a new ~/.tardis.db file if one does not exist.
  result_code = sqlite3_open(home_db, &db);
  if (result_code) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    sqlite3_close(db);
    exit(EXIT_FAILURE);
  }

  static char *create_sql =
    "create table if not exists entries(        \
      id integer primary key autoincrement,     \
      start datetime default current_timestamp, \
      project text,                             \
      description text,                         \
      end datetime)";

  result_code = sqlite3_exec(db, create_sql, sink, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
  }

  if (!strcmp(mode, "s") || !strcmp(mode, "start")) {
// -----------------------------------------------------------------------------
// Start Mode
// -----------------------------------------------------------------------------

    if (argc < 3) {
      fprintf(stderr, "Usage: %s s[tart] project_name [description]\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    project = argv[2];
    description = argv[3];

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    sprintf(insert_sql, insert_template, project, escape(description));
    printf("%s", insert_sql);
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  } else if (!strcmp(mode, "export")) {
// -----------------------------------------------------------------------------
// Export Mode
// -----------------------------------------------------------------------------
    printf("Export to Harvest coming soon.\n");

  } else if (!strcmp(mode, "report")) {
// -----------------------------------------------------------------------------
// Report Mode
// -----------------------------------------------------------------------------

    printf("+-----------------------+--------------+\n");
    printf("| project               | time         |\n");
    printf("+-----------------------+--------------+\n");

    static char *report_sql =
      "select project, \
        sum(strftime('%s',end) - strftime('%s',start)) as seconds \
        from entries group by project";
    result_code = sqlite3_exec(db, report_sql, report_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    printf("+-----------------------+--------------+\n");

  } else if (!strcmp(mode, "all")) {
// -----------------------------------------------------------------------------
// All Mode
// -----------------------------------------------------------------------------

    printf("+---------------------------------------------+-------------------------+-------------------------------\n");
    printf("| time                                        | project                 | description                    \n");
    printf("+---------------------------------------------+-------------------------+-------------------------------\n");

    static char *all_sql =
      "select start, end, project, description from entries order by start";
    result_code = sqlite3_exec(db, all_sql, all_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    printf("+---------------------------------------------+-------------------------+-------------------------------\n");

  } else if (!strcmp(mode, "add")) {
// -----------------------------------------------------------------------------
// Add Mode
// -----------------------------------------------------------------------------

    if (argc < 5 || argc > 6) {
      fprintf(stderr, "Usage: %s add project_name start end [description]\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    project = argv[2];
    char *start = argv[3];
    char *end = argv[4];
    description = argv[5];
    static char *add_template =
      "insert into entries(project, start, end, description) values('%s','%s','%s','%s')";

    sprintf(add_sql, add_template, project, start, end, escape(description));

    result_code = sqlite3_exec(db, add_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  } else if (!strcmp(mode, "switch")) {
// -----------------------------------------------------------------------------
// Switch Mode
// -----------------------------------------------------------------------------

    // same as `tardis s switch`
    if (argc != 2) {
      fprintf(stderr, "Usage: %s switch\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    sprintf(insert_sql, insert_template, "switch", "");
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  } else if (!strcmp(mode, "all")) {
// -----------------------------------------------------------------------------
// Switch Mode
// -----------------------------------------------------------------------------

    if (argc != 2) {
      fprintf(stderr, "Usage: %s all\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    project = argv[2];
    description = argv[3];

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

    sprintf(insert_sql, insert_template, project, description);
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  } else if (!strcmp(mode, "stop")) {
// -----------------------------------------------------------------------------
// Stop Mode
// -----------------------------------------------------------------------------

    if (argc != 2) {
      fprintf(stderr, "Usage: %s stop\n", argv[0]);
      exit(EXIT_FAILURE);
    }

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
    }

  } else {
    fprintf(stderr, "Unrecognized mode\n");
    fprintf(stderr, "Available modes: s[tart], stop, report, add\n");
    exit(EXIT_FAILURE);
  }

  sqlite3_close(db);
  return EXIT_SUCCESS;
}

// -----------------------------------------------------------------------------
// Callback Functions
// -----------------------------------------------------------------------------
static int
report_row(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  long time_spent = argv[1] ? atoi(argv[1]) : 0;

  printf("| %-21s | %12s |\n", argv[0], seconds_to_time_string(time_spent));
  return 0;
}

static int
all_row(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  printf("| %s to %s | %s | %s |\n", argv[0], argv[1], argv[2], argv[3]);
  return 0;
}

static int
sink(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  return 0;
}

// -----------------------------------------------------------------------------
// Utility Functions
// -----------------------------------------------------------------------------
char *
seconds_to_time_string(int seconds) {
  int h, m, s;
  static char buff[TIME_LENGTH];

  if (seconds > 0) {
    m = seconds / 60;
    s = seconds % 60;
    h = m / 60;
    m = m % 60;
  } else {
    return "0h  0m  0s";
  }

  sprintf(buff, "%2dh %2dm %2ds", h, m, s);

  return buff;
}

char *
escape(char *query) {
  static char buffer[BUFFER_LENGTH];
  char new_string[BUFFER_LENGTH];

  return new_string;
}

char *
str_newlace(const char *str, const char *old, const char *new)
{
  static char buffer[BUFFER_LENGTH];
  char *p;
  long d;

  p = strstr(str, old);
  d = p - str;

  if (!p) return str;

  strncpy(buffer, str, d);
  buffer[d] = '\0';

  sprintf(buffer + d, "%s%s", new, p + strlen(old));

  return buffer;
}
