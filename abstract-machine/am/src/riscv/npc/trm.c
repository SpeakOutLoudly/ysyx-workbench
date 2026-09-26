#include <am.h>
#include <klib-macros.h>

extern char _heap_start;
int main(const char *args);

extern char _pmem_start;
#define PMEM_SIZE (128 * 1024 * 1024)
#define PMEM_END  ((uintptr_t)&_pmem_start + PMEM_SIZE)

Area heap = RANGE(&_heap_start, PMEM_END);
static const char mainargs[MAINARGS_MAX_LEN] = MAINARGS_PLACEHOLDER; // defined in CFLAGS
static volatile uint8_t *const uart = (volatile uint8_t *)0x10000000u;

static void uart_init(void) {
  uart[3] = 0x83;  // LCR：8N1，DLAB=1
  uart[1] = 0x00;  // DLM：分频值高 8 位
  uart[0] = 0x0e;  // DLL：分频值低 8 位
  uart[3] = 0x03;  // LCR：8N1，DLAB=0
}

void putch(char ch) {
  while((uart[5] & 0x20) == 0) { }
  uart[0] = (uint8_t)ch;      // 传输数据到 THR
}

void halt(int code) {
  asm volatile(
    "mv a0, %0;  ebreak" 
    : 
    : "r"(code));
  while (1);
}

void _trm_init() {
  uart_init();

  int ret = main(mainargs);
  halt(ret);
}
