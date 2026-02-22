#include "../include/env.h"
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static void get_hypr_conf_path(char *out, size_t sz) {
  const char *home = getenv("HOME");
  if (!home) {
    struct passwd *pw = getpwuid(getuid());
    home = pw ? pw->pw_dir : "/root";
  }
  snprintf(out, sz, "%s/.config/hypr/hyprland.conf", home);
}

static int search_in_file(const char *filepath, const char *key, char *out_val,
                          size_t out_sz) {
  FILE *fp = fopen(filepath, "r");
  if (!fp)
    return 0;

  char line[1024];
  size_t keylen = strlen(key);
  int found = 0;

  while (fgets(line, sizeof(line), fp)) {
    char *p = line;
    while (*p == ' ' || *p == '\t')
      p++;
    if (*p == '#' || *p == '\n' || *p == '\0')
      continue;

    if (strncmp(p, "env", 3) == 0) {
      p += 3;
      while (*p == ' ' || *p == '\t')
        p++;
      if (*p == '=')
        p++;
      while (*p == ' ' || *p == '\t')
        p++;
    }

    if (strncmp(p, key, keylen) == 0 && p[keylen] == '=') {
      char *val = p + keylen + 1;
      val[strcspn(val, "\n")] = '\0';
      if (out_val)
        snprintf(out_val, out_sz, "%s", val);
      found = 1;
      break;
    }
  }

  fclose(fp);
  return found;
}

static EnvResult find_env(const char *key) {
  EnvResult r = {0};
  char hypr_path[512];
  get_hypr_conf_path(hypr_path, sizeof(hypr_path));

  char val[256];

  const char *pval = getenv(key);
  if (pval) {
    r.sources |= SRC_PROCESS;
    snprintf(r.value, sizeof(r.value), "%s", pval);
  }

  if (search_in_file("/etc/environment", key, val, sizeof(val))) {
    r.sources |= SRC_ETC;
    if (!r.value[0])
      snprintf(r.value, sizeof(r.value), "%s", val);
  }

  if (search_in_file(hypr_path, key, val, sizeof(val))) {
    r.sources |= SRC_HYPR;
    if (!r.value[0])
      snprintf(r.value, sizeof(r.value), "%s", val);
  }

  return r;
}

static void print_sources(int sources) {
  printf("        fuentes:");
  if (sources & SRC_PROCESS)
    printf(" [proceso]");
  if (sources & SRC_ETC)
    printf(" [/etc/environment]");
  if (sources & SRC_HYPR)
    printf(" [hyprland.conf]");
  printf("\n");
}

static int value_ok(const char *val, const EnvValidator *v) {
  if (!v)
    return 1;
  if (v->exact && strcmp(val, v->exact) == 0)
    return 1;
  if (v->prefix && strncmp(val, v->prefix, strlen(v->prefix)) == 0)
    return 1;
  if (v->accepted) {
    for (int i = 0; i < v->accepted_len; i++)
      if (strcmp(val, v->accepted[i]) == 0)
        return 1;
  }
  return 0;
}

static void check_var(const char *key, const EnvValidator *v,
                      const char *severity, const char *hint) {
  EnvResult r = find_env(key);
  const char *label = strcmp(severity, C_RED) == 0 ? "ERROR" : "WARNING";

  if (r.sources == SRC_NONE) {
    printf("  %s[%s]" C_RESET " %s no definida\n", severity, label, key);
    if (hint)
      printf("        %s\n", hint);
    return;
  }

  if (!value_ok(r.value, v)) {
    printf("  " C_RED "[ERROR]" C_RESET " %s=%s\n", key, r.value);
    print_sources(r.sources);
    if (hint)
      printf("        %s\n", hint);
    return;
  }

  printf("  " C_GREEN "[OK]" C_RESET " %s=%s\n", key, r.value);

  if (__builtin_popcount(r.sources) > 1) {
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " %s definida en multiples fuentes\n",
           key);
    print_sources(r.sources);
  }
}

static int portal_installed(const char *name) {
  char path[512];

  snprintf(path, sizeof(path),
           "/usr/share/xdg-desktop-portal/portals/%s.portal", name);
  FILE *fp = fopen(path, "r");
  if (fp) {
    fclose(fp);
    return 1;
  }

  snprintf(path, sizeof(path), "/usr/lib/xdg-desktop-portal-%s", name);
  fp = fopen(path, "r");
  if (fp) {
    fclose(fp);
    return 1;
  }

  return 0;
}

