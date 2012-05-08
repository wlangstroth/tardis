// -----------------------------------------------------------------------------
//
// TARDIS
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

#include "tardis.h"

// SQLite Callbacks
int report_row    (void *, int, char **, char **);
int all_row       (void *, int, char **, char **);
int task_list_row (void *, int, char **, char **);
int raw_row       (void *, int, char **, char **);
int sink          (void *, int, char **, char **);


int
main(int argc, char *argv[])
{
  int           result_code;
  time_t        rawtime;

  char          update_sql[BUFFER_LENGTH];
  char          insert_sql[BUFFER_LENGTH];
  char          add_sql[BUFFER_LENGTH];
  char          date_buffer[DATE_LENGTH];
  char          home_db[BUFFER_LENGTH];
  char          backup_command[BUFFER_LENGTH];

  char         *error_message = 0;
  char         *project;
  char         *description;
  struct tm    *timeinfo;

  sqlite3      *db;

  const char   *time_format = "%Y-%m-%d %H:%M:%S";
  int result = EXIT_FAILURE; // guilty until proven innocent

  const char *insert_template =
    "insert into entries(project, description) values('%s','%s')";
  const char *update_template =
    "update entries \
      set end='%s'  \
      where start = (select max(start) from entries where end is null)";

  if (argc < 2) {
    fprintf(stderr, "Usage: %s mode [options]\n", argv[0]);
    fprintf(stderr, "(More usage information)\n");
    goto bail;
  }

  char *mode = argv[1];

  sprintf(home_db, "%s/.tardis/current.db", getenv("HOME"));

  // This will create a new ~/.tardis.db file if one does not exist.
  result_code = sqlite3_open(home_db, &db);
  if (result_code) {
    fprintf(stderr, "Error opening database -> %s\n", sqlite3_errmsg(db));
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
    fprintf(stderr, "Error creating entries table -> %s\n", error_message);
    sqlite3_free(error_message);
    goto bail;
  }

  // the estimate integer is in hours
  const char *create_tasks_sql =
    "create table if not exists tasks(        \
     id integer primary key autoincrement,    \
     parent integer default 1,                \
     stamp datetime default current_datetime, \
     project text,                            \
     description text,                        \
     due datetime,                            \
     priority int,                            \
     estimate integer,                        \
     foreign key(parent) references tasks(id))";

  result_code = sqlite3_exec(db, create_tasks_sql, sink, 0, &error_message);
  if (result_code) {
    fprintf(stderr, "Error creating tasks table -> %s\n", error_message);
    sqlite3_free(error_message);
    goto bail;
  }

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

  if (!strcmp(mode, "task") || !strcmp(mode, "t")) {
// -----------------------------------------------------------------------------
// Task Command
// -----------------------------------------------------------------------------

    if (argc == 5) {
      project = argv[2];
      description = argv[3];
      char *estimate = argv[4];
      char task_sql[BUFFER_LENGTH];

      const char *task_template =
        "insert into tasks(project, description, estimate) \
         values('%s', '%s', '%s')";

      sprintf(task_sql, task_template, project, description, estimate);

      result_code = sqlite3_exec(db, task_sql, sink, 0, &error_message);
      if (result_code) {
        fprintf(stderr, "SQL error: %s\n", error_message);
        sqlite3_free(error_message);
        goto bail;
      }
    } else if (argc == 2) {
      printf("Task list here\n");
      const char *task_list_sql =
        "select id,       \
         parent,          \
         stamp,           \
         project,         \
         description,     \
         estimate         \
         from tasks       \
         where id != 0    \
         order by id";

      result_code = sqlite3_exec(db, task_list_sql, task_list_row, 0, &error_message);
      if (result_code) {
        fprintf(stderr, "Error listing tasks -> %s\n", error_message);
        sqlite3_free(error_message);
        goto bail;
      }
    } else {
      fprintf(stderr,
          "Usage: %s t[ask] [add] [<project-name> <description> <estimate>]\n",
          argv[0]);
      goto bail;
    }


  } else if (!strcmp(mode, "backup") || !strcmp(mode, "b")) {
// -----------------------------------------------------------------------------
// Backup Command
// -----------------------------------------------------------------------------

    const char *date_format = "%Y-%m-%d";

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, date_format, timeinfo);

    sprintf(backup_command,
        "cp %s/.tardis/current.db %s/.tardis/%s.db",
        getenv("HOME"),
        getenv("HOME"),
        date_buffer);

    if (system(backup_command)) {
      fprintf(stderr, "Error backing up\n");
      goto bail;
    }

    printf("Backed up\n");
  } else if (!strcmp(mode, "start") || !strcmp(mode, "s")) {
// -----------------------------------------------------------------------------
// Start Command
// -----------------------------------------------------------------------------

    if (argc < 3 || argc > 4) {
      fprintf(stderr,
          "Usage: %s s[tart] <project-name> [<description>]\n",
          argv[0]);
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
      fprintf(stderr, "Error stopping previous entry -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

    sprintf(insert_sql, insert_template, project, escape(description));
    result_code = sqlite3_exec(db, insert_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "Error starting entry -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else if (!strcmp(mode, "report") || !strcmp(mode, "r")) {
// -----------------------------------------------------------------------------
// Report Command
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
      sprintf(where_clause,
          "where date(start, 'localtime') between '%s' and '%s'",
          argv[2],
          argv[3]);
      sprintf(report_sql, report_template, where_clause);
    }

    printf("┌───────────────────────┬──────────────┐\n");
    printf("│ project               │ time         │\n");
    printf("├───────────────────────┼──────────────┤\n");

    result_code = sqlite3_exec(db, report_sql, report_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "Error with report -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

    printf("└───────────────────────┴──────────────┘\n");

  } else if (!strcmp(mode, "all")) {
// -----------------------------------------------------------------------------
// All Command
// -----------------------------------------------------------------------------

    printf("┌─────────────────────────────┬──────────────────────┬────────────────────────────────────────────────────┐\n");
    printf("│ time                        │ project              │ description                                        │\n");
    printf("├────────────┬────────────────┼──────────────────────┼────────────────────────────────────────────────────┤\n");

    const char *all_sql =
      "select date(start, 'localtime'),         \
        strftime('%H:%M', start, 'localtime'),  \
        strftime('%H:%M', end, 'localtime'),    \
        project, description                    \
       from entries                             \
       order by start";

    result_code = sqlite3_exec(db, all_sql, all_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "Error listing entries -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

    printf("└────────────┴────────────────┴──────────────────────┴────────────────────────────────────────────────────┘\n");

  } else if (!strcmp(mode, "last")) {
// -----------------------------------------------------------------------------
// Last Command
// -----------------------------------------------------------------------------

    const char *last_sql =
      "select                                           \
        id,                                             \
        strftime('%H:%M', start, 'localtime'),          \
        project,                                        \
        description                                     \
       from entries                                     \
       where start = (select max(start) from entries)";

    result_code = sqlite3_exec(db, last_sql, raw_row, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "Error getting last command -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else if (!strcmp(mode, "add")) {
// -----------------------------------------------------------------------------
// Add Command
// -----------------------------------------------------------------------------

    if (argc < 5 || argc > 6) {
      fprintf(stderr,
          "Usage: %s add <project_name> <start> <end> [<description>]\n",
          argv[0]);
      goto bail;
    }

    project = argv[2];
    char *start = argv[3];
    char *end = argv[4];
    description = argv[5] ? argv[5] : "";
    const char *add_template =
      "insert into entries(project, start, end, description) \
      values('%s',datetime('%s', 'utc'),datetime('%s', 'utc'),'%s')";

    sprintf(add_sql, add_template, project, start, end, escape(description));

    result_code = sqlite3_exec(db, add_sql, sink, 0, &error_message);
    if (result_code) {
      fprintf(stderr, "Error adding entry -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else if (!strcmp(mode, "stop")) {
// -----------------------------------------------------------------------------
// Stop Command
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
      fprintf(stderr, "Error stopping -> %s\n", error_message);
      sqlite3_free(error_message);
      goto bail;
    }

  } else if (!strcmp(mode, "backup")) {
// -----------------------------------------------------------------------------
// Backup Command
// -----------------------------------------------------------------------------

    // export sql to some backup folder

  } else {
    fprintf(stderr,
        "Available commands: all, add, last, r[eport], s[tart], stop, t[ask]\n");
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
int
report_row(void *not_used, int argc, char *argv[], char *az_col_name[])
{
  long time_spent = argv[1] ? atoi(argv[1]) : 0;

  printf("│ %-21s │ %12s │\n", argv[0], seconds_to_time_string(time_spent));
  return 0;
}

int
raw_row(void *not_used, int argc, char *argv[], char *az_col_name[])
{
  int i;

  for (i = 0; i < argc; i++) {
    printf("| %s ", argv[i]);
  }
  printf("|\n");

  return 0;
}

int
all_row(void *not_used, int argc, char *argv[], char *az_col_name[])
{
  printf("│ %s │ %s to %s │ %-20s │ %-50s │\n",
      argv[0],
      argv[1],
      argv[2] ? argv[2] : "     ",
      argv[3],
      argv[4]);
  return 0;
}

int
task_list_row(void *not_used, int argc, char *argv[], char *az_col_name[])
{
  printf("│ %s │ %s │ %s │ %s │ %s │ %s │ %s │ %s │\n",
      argv[0],
      argv[1],
      argv[2],
      argv[3],
      argv[4],
      argv[5],
      argv[6],
      argv[7]);
  return 0;
}

int
sink(void *not_used, int argc, char *argv[], char *az_col_name[]) {
  return 0;
}
