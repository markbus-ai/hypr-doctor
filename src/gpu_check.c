#include "../include/gpu.h"
#include <stdio.h>
#include <string.h>

static int parse_modules(GpuModules *m) {
  FILE *fp = fopen("/proc/modules", "r");
  if (!fp) {
    printf(C_RED "[ERROR]" C_RESET " no se pudo abrir /proc/modules\n");
    return 0;
  }

  char line[1024];
  char name[256];

  while (fgets(line, sizeof(line), fp)) {
    sscanf(line, "%255s", name);
    if (strcmp(name, "nvidia") == 0)
      m->nvidia = 1;
    else if (strcmp(name, "nvidia_drm") == 0)
      m->nvidia_drm = 1;
    else if (strcmp(name, "nvidia_modeset") == 0)
      m->nvidia_modeset = 1;
    else if (strcmp(name, "nouveau") == 0)
      m->nouveau = 1;
    else if (strcmp(name, "amdgpu") == 0)
      m->amdgpu = 1;
    else if (strcmp(name, "i915") == 0)
      m->i915 = 1;
    else if (strcmp(name, "xe") == 0)
      m->xe = 1;
  }

  fclose(fp);
  return 1;
}

static int read_nvidia_drm_modeset(void) {
  FILE *fp = fopen("/sys/module/nvidia_drm/parameters/modeset", "r");
  if (!fp)
    return -1;

  char val = 0;
  fscanf(fp, "%c", &val);
  fclose(fp);

  if (val == 'Y' || val == 'y' || val == '1')
    return 1;
  if (val == 'N' || val == 'n' || val == '0')
    return 0;
  return -1;
}

static void check_nvidia(const GpuModules *m) {
  printf("  " C_BLUE "[NVIDIA]" C_RESET "\n");

  if (m->nouveau) {
    printf("  " C_RED "[CRITICAL]" C_RESET
           " nvidia + nouveau cargados al mismo tiempo\n");
    printf("             blacklist nouveau en /etc/modprobe.d/\n");
    return;
  }

  if (!m->nvidia_drm)
    printf("  " C_RED "[ERROR]" C_RESET
           " nvidia_drm faltante — critico para Wayland\n");

  if (!m->nvidia_modeset)
    printf("  " C_RED "[ERROR]" C_RESET
           " nvidia_modeset faltante — necesario para resoluciones\n");

  if (m->nvidia_drm) {
    int modeset = read_nvidia_drm_modeset();
    if (modeset == 1)
      printf("  " C_GREEN "[OK]" C_RESET " nvidia_drm.modeset=Y\n");
    else if (modeset == 0) {
      printf("  " C_RED "[ERROR]" C_RESET " nvidia_drm.modeset=N\n");
      printf("             agregá nvidia_drm.modeset=1 a los parametros del "
             "kernel\n");
    } else
      printf("  " C_YELLOW "[WARNING]" C_RESET
             " no se pudo leer /sys/module/nvidia_drm/parameters/modeset\n");
  }

  if (m->nvidia && m->nvidia_drm && m->nvidia_modeset)
    printf("  " C_GREEN "[OK]" C_RESET
           " stack completo (nvidia + nvidia_drm + nvidia_modeset)\n");
}

static void check_nouveau_solo(void) {
  printf("  " C_BLUE "[NVIDIA]" C_RESET "\n");
  printf("  " C_YELLOW "[WARNING]" C_RESET
         " solo nouveau — no soporta Hyprland\n");
  printf("            instala los drivers oficiales de NVIDIA\n");
}

static void check_amd(void) {
  printf("  " C_BLUE "[AMD]" C_RESET "\n");
  printf("  " C_GREEN "[OK]" C_RESET
         " amdgpu cargado — soporte Wayland nativo\n");
}

static void check_intel(const GpuModules *m) {
  printf("  " C_BLUE "[INTEL]" C_RESET "\n");

  if (m->i915 && m->xe) {
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " i915 y xe cargados simultaneamente\n");
    return;
  }

  if (m->xe)
    printf("  " C_GREEN "[OK]" C_RESET " xe cargado (12va gen+ / Arc)\n");
  else
    printf("  " C_GREEN "[OK]" C_RESET " i915 cargado\n");
}

int check_gpu(void) {
  GpuModules m = {0};

  if (!parse_modules(&m))
    return 0;

  int hay_nvidia = m.nvidia || m.nvidia_drm || m.nvidia_modeset;
  int hay_nouveau = m.nouveau;
  int hay_amd = m.amdgpu;
  int hay_intel = m.i915 || m.xe;

  if (!hay_nvidia && !hay_nouveau && !hay_amd && !hay_intel) {
    printf(C_RED "[ERROR]" C_RESET " no se detecto ningun driver de GPU\n");
    return 0;
  }

  printf(C_WHITE "[GPU CHECK]" C_RESET "\n");

  if (hay_nvidia)
    check_nvidia(&m);

  else if (hay_nouveau)
    check_nouveau_solo();
  if (hay_amd)
    check_amd();
  if (hay_intel)
    check_intel(&m);

  if ((hay_nvidia || hay_nouveau) + hay_amd + hay_intel > 1)
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " multi-GPU detectada — configurá WLR_DRM_DEVICES\n");
  return hay_nvidia;
}
