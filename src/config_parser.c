#include "../include/env.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *pattern;
  const char *replacement;
  const char *level;
  const char *color;
  const char *msg;
} DeprecatedPattern;

static const DeprecatedPattern patterns[] = {
    {"windowrulev2", "windowrule", "DEPRECATED", C_YELLOW,
     "v0.53+ unificó a 'windowrule'"},
    {"= yes", "= true", "ERROR", C_RED, "Usá 'true' en lugar de 'yes'"},
    {"= no", "= false", "ERROR", C_RED, "Usá 'false' en lugar de 'no'"},
    {"noblur", "no_blur", "DEPRECATED", C_YELLOW, "Cambió a 'no_blur'"},
    {"togglesplit", "layoutmsg, togglesplit", "REMOVED", C_RED,
     "Dispatcher removido en v0.54"},
    {"swapsplit", "layoutmsg, swapsplit", "REMOVED", C_RED,
     "Dispatcher removido en v0.54"},
    {"new_window_takes_over_fullscreen", "on_focus_under_fullscreen", "REMOVED",
     C_RED, "Cambió de nombre en v0.54"},
    {"disable_hyprland_qtutils_check", "disable_hyprland_guiutils_check",
     "CHANGED", C_YELLOW, "Renombrado en v0.54"}};

#define N_PATTERNS (sizeof(patterns) / sizeof(patterns[0]))

// Función auxiliar para verificar si una línea es un comentario o está vacía
int is_useless(const char *line) {
  while (*line && isspace(*line))
    line++;
  return (*line == '#' || *line == '\0');
}

// Reemplazo simple de una ocurrencia en una línea
void patch_line(char *line, const DeprecatedPattern *p, int lineno) {
  char *pos = strstr(line, p->pattern);
  if (pos) {
    printf("  %s[%s]" C_RESET " linea %d: '%s' -> %s\n", p->color, p->level,
           lineno, p->pattern, p->msg);

    // Construimos la nueva línea
    char new_line[2048] = {0};
    int prefix_len = pos - line;

    strncpy(new_line, line, prefix_len);
    strcat(new_line, p->replacement);
    strcat(new_line, pos + strlen(p->pattern));

    strcpy(line, new_line);
  }
}

int parser(void) {
  char *home = getenv("HOME");
  if (!home)
    return 1;

  char path[512], tmp_path[512];
  snprintf(path, sizeof(path), "%s/.config/hypr/hyprland.conf", home);
  snprintf(tmp_path, sizeof(tmp_path), "%s/.config/hypr/hyprland.conf.tmp",
           home);

  FILE *in = fopen(path, "r");
  if (!in) {
    fprintf(stderr, "  " C_RED "[ERROR]" C_RESET " No se abrió %s\n", path);
    return 1;
  }

  FILE *out = fopen(tmp_path, "w");
  if (!out) {
    fclose(in);
    return 1;
  }

  printf(C_WHITE "\n[HYPR-DOCTOR: CONFIG CHECK v0.54]" C_RESET "\n");

  char line[2048];
  int lineno = 1;
  int total_matches = 0;

  while (fgets(line, sizeof(line), in)) {
    if (!is_useless(line)) {
      for (size_t i = 0; i < N_PATTERNS; i++) {
        if (strstr(line, patterns[i].pattern)) {
          patch_line(line, &patterns[i], lineno);
          total_matches++;
        }
      }
    }
    fputs(line, out);
    lineno++;
  }

  fclose(in);
  fclose(out);

  if (total_matches == 0) {
    printf("  " C_GREEN "[OK]" C_RESET
           " Tu config está limpia para la v0.54\n");
    remove(tmp_path); // No necesitamos el tmp si no hubo cambios
  } else {
    printf("\n  " C_CYAN "[INFO]" C_RESET " Se generó una config corregida en: %s\n",
           tmp_path);
    printf("  Revisala y reemplazá tu config original si los cambios son "
           "correctos.\n");
  }

  return 0;
}
