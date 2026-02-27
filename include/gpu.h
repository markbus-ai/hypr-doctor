#ifndef GPU_H
#define GPU_H
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

typedef struct {
  char key[64];
  char value[64];
} env_var;
typedef struct {
  int nvidia;
  int nvidia_drm;
  int nvidia_modeset;
  int nouveau;
  int amdgpu;
  int i915;
  int xe;
} GpuModules;
int check_gpu(void);

#endif
