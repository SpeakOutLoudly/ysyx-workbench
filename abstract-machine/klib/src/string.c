#include <klib.h>
#include <klib-macros.h>
#include <stdint.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char *s) {
  size_t t = 0;
  while(s[t] != '\0'){ t++; }
  return t;
}

char *strcpy(char *dst, const char *src) {
  char *p = dst;
  while (*src != '\0') {
    *p = *src;
    p++;
    src++;
  }
  *p = '\0';
  return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
  size_t t = 0;
  while (t < n && src[t] != '\0') {
    dst[t] = src[t];
    t++;
  }
  // 源字符串不足 n 个字符时，剩余位置全部补零。
  while (t < n) {
    dst[t] = '\0';
    t++;
  }
  return dst;
}

char *strcat(char *dst, const char *src) {
  char *tail = dst;
  while (*tail != '\0') {
    tail++;
  }
  while (*src != '\0') {
    *tail = *src;
    tail++;
    src++;
  }
  *tail = '\0';
  return dst;
}

int strcmp(const char *s1, const char *s2) {
  while (*s1 != '\0' && *s1 == *s2) {
    s1++;
    s2++;
  }
  return (unsigned char)*s1 - (unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
  for (size_t t = 0; t < n; t++) {
    unsigned char c1 = (unsigned char)s1[t];
    unsigned char c2 = (unsigned char)s2[t];
    if (c1 != c2) {
      return c1 - c2;
    }
    if (c1 == '\0') {
      return 0;
    }
  }
  return 0;
}

void *memset(void *s, int c, size_t n) {
  // 这里将 void * 转换成 unsigned char * 因为后者表示一个原始字节
  unsigned char *p = s;
  size_t t = 0;
  while(t < n){
    *p = (unsigned char)c;
    p++;
    t++;
  }
  return s;
}

void *memmove(void *dst, const void *src, size_t n) {
  unsigned char *d = dst;
  const unsigned char *s = src;
  if ((uintptr_t)d < (uintptr_t)s) {
    for (size_t t = 0; t < n; t++) {
      d[t] = s[t];
    }
  } else {
    // 从后往前复制，避免覆盖尚未读取的源数据。
    while (n > 0) {
      n--;
      d[n] = s[n];
    }
  }
  return dst;
}

void *memcpy(void *out, const void *in, size_t n) {
  unsigned char *d = out;
  const unsigned char *s = in;
  for (size_t t = 0; t < n; t++) {
    d[t] = s[t];
  }
  return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
  const unsigned char *p1 = s1;
  const unsigned char *p2 = s2;
  for (size_t t = 0; t < n; t++) {
    if (p1[t] != p2[t]) {
      return (int)p1[t] - (int)p2[t];
    }
  }
  return 0;
}

#endif
