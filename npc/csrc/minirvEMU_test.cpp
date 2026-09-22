#include "defines.h"
#include "minirvEMU.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>

bool load_img(uint8_t pmem[], size_t &nread, const char *filename);

static uint8_t image[PMEM_SIZE] = {};

int main(int argc, char **argv) {
  if (argc != 2) {
    std::fprintf(stderr, "Usage: %s IMAGE.bin\n", argv[0]);
    return EXIT_FAILURE;
  }

  size_t image_size = 0;
  load_img(image, image_size, argv[1]);
  minirv::Emulator ref;
  ref.reset(image, image_size);

  constexpr uint64_t kMaxSteps = 10000000;
  for (uint64_t count = 0; count < kMaxSteps; ++count) {
    const minirv::StepResult step = ref.step(count / 100);
    if (!step.ok) {
      std::fprintf(stderr, "minirvEMU error at pc=0x%08x inst=0x%08x: %s\n",
                   step.pc, step.inst, step.error.c_str());
      return EXIT_FAILURE;
    }
    if (step.halted) {
      const int32_t code = static_cast<int32_t>(ref.reg(10));
      std::printf("%s (a0=%d, instructions=%llu)\n",
                  code == 0 ? "HIT GOOD TRAP" : "HIT BAD TRAP", code,
                  static_cast<unsigned long long>(count));
      return code == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
    }
    if (step.store && step.store_address == minirv::kUartAddress)
      std::fputc(step.store_data & 0xffu, stderr);
  }
  std::fprintf(stderr, "minirvEMU timeout after %llu instructions\n",
               static_cast<unsigned long long>(kMaxSteps));
  return EXIT_FAILURE;
}
