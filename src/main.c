// -----------------------------------------------------------------------------
//
// TARDIS
//
// Time tracking for Time Lords!
//
// Or people who spend most of their time on the command line, and always forget
// to open that browser window, sign in, bugger about with the mouse until
// some drop-down gives you the correct time, click on the project, find the
// activity, select the ... oh, screw it! I'm writing my own!
//
// Author:  Will Langstroth
// License: MIT
//
// Note: This was more of an experiment in writing readable C code than
// anything. For entertainment purposes only.
// -----------------------------------------------------------------------------

#include "tardis.h"

// SQLite Callbacks
int report_row    (void *, int, char **, char **);
int list_row      (void *, int, char **, char **);
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
  char          end_sql[BUFFER_LENGTH];
  char          date_buffer[DATE_LENGTH];
  char          home_db[BUFFER_LENGTH];
  char          backup_command[BUFFER_LENGTH];

  char         *error_message = 0;
  char         *project;
  char         *description;
  struct tm    *timeinfo = NULL;

  sqlite3      *db = NULL;

  int           result = EXIT_FAILURE; // guilty until proven innocent
  const char   *time_format = "%Y-%m-%d %H:%M:%S";

  const char *insert_template =
    "insert into entries(project, description) values('%s','%s')";
  const char *update_template =
    "update entries \
     set end='%s'   \
     where start = (select max(start) from entries where end is null)";

  if (argc < 2) {
    fprintf(stderr, "Usage: %s command [options]\n", argv[0]);
    fprintf(stderr, AVAILABLE_COMMANDS);
    fprintf(stderr, "Try 'tardis help'\n");
    goto bail;
  }

  char *command = argv[1];

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

  DB_EXEC(create_entries_sql, sink, "Error creating entries table -> %s\n");

  // the estimate integer is in hours
  const char *create_tasks_sql =
    "create table if not exists tasks(            \
     id integer primary key autoincrement,        \
     parent integer default 1,                    \
     created datetime default current_timestamp,  \
     project text,                                \
     description text,                            \
     due datetime,                                \
     priority int,                                \
     estimate integer,                            \
     foreign key(parent) references tasks(id))";

  DB_EXEC(create_tasks_sql, sink, "Error creating tasks table -> %s\n");

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

  if (!strcmp(command, "task") || !strcmp(command, "t")) {

// -- Task Command -------------------------------------------------------------

    if (argc == 6) {
      project = argv[2];
      description = argv[3];
      char *estimate = argv[4];
      char task_sql[BUFFER_LENGTH];

      const char *task_template =
        "insert into tasks(project, description, estimate) \
         values('%s', '%s', '%s')";

      sprintf(task_sql, task_template, project, description, estimate);

      DB_EXEC(task_sql, sink, "SQL error: -> %s\n");

    } else if (argc == 2) {
      printf("Tasks:\n");
      const char *task_list_sql =
        "select id,       \
         parent,          \
         created,         \
         project,         \
         description,     \
         estimate         \
         from tasks       \
         where id != 0    \
         order by id";

      DB_EXEC(task_list_sql, task_list_row, "Error listing tasks -> %s\n");
    } else {
      fprintf(stderr,
          "Usage: %s t[ask] [add <project-name> <description> <estimate>]\n",
          argv[0]);
      goto bail;
    }

  } else if (!strcmp(command, "help")) {

// -- Help Command -------------------------------------------------------------

    printf(AVAILABLE_COMMANDS);

  } else if (!strcmp(command, "break")) {

// -- Break Command ------------------------------------------------------------

    project = "break";
    description = "";

    time(&rawtime);
    timeinfo = gmtime(&rawtime);
    strftime(date_buffer, DATE_LENGTH, time_format, timeinfo);

    sprintf(update_sql, update_template, date_buffer);
    DB_EXEC(update_sql, sink, "Error stopping previous entry -> %s\n");

    sprintf(insert_sql, insert_template, project, escape(description));
    DB_EXEC(insert_sql, sink, "Error starting entry -> %s\n");

  } else if (!strcmp(command, "backup") || !strcmp(command, "b")) {

// -- Backup Command -----------------------------------------------------------

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

  } else if (!strcmp(command, "end")) {

// -- End Command --------------------------------------------------------------

    if (argc != 4) {
      fprintf(stderr,
          "Usage: %s end <id> <end-time>\n",
          argv[0]);
      goto bail;
    }

    char *row_id = argv[2];
    char *end_time = argv[3];

    const char *end_template =
      "update entries set end=datetime('%s', 'utc') where id=%s";

    sprintf(end_sql, end_template, end_time, row_id);
    DB_EXEC(end_sql, sink, "Error ending entry -> %s\n");

  } else if (!strcmp(command, "start") || !strcmp(command, "s")) {

// -- Start Command ------------------------------------------------------------

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
    DB_EXEC(update_sql, sink, "Error stopping previous entry -> %s\n");

    sprintf(insert_sql, insert_template, project, escape(description));
    DB_EXEC(insert_sql, sink, "Error stopping previous entry -> %s\n");

  } else if (!strcmp(command, "report") || !strcmp(command, "r")) {

// -- Report Command -----------------------------------------------------------

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

    DB_EXEC(report_sql, report_row, "Error with report -> %s\n");

    printf("└───────────────────────┴──────────────┘\n");

  } else if (!strcmp(command, "list") || !strcmp(command, "ls")) {

// -- List Command -------------------------------------------------------------

    printf("┌─────────────────────────────┬──────────────────────┬──────────────────────────────────────────────────────────────┐\n");
    printf("│ time                        │ project              │ description                                                  │\n");
    printf("├────────────┬────────────────┼──────────────────────┼──────────────────────────────────────────────────────────────┤\n");

    const char *list_sql =
      "select date(start, 'localtime'),         \
        strftime('%H:%M', start, 'localtime'),  \
        strftime('%H:%M', end, 'localtime'),    \
        project, description                    \
       from entries                             \
       order by start";

    DB_EXEC(list_sql, list_row, "Error listing entries -> %s\n");

    printf("└────────────┴────────────────┴──────────────────────┴──────────────────────────────────────────────────────────────┘\n");

  } else if (!strcmp(command, "last")) {

// -- Last Command -------------------------------------------------------------

    const char *last_sql =
      "select                                   \
        id,                                     \
        strftime('%H:%M', start, 'localtime'),  \
        project,                                \
        description                             \
       from entries                             \
       where start = (select max(start) from entries)";

    DB_EXEC(last_sql, raw_row, "Error getting last command -> %s\n");

  } else if (!strcmp(command, "add")) {

// -- Add Command --------------------------------------------------------------

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
    DB_EXEC(add_sql, sink, "Error adding entry -> %s\n");

  } else if (!strcmp(command, "stop")) {

// -- Stop Command -------------------------------------------------------------

    if (argc < 2 || argc > 3) {
      fprintf(stderr, "Usage: %s stop [<end-time>]\n", argv[0]);
      goto bail;
    }
    const char *stop_template =
        "update entries          \
         set end=datetime('%s')  \
         where start = (select max(start) from entries where end is null)";

    char *end = "now";
    if (argc == 3) {
      end = argv[2];
    }
    sprintf(update_sql, stop_template, end);
    DB_EXEC(update_sql, sink, "Error stopping -> %s\n");

// -- End Commands -------------------------------------------------------------

  } else {
    fprintf(stderr, AVAILABLE_COMMANDS);
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
list_row(void *not_used, int argc, char *argv[], char *az_col_name[])
{
  char *short_desc = argv[4];
  short_desc[60] = '\0';
  printf("│ %s │ %s to %s │ %-20s │ %-60s │\n",
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
sink(void *not_used, int argc, char *argv[], char *az_col_name[])
{
  return 0;
}
