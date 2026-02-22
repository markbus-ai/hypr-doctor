#include "include/env.h"
#include "include/gpu.h"
#include "include/log.h"
#include "include/proc.h"

#include <stdio.h>

int main(void) {
  printf(C_WHITE "=== Hyprland Doctor ===" C_RESET "\n");

  int nvidia = check_gpu();
  parser();
  check_procs(nvidia);
  check_env(nvidia);
  tail_logs();

  return 0;
}
