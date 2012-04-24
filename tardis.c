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
int time_string_to_seconds(char *);
char *seconds_to_time_string(int);
char *str_replace_all(const char *, const char *, const char *);
char *escape(const char *);

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
  int result = EXIT_FAILURE; // guilty until proven innocent

  const char *insert_template =
    "insert into entries(project, description) values('%s','%s')";
  const char *update_template =
    "update entries \
      set end='%s'  \
      where start = (select max(start) from entries)";

  if (argc < 2) {
    fprintf(stderr, "Usage: %s mode [options]\n", argv[0]);
    fprintf(stderr, "(More usage information)\n");
    goto bail;
  }

  char *mode = argv[1];

  sprintf(home_db, "%s/.tardis.db", getenv("HOME"));

  // This will create a new ~/.tardis.db file if one does not exist.
  result_code = sqlite3_open(home_db, &db);
  if (result_code) {
    fprintf(stderr, "%s\n", sqlite3_errmsg(db));
    goto bail;
  }

// -----------------------------------------------------------------------------
// Table set-up
// -----------------------------------------------------------------------------

  const char *create_entries_sql =
    "create table if not exists entries(        \
     id integer primary key autoincrement,      \
     start datetime default current_timestamp,  \
     project text,                              \
     description text,                          \
     end datetime)";

  result_code = sqlite3_exec(db, create_entries_sql, sink, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    goto bail;
  }

  // the estimate integer is in seconds
  const char *create_tasks_sql =
    "create table if not exists tasks(        \
     id integer primary key autoincrement,    \
     stamp datetime default current_datetime, \
     project text,                            \
     description text,                        \
     estimate integer)";

  result_code = sqlite3_exec(db, create_tasks_sql, sink, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "SQL error: %s\n", error_message);
    sqlite3_free(error_message);
    goto bail;
  }

