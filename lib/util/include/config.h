#ifndef UTIL_INCLUDE_CONFIG_H_
#define UTIL_INCLUDE_CONFIG_H_

// Lee una clave de un .ini. Devuelve 1 si la encontró, 0 si no.
int configGet(const char* rutaIni, const char* clave, char* salida, int maxLen);

#endif /* UTIL_INCLUDE_CONFIG_H_ */
