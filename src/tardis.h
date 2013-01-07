#ifndef TARDIS_H
#define TARDIS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sqlite3.h>
#include <sys/stat.h>

#define BUFFER_LENGTH 4096
#define TIME_LENGTH     64
#define DATE_LENGTH     20
#define AVAILABLE     "Available commands:\nall\nadd\nb[ackup]\nend\nlast\nlist (ls)\nr[eport]\ns[tart]\nstop\nt[ask]\n"

int time_string_to_seconds(char *);
char *seconds_to_time_string(int);
char *str_replace_all(const char *, const char *, const char *);
char *escape(const char *);

#endif