// -----------------------------------------------------------------------------
// Modes (commands)
// -----------------------------------------------------------------------------

  if (!strcmp(mode, "task") || !strcmp(mode, "t")) {
// -----------------------------------------------------------------------------
// Task Mode
// -----------------------------------------------------------------------------

    if (argc < 4) {
      fprintf(stderr, "Usage: %s t[ask] <project_name> <description> <estimate>\n", argv[0]);
      goto bail;
    }

  } else if (!strcmp(mode, "start") || !strcmp(mode, "s")) {
// -----------------------------------------------------------------------------
// Start Mode
// -----------------------------------------------------------------------------

    if (argc < 3) {
      fprintf(stderr, "Usage: %s s[tart] <project_name> [<description>]\n", argv[0]);
      goto bail;
    }

    project = argv[2];
    description = argv[3] ? argv[3] : "";

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

    sprintf(insert_sql, insert_template, project, escape(description));
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else if (!strcmp(mode, "report") || !strcmp(mode, "r")) {
// -----------------------------------------------------------------------------
// Report Mode
// -----------------------------------------------------------------------------

    const char *report_template =
      "select project,                                    \
       sum(strftime('%%s',end) - strftime('%%s', start))  \
       from entries                                       \
       %s                                                 \
       group by project";

    static char report_sql[BUFFER_LENGTH];
    static char where_clause[BUFFER_LENGTH];

    if (argc == 2) {
      sprintf(report_sql, report_template, "");
    }

    if (argc == 3) {
      sprintf(where_clause, "where date(start, 'localtime') = '%s'", argv[2]);
      sprintf(report_sql, report_template, where_clause);
    }

    if (argc == 4) {
      sprintf(where_clause, "where date(start, 'localtime') between '%s' and '%s'", argv[2], argv[3]);
      sprintf(report_sql, report_template, where_clause);
    }

    printf("┌───────────────────────┬──────────────┐\n");
    printf("│ project               │ time         │\n");
    printf("├───────────────────────┼──────────────┤\n");

    result_code = sqlite3_exec(db, report_sql, report_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

    printf("└───────────────────────┴──────────────┘\n");

  } else if (!strcmp(mode, "all")) {
// -----------------------------------------------------------------------------
// All Mode
// -----------------------------------------------------------------------------

    printf("┌─────────────────────────────┬──────────────────────┬────────────────────────────────────────────────────┐\n");
    printf("│ time                        │ project              │ description                                        │\n");
    printf("├─────────────────────────────┼──────────────────────┼────────────────────────────────────────────────────┤\n");

    static char *all_sql =
      "select date(start, 'localtime'),        \
        strftime('%H:%M', start, 'localtime'), \
        strftime('%H:%M', end, 'localtime'),   \
        project, description      \
       from entries               \
       order by start";
    result_code = sqlite3_exec(db, all_sql, all_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

    printf("└─────────────────────────────┴──────────────────────┴────────────────────────────────────────────────────┘\n");

  } else if (!strcmp(mode, "add")) {
// -----------------------------------------------------------------------------
// Add Mode
// -----------------------------------------------------------------------------

    if (argc < 5 || argc > 6) {
      fprintf(stderr, "Usage: %s add <project_name> <start> <end> [<description>]\n", argv[0]);
      goto bail;
    }

    project = argv[2];
    char *start = argv[3];
    char *end = argv[4];
    description = argv[5];
    static char *add_template =
      "insert into entries(project, start, end, description) \
      values('%s',datetime('%s', 'utc'),datetime('%s', 'utc'),'%s')";

    sprintf(add_sql, add_template, project, start, end, escape(description));

    result_code = sqlite3_exec(db, add_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else if (!strcmp(mode, "stop")) {
// -----------------------------------------------------------------------------
// Stop Mode
// -----------------------------------------------------------------------------

    if (argc != 2) {
      fprintf(stderr, "Usage: %s stop\n", argv[0]);
      goto bail;
    }

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    result_code = sqlite3_exec(db, update_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "SQL error: %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else {
    fprintf(stderr, "Unrecognized mode\n");
    fprintf(stderr, "Available modes: s[tart], r[eport], all, add, stop, t[ask]\n");
    goto bail;
  }

  result = EXIT_SUCCESS;

bail:

  sqlite3_close(db);
  return result;
}

// -----------------------------------------------------------------------------
// Callback Functions
// -----------------------------------------------------------------------------
static int
report_row(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  long time_spent = argv[1] ? atoi(argv[1]) : 0;

  printf("│ %-21s │ %12s │\n", argv[0], seconds_to_time_string(time_spent));
  return 0;
}

static int
all_row(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  char *end_time = argv[2] ? argv[2] : "     ";

  printf("│ %s - %s to %s │ %-20s │ %-50s │\n", argv[0], argv[1], end_time, argv[3], argv[4]);
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

int
time_string_to_seconds(char *time_string) {
  int h, m, s;
  sscanf("%dh %dm %ds", h, m, s);
  return h * 3600 + m * 60 + s;
}

char *
str_replace_all(const char *string, const char *substr, const char *replacement) {
  char *token = NULL;
  char *new_string = NULL;
  char *old_string = NULL;
  char *head = NULL;
  size_t delta;
  size_t rep_length = strlen(replacement);
  size_t sub_length = strlen(substr);
  size_t old_length;

  if (substr == NULL || replacement == NULL)
    return strdup(string);

  new_string = strdup(string);
  head = new_string;

  while ((token = strstr(head, substr))) {
    old_string = new_string;
    new_string = malloc(strlen(old_string) - strlen(substr) + rep_length + 1);

    if (new_string == NULL) {
      free(old_string);
      return NULL;
    }

    delta = token - old_string;
    old_length = strlen(old_string);

    memcpy(new_string, old_string, delta);
    memcpy(new_string + delta, replacement, rep_length);
    memcpy(new_string + delta + rep_length, token + sub_length, old_length - sub_length - delta);

    memset(new_string + old_length - sub_length + rep_length, 0, 1);

    head = new_string + delta + rep_length;
    free(old_string);
  }
  return new_string;
}

char *
escape(const char *string) {
  return str_replace_all(string, "'", "''");
}
