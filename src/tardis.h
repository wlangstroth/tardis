#ifndef TARDIS_H
#define TARDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>

#define BUFFER_LENGTH 4096
#define TIME_LENGTH     64
#define DATE_LENGTH     20

int time_string_to_seconds(char *);
char *seconds_to_time_string(int);
char *str_replace_all(const char *, const char *, const char *);
char *escape(const char *);

#endif
