#ifndef ENV_H
#define ENV_H

// env
#define SRC_NONE 0
#define SRC_PROCESS (1 << 0)
#define SRC_ETC (1 << 1)
#define SRC_HYPR (1 << 2)

typedef struct {
  int sources;
  char value[256];
} EnvResult;

// colores
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
  const char *exact;     // valor exacto, o NULL
  const char *prefix;    // prefijo aceptado, o NULL
  const char **accepted; // array de valores aceptados, o NULL
  int accepted_len;
} EnvValidator;
// funciones publicas
void check_env(int nvidia_detected);
int parser();
#endif
