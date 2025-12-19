/* A simple utility that fails if the user is signed in more than once. */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <utmpx.h>

int main(void) {
  const char *username = getlogin();
  if (username == NULL) {
    return EXIT_FAILURE;
  }

  setutxent();

  struct utmpx *entry;
  bool found_session = false;

  while ((entry = getutxent()) != NULL) {
    if (entry->ut_type == USER_PROCESS &&
        strncmp(entry->ut_line, "pts", 3) == 0 &&
        strcmp(entry->ut_user, username) == 0) {
      if (found_session) {
        endutxent();
        return EXIT_FAILURE;
      }
      found_session = true;
    }
  }

  endutxent();
}

