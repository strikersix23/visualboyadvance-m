#include "Util.h"

#include <cstring>

#include "core/base/port.h"
#include "core/base/system.h"
#include "core/gba/gbaFilter.h"
#include "core/gba/gbaFlash.h"
#include "core/gba/gbaGlobals.h"
#include "core/gba/gbaRtc.h"

#if defined(_MSC_VER)
#define strcasecmp _stricmp
#endif  // defined(_MSC_VER)

void utilUpdateSystemColorMaps(bool lcd) {
    switch (systemColorDepth) {
        case 16: {
            for (int i = 0; i < 0x10000; i++) {
                systemColorMap16[i] = ((i & 0x1f) << systemRedShift) |
                                      (((i & 0x3e0) >> 5) << systemGreenShift) |
                                      (((i & 0x7c00) >> 10) << systemBlueShift);
            }
            if (lcd)
                gbafilter_pal(systemColorMap16, 0x10000);
        } break;
        case 24:
        case 32: {
            for (int i = 0; i < 0x10000; i++) {
                systemColorMap32[i] = ((i & 0x1f) << systemRedShift) |
                                      (((i & 0x3e0) >> 5) << systemGreenShift) |
                                      (((i & 0x7c00) >> 10) << systemBlueShift);
            }
            if (lcd)
                gbafilter_pal32(systemColorMap32, 0x10000);
        } break;
    }
}

void utilGBAFindSave(const int size) {
    uint32_t* p = (uint32_t*)&g_rom[0];
    uint32_t* end = (uint32_t*)(&g_rom[0] + size);
    int detectedSaveType = 0;
    int flashSize = 0x10000;
    bool rtcFound = false;

    while (p < end) {
        uint32_t d = READ32LE(p);

        if (d == 0x52504545) {
            if (memcmp(p, "EEPROM_", 7) == 0) {
                if (detectedSaveType == 0 || detectedSaveType == 4)
                    detectedSaveType = 1;
            }
        } else if (d == 0x4D415253) {
            if (memcmp(p, "SRAM_", 5) == 0) {
                if (detectedSaveType == 0 || detectedSaveType == 1 || detectedSaveType == 4)
                    detectedSaveType = 2;
            }
        } else if (d == 0x53414C46) {
            if (memcmp(p, "FLASH1M_", 8) == 0) {
                if (detectedSaveType == 0) {
                    detectedSaveType = 3;
                    flashSize = 0x20000;
                }
            } else if (memcmp(p, "FLASH512_", 9) == 0) {
                if (detectedSaveType == 0) {
                    detectedSaveType = 3;
                    flashSize = 0x10000;
                }
            } else if (memcmp(p, "FLASH", 5) == 0) {
                if (detectedSaveType == 0) {
                    detectedSaveType = 4;
                    flashSize = 0x10000;
                }
            }
        } else if (d == 0x52494953) {
            if (memcmp(p, "SIIRTC_V", 8) == 0)
                rtcFound = true;
        }
        p++;
    }
    // if no matches found, then set it to NONE
    if (detectedSaveType == 0) {
        detectedSaveType = 5;
    }
    if (detectedSaveType == 4) {
        detectedSaveType = 3;
    }
    rtcEnable(rtcFound);
    rtcEnableRumble(!rtcFound);
    coreOptions.saveType = detectedSaveType;
    flashSetSize(flashSize);
}