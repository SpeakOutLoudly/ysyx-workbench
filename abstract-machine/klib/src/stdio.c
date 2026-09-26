#include <am.h>
#include <klib.h>
#include <klib-macros.h>
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

int printf(const char *fmt, ...) {
  panic("Not implemented");
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  panic("Not implemented");
}

int sprintf(char *out, const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  char *p = out;

  while (*fmt != '\0') {
    if (*fmt != '%') {
      *p++ = *fmt++;
      continue;
    }

    fmt++;
    switch (*fmt) {
      case 's': {
        const char *s = va_arg(ap, const char *);
        while (*s != '\0') {
          *p++ = *s++;
        }
        break;
      }
      case 'c':
        *p++ = (char)va_arg(ap, int);
        break;
      case '%':
        *p++ = '%';
        break;
      case 'd':
      case 'i':
      case 'u':
      case 'o':
      case 'x':
      case 'X': {
        unsigned int value;
        unsigned int base = 10;
        if (*fmt == 'd' || *fmt == 'i') {
          int number = va_arg(ap, int);
          value = (unsigned int)number;
          if (number < 0) {
            *p++ = '-';
            // 用无符号运算取绝对值，避免 -INT_MIN 溢出。
            value = 0u - value;
          }
        } else {
          value = va_arg(ap, unsigned int);
          if (*fmt == 'o') base = 8;
          if (*fmt == 'x' || *fmt == 'X') base = 16;
        }

        const char *digits = (*fmt == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
        char buf[sizeof(unsigned int) * 8];
        size_t count = 0;
        // 先从低位提取数字，再反向写入输出缓冲区。
        do {
          buf[count++] = digits[value % base];
          value /= base;
        } while (value != 0);
        while (count > 0) {
          *p++ = buf[--count];
        }
        break;
      }
      default:
        panic("Unsupported sprintf format");
    }
    fmt++;
  }

  *p = '\0';
  va_end(ap);
  return (int)(p - out);
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  panic("Not implemented");
}

int vsnprintf(char *out, size_t n, const char *fmt, va_list ap) {
  panic("Not implemented");
}

#endif
