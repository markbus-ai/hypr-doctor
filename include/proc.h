#ifndef PROC_H
#define PROC_H
#include <stdio.h>
#define C_RESET "\033[0m"
#define C_RED "\033[1;31m"
#define C_GREEN "\033[1;32m"
#define C_YELLOW "\033[1;33m"
#define C_BLUE "\033[1;34m"
#define C_WHITE "\033[1;37m"
#define C_CYAN "\033[1;36m"
#define C_MAGENTA "\033[1;35m"
#define C_BOLD "\033[1m"
typedef enum {
  SEV_OK,       // su presencia es buena
  SEV_ERROR,    // su ausencia es un error
  SEV_WARNING,  // su ausencia es un warning
  SEV_CONFLICT, // su presencia es un problema
} Severity;

typedef struct {
  const char *name;
  Severity severity;
  const char *msg_present;
  const char *msg_absent;
} ProcCheck;

int is_running(const char *proc_name);
void check_procs(int nvidia_detected);
#endif
