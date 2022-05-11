#ifndef LOG_H
#define LOG_H

#define LOGTEMP(...) {printf("Log: "__VA_ARGS__);putchar('\n');}
#define LOGWARN(...) {fprintf(stderr, "\x1b[93mWarning: "__VA_ARGS__);fputs("\x1b[0m\n", stderr);}
#define LOGERROR(...) {fprintf(stderr, "\x1b[31mERROR: "__VA_ARGS__);fputs("\x1b[0m\n", stderr);}

#include <stdio.h>

#endif /* LOG_H */