static void section_nvidia(void) {
  printf("\n  " C_WHITE "[ENV — NVIDIA]" C_RESET "\n");

  check_var("LIBVA_DRIVER_NAME", &(EnvValidator){.exact = "nvidia"}, C_RED,
            "necesario para VA-API con NVIDIA");

  check_var("__GLX_VENDOR_LIBRARY_NAME", &(EnvValidator){.exact = "nvidia"},
            C_RED, "fuerza GLX a usar el vendor nvidia");

  check_var("GBM_BACKEND", &(EnvValidator){.exact = "nvidia-drm"}, C_RED,
            "backend GBM para Wayland con NVIDIA");

  check_var("__NV_PRIME_RENDER_OFFLOAD", &(EnvValidator){.exact = "1"},
            C_YELLOW, "solo en laptops hibridas PRIME");
}

static void section_xdg(void) {
  printf("\n  " C_WHITE "[ENV — XDG]" C_RESET "\n");

  static const char *desktop_vals[] = {"Hyprland", "hyprland"};
  check_var("XDG_CURRENT_DESKTOP",
            &(EnvValidator){.accepted = desktop_vals, .accepted_len = 2}, C_RED,
            "los portales no saben que DE corre sin esto");

  check_var("XDG_SESSION_TYPE", &(EnvValidator){.exact = "wayland"}, C_RED,
            "debe ser 'wayland', no 'x11'");

  static const char *session_vals[] = {"Hyprland", "hyprland"};
  check_var("XDG_SESSION_DESKTOP",
            &(EnvValidator){.accepted = session_vals, .accepted_len = 2},
            C_YELLOW, "fallback de XDG_CURRENT_DESKTOP");

  printf("\n  " C_WHITE "[XDG PORTALS]" C_RESET "\n");

  int hyprland = portal_installed("hyprland");
  int gnome = portal_installed("gnome");
  int kde = portal_installed("kde");
  int gtk = portal_installed("gtk");
  int wlr = portal_installed("wlr");

  if (hyprland)
    printf("  " C_GREEN "[OK]" C_RESET
           " xdg-desktop-portal-hyprland instalado\n");
  else
    printf("  " C_RED "[ERROR]" C_RESET
           " xdg-desktop-portal-hyprland no encontrado\n"
           "         instala xdg-desktop-portal-hyprland\n");

  if (hyprland && gnome)
    printf("  " C_RED "[CRITICAL]" C_RESET
           " portal GNOME + Hyprland instalados — GNOME toma precedencia\n"
           "            desinstala xdg-desktop-portal-gnome\n");

  if (hyprland && kde)
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " portal KDE instalado junto a Hyprland\n");

  if (hyprland && wlr)
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " xdg-desktop-portal-wlr es el predecesor — puede generar "
           "duplicados\n");

  if (gtk && !hyprland)
    printf("  " C_YELLOW "[WARNING]" C_RESET
           " solo portal GTK — screen sharing probablemente no funciona\n");

  if (!hyprland && !gnome && !kde && !gtk && !wlr)
    printf("  " C_RED "[ERROR]" C_RESET " ningun portal XDG detectado\n");
}

static void section_community(void) {
  printf("\n  " C_WHITE "[ENV — WAYLAND]" C_RESET "\n");

  static const char *electron_vals[] = {"auto", "wayland"};
  check_var("ELECTRON_OZONE_PLATFORM_HINT",
            &(EnvValidator){.accepted = electron_vals, .accepted_len = 2},
            C_YELLOW, "Discord/VSCode borrosos en XWayland sin esto");

  check_var("MOZ_ENABLE_WAYLAND", &(EnvValidator){.exact = "1"}, C_YELLOW,
            "Firefox nativo en Wayland");

  check_var("QT_QPA_PLATFORM", &(EnvValidator){.prefix = "wayland"}, C_YELLOW,
            "apps Qt corren en XWayland sin esto");

  check_var("SDL_VIDEODRIVER", &(EnvValidator){.prefix = "wayland"}, C_YELLOW,
            "juegos SDL2 en XWayland sin esto");

  check_var("CLUTTER_BACKEND", &(EnvValidator){.exact = "wayland"}, C_YELLOW,
            "apps GTK3/Clutter sin esto pueden fallar");
}

void check_env(int nvidia_detected) {
  printf(C_WHITE "\n[ENV CHECK]" C_RESET "\n");

  if (nvidia_detected)
    section_nvidia();

  section_xdg();
  section_community();
}
