#include "common.h"

uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod) {
  uint64_t result = 0;
  a = a % mod;
  while (b > 0) {
    if (b % 2 == 1)
      result = (result + a) % mod;
    a = (a * 2) % mod;
    b /= 2;
  }
  return result % mod;
}
bool ConvertStringToUI64(const char *str, uint64_t *val) {
  char *end = NULL;
  errno = 0;
  unsigned long long i = strtoull(str, &end, 10);
  if (errno == ERANGE) {
    fprintf(stderr, "Out of uint64_t range: %s\n", str);
    return false;
  }
  if (errno != 0)
    return false;
  *val = i;
  return true;
}
uint64_t Factorial(const struct FactorialArgs *args) {
  uint64_t ans = 1;
  for (uint64_t i = args->begin; i <= args->end; i++) {
    ans = MultModulo(ans, i, args->mod);
  }
  printf("Factorial range %llu-%llu mod %llu = %llu\n", 
         (unsigned long long)args->begin, 
         (unsigned long long)args->end,
         (unsigned long long)args->mod,
         (unsigned long long)ans);
  
  return ans;
}