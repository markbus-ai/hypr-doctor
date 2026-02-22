#include "../include/env.h"
#include <ctype.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

static int read_comm(const char *pid, char *out, size_t sz) {
  char path[64];
  snprintf(path, sizeof(path), "/proc/%s/comm", pid);

  FILE *fp = fopen(path, "r");
  if (!fp)
    return 0;

  if (!fgets(out, sz, fp)) {
    fclose(fp);
    return 0;
  }
  out[strcspn(out, "\n")] = '\0';

  fclose(fp);
  return 1;
}

int is_running(const char *proc_name) {
  DIR *dir = opendir("/proc");
  if (!dir)
    return 0;

  struct dirent *entry;
  char comm[256];

  while ((entry = readdir(dir)) != NULL) {
    if (!isdigit(entry->d_name[0]))
      continue;
    if (!read_comm(entry->d_name, comm, sizeof(comm)))
      continue;

    // comm trunca a 15 chars
    if (strncmp(comm, proc_name, 15) != 0)
      continue;

    // match exacto
    if (strcmp(comm, proc_name) == 0) {
      closedir(dir);
      return 1;
    }

    // comm truncado: verificar cmdline completo
    char cmdline_path[64];
    snprintf(cmdline_path, sizeof(cmdline_path), "/proc/%s/cmdline",
             entry->d_name);

    FILE *fp = fopen(cmdline_path, "r");
    if (!fp)
      continue;

    char cmdline[2048] = {0};
    fread(cmdline, 1, sizeof(cmdline) - 1, fp);
    fclose(fp);

    char *base = strrchr(cmdline, '/');
    base = base ? base + 1 : cmdline;

    if (strcmp(base, proc_name) == 0) {
      closedir(dir);
      return 1;
    }
  }

  closedir(dir);
  return 0;
}

static int dbus_ok(void) {
  struct stat st;
  return stat("/run/dbus/system_bus_socket", &st) == 0;
}

typedef enum {
  SEV_OK,
  SEV_ERROR,
  SEV_WARNING,
  SEV_CONFLICT,
} Severity;

typedef struct {
  const char *name;
  Severity severity;
  const char *msg_present;
  const char *msg_absent;
} ProcCheck;

static const ProcCheck general_procs[] = {
    {"pipewire", SEV_ERROR, NULL,
     "pipewire no corre — audio y screenshare van a fallar"},
    {"wireplumber", SEV_ERROR, NULL,
     "wireplumber no corre — session manager de pipewire ausente"},
    {"xdg-desktop-portal", SEV_ERROR, NULL,
     "xdg-desktop-portal base no corre — file picker y screenshare rotos"},
    {"xdg-desktop-portal-hyprland", SEV_ERROR, NULL,
     "xdg-desktop-portal-hyprland no corre — screenshare no va a funcionar"},
    {"xdg-desktop-portal-gnome", SEV_CONFLICT,
     "xdg-desktop-portal-gnome corriendo — va a tomar precedencia sobre "
     "hyprland",
     NULL},
    {"xdg-desktop-portal-kde", SEV_CONFLICT,
     "xdg-desktop-portal-kde corriendo junto a hyprland — posible conflicto en "
     "file picker",
     NULL},
    {"xdg-desktop-portal-wlr", SEV_CONFLICT,
     "xdg-desktop-portal-wlr corriendo — predecesor de hyprland, puede generar "
     "duplicados",
     NULL},
};

static const ProcCheck nvidia_procs[] = {
    {"nvidia-persistenced", SEV_WARNING, NULL,
     "nvidia-persistenced no corre — puede causar delays al iniciar apps GPU"},
};

static void report(const ProcCheck *p, int running) {
  if (p->severity == SEV_CONFLICT) {
    if (running)
      printf("  " C_RED "[CRITICAL]" C_RESET " %s\n", p->msg_present);
    return;
  }

  if (running) {
    printf("  " C_GREEN "[OK]" C_RESET " %s corriendo\n", p->name);
    return;
  }

  switch (p->severity) {
  case SEV_ERROR:
    printf("  " C_RED "[ERROR]" C_RESET " %s\n", p->msg_absent);
    break;
  case SEV_WARNING:
    printf("  " C_YELLOW "[WARNING]" C_RESET " %s\n", p->msg_absent);
    break;
  default:
    break;
  }
}

void check_procs(int nvidia_detected) {
  printf(C_WHITE "\n[PROC CHECK]" C_RESET "\n");

  if (dbus_ok())
    printf("  " C_GREEN "[OK]" C_RESET
           " dbus corriendo (/run/dbus/system_bus_socket)\n");
  else
    printf("  " C_RED "[ERROR]" C_RESET
           " dbus socket no encontrado — la mayoria de apps van a fallar\n");

  size_t n = sizeof(general_procs) / sizeof(general_procs[0]);
  for (size_t i = 0; i < n; i++)
    report(&general_procs[i], is_running(general_procs[i].name));

  if (nvidia_detected) {
    printf("\n  " C_WHITE "[PROC — NVIDIA]" C_RESET "\n");
    size_t nn = sizeof(nvidia_procs) / sizeof(nvidia_procs[0]);
    for (size_t i = 0; i < nn; i++)
      report(&nvidia_procs[i], is_running(nvidia_procs[i].name));
  }
}
