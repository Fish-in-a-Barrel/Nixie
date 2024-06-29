#include "time_zone.h"

int8_t gTimeZoneOffset = -6;
uint8_t gDstType = DST_TYPE_AUTO_US;

#define TIME_ZONE_MEMORY_LOCATION 0x8000

const char* TIME_ZONE_ABRV[27][3] =
{ //   _TZ_    _ST_    _DT_
    { "BIT",  "BIT",  "BIT"  }, // -12
    { "ST",   "SST",  "SDT"  }, // -11
    { "HT",   "HST",  "HDT"  }, // -10
    { "AKT",  "AKST", "AKDT" }, // -09
    { "PT",   "PST",  "PDT"  }, // -08
    { "MT",   "MST",  "MDT"  }, // -07
    { "CT",   "CST",  "CDT"  }, // -06
    { "ET",   "EST",  "EDT"  }, // -05
    { "AT",   "AST",  "ADT"  }, // -04
    { "BRT",  "BRT",  "BRT"  }, // -03
    { "GST",  "GST",  "GST"  }, // -02
    { "CVT",  "CVT",  "CVT"  }, // -01
    { "GMT",  "GMT",  "GMT"  }, //  00
    { "CET",  "CET",  "CEST" }, // +01
    { "EET",  "EET",  "EEST" }, // +02
    { "MSK",  "MSK",  "MSK"  }, // +03
    { "GST",  "GST",  "GST"  }, // +04
    { "PKT",  "PKT",  "PKT"  }, // +05
    { "IOT",  "IOT",  "IOT"  }, // +06
    { "ICT",  "ICT",  "ICT"  }, // +07
    { "CST",  "CST",  "CST"  }, // +08
    { "JST",  "JST",  "JST"  }, // +09
    { "AET",  "AEST", "AEDT" }, // +10
    { "VUT",  "VUT",  "VUT"  }, // +11
    { "NZT",  "NZST", "NZDT" }, // +12
    { "TOT",  "TOT",  "TOT"  }, // +13
    { "LINT", "LINT", "LINT" }, // +14
};

const char* DST_TYPE_ABRV[2] =
{
    "Off      ",
    "Auto (US)",
};

void UnlockNVM(void)
{
    // Disable interrupts during unlock
    INTCONbits.GIE = 0;
    
    // Unlock sequence (§15.3.2)
    NVMCON2 = 0x55;
    NVMCON2 = 0xAA;
    NVMCON1bits.WR = 1; 
    
    INTCONbits.GIE = 1;
}

void TimeZone_Save(void)
{
    // Clear the memory (§15.3.3)
    NVMREGS = 1;
    NVMCON1bits.FREE = 1;
    NVMCON1bits.WREN = 1;
    NVMADR = TIME_ZONE_MEMORY_LOCATION;
    UnlockNVM();
    
    //
    // NOTE: We're only writing a single word, so there is no need to load the write latches.
    //

    // Write (§15.3.4)
    NVMREGS = 1;
    NVMCON1bits.FREE = 0;
    NVMCON1bits.WREN = 1;
    NVMADR = TIME_ZONE_MEMORY_LOCATION;
    NVMDATL = (uint8_t)gTimeZoneOffset;
    NVMDATH = gDstType;
    UnlockNVM();

    NVMCON1bits.WREN = 0;
}

void TimeZone_Load(void)
{
    // Read the memory (§15.3.1)
    NVMREGS = 1;
    NVMADR = TIME_ZONE_MEMORY_LOCATION;
    NVMCON1bits.RD = 1;
    
    // 0x3FFF is the cleared state of the memory, which means nothing has been save yet.
    if (NVMDAT != 0x3FFF)
    {
        gTimeZoneOffset = (int8_t)NVMDATL;
        gDstType = NVMDATH;
    }
}