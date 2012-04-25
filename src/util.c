#include "tardis.h"

char *
seconds_to_time_string(int seconds)
{
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
time_string_to_seconds(char *time_string)
{
  int h = 0;
  int m = 0;
  int s = 0;

  sscanf(time_string, "%dh %dm %ds", h, m, s);
  return h * 3600 + m * 60 + s;
}

char *
str_replace_all(const char *string, const char *substr, const char *replacement)
{
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
    new_string = malloc(strlen(old_string) - sub_length + rep_length + 1);

    if (new_string == NULL) {
      free(old_string);
      return NULL;
    }

    delta = token - old_string;
    old_length = strlen(old_string);

    memcpy(new_string, old_string, delta);
    memcpy(new_string + delta, replacement, rep_length);
    memcpy(new_string + delta + rep_length,
        token + sub_length,
        old_length - sub_length - delta);

    memset(new_string + old_length - sub_length + rep_length, 0, 1);

    head = new_string + delta + rep_length;
    free(old_string);
  }
  return new_string;
}

char *
escape(const char *string)
{
  return str_replace_all(string, "'", "''");
}
