#include "../include/env.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define MAX_LINE 2048
#define TAIL_SIZE 50

typedef struct {
  const char *pattern;
  const char *color;
  const char *explanation;
} LogPattern;

static const LogPattern patterns[] = {
    {"[CRITICAL]", C_RED, NULL},
    {"[ERROR]", C_RED, NULL},
    {"[WARN]", C_YELLOW, NULL},
    {"Wayland", C_YELLOW,
     "problema con el protocolo Wayland — revisá nvidia_drm.modeset y "
     "XDG_SESSION_TYPE"},
    {"nvidia", C_YELLOW,
     "problema relacionado a drivers NVIDIA — revisá el stack nvidia con [GPU "
     "CHECK]"},
    {"xdg-desktop-portal", C_YELLOW,
     "portal XDG fallando — revisá que xdg-desktop-portal-hyprland este "
     "corriendo"},
    {"dbus", C_YELLOW,
     "problema con dbus — revisá que dbus-daemon este corriendo"},
    {"No such file", C_RED,
     "archivo o socket no encontrado — puede indicar un servicio que no "
     "arranco"},
    {"Permission denied", C_RED,
     "problema de permisos — revisá grupos (video, input) del usuario"},
    {"Segmentation fault", C_RED,
     "crash de Hyprland — reporta el log completo en "
     "github.com/hyprwm/Hyprland"},
    {"signal 11", C_RED,
     "crash por SIGSEGV — misma causa que Segmentation fault"},
    {"signal 6", C_RED,
     "crash por SIGABRT — plugin o config invalida puede ser la causa"},
};

#define N_PATTERNS (sizeof(patterns) / sizeof(patterns[0]))

// encuentra el log mas nuevo en /tmp/hypr
static int find_latest_log(char *out_path, size_t sz) {
  DIR *dir = opendir("/tmp/hypr");
  if (!dir)
    return 0;

  struct dirent *entry;
  char best[512] = {0};
  time_t best_mtime = 0;

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue;

    char candidate[512];
    snprintf(candidate, sizeof(candidate), "/tmp/hypr/%s/hyprland.log",
             entry->d_name);

    struct stat st;
    if (stat(candidate, &st) != 0)
      continue;

    if (st.st_mtime > best_mtime) {
      best_mtime = st.st_mtime;
      snprintf(best, sizeof(best), "%s", candidate);
    }
  }

  closedir(dir);

  if (!best[0])
    return 0;
  snprintf(out_path, sz, "%s", best);
  return 1;
}

// carga las ultimas lineas
static int load_tail(const char *path, char tail[][MAX_LINE], int *count) {
  FILE *fp = fopen(path, "r");
  if (!fp)
    return 0;

  int idx = 0;
  *count = 0;

  while (fgets(tail[idx % TAIL_SIZE], MAX_LINE, fp)) {
    idx++;
    if (*count < TAIL_SIZE)
      (*count)++;
  }

  fclose(fp);

  // reordenar para que tail[0] sea la linea mas vieja del buffer
  // el buffer circular empieza en idx % TAIL_SIZE si count == TAIL_SIZE
  if (*count == TAIL_SIZE) {
    // rotar in-place usando un buffer temporal
    char tmp[MAX_LINE];
    int start = idx % TAIL_SIZE;
    int n = TAIL_SIZE;
    // algoritmo de rotacion por reversa
    // reversa [0, start-1]
    for (int a = 0, b = start - 1; a < b; a++, b--) {
      memcpy(tmp, tail[a], MAX_LINE);
      memcpy(tail[a], tail[b], MAX_LINE);
      memcpy(tail[b], tmp, MAX_LINE);
    }
    // reversa [start, n-1]
    for (int a = start, b = n - 1; a < b; a++, b--) {
      memcpy(tmp, tail[a], MAX_LINE);
      memcpy(tail[a], tail[b], MAX_LINE);
      memcpy(tail[b], tmp, MAX_LINE);
    }
    // reversa total
    for (int a = 0, b = n - 1; a < b; a++, b--) {
      memcpy(tmp, tail[a], MAX_LINE);
      memcpy(tail[a], tail[b], MAX_LINE);
      memcpy(tail[b], tmp, MAX_LINE);
    }
  }

  return 1;
}

static const LogPattern *match_line(const char *line) {
  for (size_t i = 0; i < N_PATTERNS; i++) {
    if (strstr(line, patterns[i].pattern))
      return &patterns[i];
  }
  return NULL;
}

void tail_logs(void) {
  printf(C_WHITE "\n[LOG CHECK]" C_RESET "\n");

  char log_path[512];
  if (!find_latest_log(log_path, sizeof(log_path))) {
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " no se encontro ningun log en /tmp/hypr/\n");
    printf("            Hyprland nunca corrio o limpio sus temporales\n");
    return;
  }

  printf("  log: %s\n\n", log_path);

  static char tail[TAIL_SIZE][MAX_LINE];
  int count = 0;

  if (!load_tail(log_path, tail, &count)) {
    printf("  " C_RED "[ERROR]" C_RESET " no se pudo leer %s\n", log_path);
    return;
  }

  int matches = 0;

  for (int i = 0; i < count; i++) {
    const LogPattern *p = match_line(tail[i]);
    if (!p)
      continue;

    // strip newline para imprimir prolijo
    char line[MAX_LINE];
    snprintf(line, sizeof(line), "%s", tail[i]);
    line[strcspn(line, "\n")] = '\0';

    printf("  %s>>>" C_RESET " %s\n", p->color, line);
    if (p->explanation)
      printf("       %s\n", p->explanation);

    matches++;
  }

  if (matches == 0)
    printf("  " C_GREEN "[OK]" C_RESET " sin errores detectados en el log\n");
  else
    printf("\n  %d linea(s) con problemas detectadas\n", matches);
}
