#include "../include/env.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  const char *pattern;
  const char *level;
  const char *color;
  const char *msg;
} DeprecatedPattern;

static const DeprecatedPattern patterns[] = {
    {"windowrulev2", "DEPRECATED", C_YELLOW,
     "en v0.53+ se unificó a 'windowrule'"},
    {"= yes", "ERROR", C_RED, "Hyprland exige 'true/false', no 'yes/no'"},
    {"= no", "ERROR", C_RED, "Hyprland exige 'true/false', no 'yes/no'"},
    {"noblur", "DEPRECATED", C_YELLOW, "'noblur' cambió a 'no_blur'"},
    {"new_window_takes_over_fullscreen", "REMOVED", C_RED,
     "ahora es 'on_focus_under_fullscreen'"},
    {"idleinhibit", "REMOVED", C_RED, "ya no va en el .conf, usá hypridle"},
    {"disable_hyprland_qtutils_check", "CHANGED", C_YELLOW,
     "cambió a 'disable_hyprland_guiutils_check'"},
};

#define N_PATTERNS (sizeof(patterns) / sizeof(patterns[0]))

static char *str_replace(const char *orig, const char *rep, const char *with) {
  if (!orig || !rep || strlen(rep) == 0)
    return NULL;
  if (!with)
    with = "";

  int len_rep = strlen(rep);
  int len_with = strlen(with);
  int count = 0;

  const char *ins = orig;
  const char *tmp;
  while ((tmp = strstr(ins, rep))) {
    ins = tmp + len_rep;
    count++;
  }

  char *result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);
  if (!result)
    return NULL;

  char *out = result;
  while (count--) {
    const char *match = strstr(orig, rep);
    int len_front = match - orig;
    memcpy(out, orig, len_front);
    out += len_front;
    memcpy(out, with, len_with);
    out += len_with;
    orig = match + len_rep;
  }
  strcpy(out, orig);
  return result;
}

static int apply(char **target, const char *rep, const char *with) {
  char *result = str_replace(*target, rep, with);
  if (!result)
    return 0;
  free(*target);
  *target = result;
  return 1;
}

int parser(void) {
  char *home = getenv("HOME");
  if (!home) {
    fprintf(stderr, C_RED "[ERROR]" C_RESET " no se pudo obtener $HOME\n");
    return 1;
  }

  char path[512], tmp_path[512];
  snprintf(path, sizeof(path), "%s/.config/hypr/hyprland.conf", home);
  snprintf(tmp_path, sizeof(tmp_path), "%s/.config/hypr/hyprland.conf.tmp",
           home);

  FILE *fp = fopen(path, "r");
  if (!fp) {
    printf("  " C_RED "[ERROR]" C_RESET " no se pudo abrir %s\n", path);
    return 1;
  }

  printf(C_WHITE "\n[CONFIG CHECK]" C_RESET "\n");

  char line[2048];
  int lineno = 1;
  int matches = 0;

  while (fgets(line, sizeof(line), fp)) {
    char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '#') {
      lineno++;
      continue;
    }

    for (size_t i = 0; i < N_PATTERNS; i++) {
      if (strstr(line, patterns[i].pattern)) {
        printf("  %s[%s]" C_RESET " linea %d: '%s' — %s\n", patterns[i].color,
               patterns[i].level, lineno, patterns[i].pattern, patterns[i].msg);
        matches++;
      }
    }
    lineno++;
  }

  if (matches == 0)
    printf("  " C_GREEN "[OK]" C_RESET
           " sin problemas detectados en hyprland.conf\n");

  fseek(fp, 0, SEEK_END);
  long fsize = ftell(fp);
  rewind(fp);

  char *buf = malloc(fsize + 1);
  if (!buf) {
    fclose(fp);
    return 1;
  }

  fread(buf, 1, fsize, fp);
  buf[fsize] = '\0';
  fclose(fp);

  if (!apply(&buf, "windowrulev2", "windowrule") ||
      !apply(&buf, "= yes", "= true") || !apply(&buf, "= no", "= false") ||
      !apply(&buf, "noblur", "no_blur") ||
      !apply(&buf, "disable_hyprland_qtutils_check",
             "disable_hyprland_guiutils_check")) {
    free(buf);
    return 1;
  }

  FILE *f_tmp = fopen(tmp_path, "w");
  if (!f_tmp) {
    printf("  " C_RED "[ERROR]" C_RESET " no se pudo escribir %s\n", tmp_path);
    free(buf);
    return 1;
  }

  fputs(buf, f_tmp);
  fclose(f_tmp);
  free(buf);

  printf("  " C_GREEN "[OK]" C_RESET " config procesada en: %s\n", tmp_path);
  return 0;
}
