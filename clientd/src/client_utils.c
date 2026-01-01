#include "client_utils.h"
#include <string.h>
#include <ctype.h>

void trim(char *s) {
    if (!s) return;
    char *p = s;
    int l = strlen(p);
    // Cat khoang trang cuoi chuoi
    while(l > 0 && isspace(p[l - 1])) p[--l] = 0;
    // Cat khoang trang dau chuoi
    while(*p && isspace(*p)) ++p, --l;
    memmove(s, p, l + 1);
}