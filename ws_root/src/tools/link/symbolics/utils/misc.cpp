// #ifdef _NEW_GCC
char *strchr(char *s, int c) {
	if (!s) return 0;
	while (*s) {
		if (*s == c) return s;
		s++;
		}
	return c==0?s:0;
	}

char *strrchr(char *s, int c) {
	if (!s) return 0;
	char *p = s;
	while (*s) {
		if (*s == c) {
			p = s;
			}
		s++;
		}
	if (p == s && *p != c) return 0;
	return c==0?p:0;
	}
// #endif
