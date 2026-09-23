// memory.h
#pragma once
#include <cstdint>

#define ADDR_OFF 0x80000000
#define FETCH_DELAY 3
#define DMEM_DELAY 5

inline constexpr uint32_t PMEM_SIZE = 128u * 1024 * 1024;

struct NpcStoreEvent {
    bool valid = false;
    uint32_t address = 0;
    uint32_t data = 0;
    uint8_t mask = 0;
};

struct MemRequestEvent{
    bool valid = false;
    bool write = false;
    uint32_t address = 0;
    uint32_t wdata = 0;
    uint8_t wmask = 0;
    uint32_t rdata = 0;
    unsigned delay = 0;
    bool resp_valid = false;
};
