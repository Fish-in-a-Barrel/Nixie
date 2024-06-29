#include "time_zone.h"

int8_t gTimeZoneOffset = -6;
uint8_t gDstType = DST_TYPE_AUTO_US;

#define TIME_ZONE_MEMORY_LOCATION 0x8000

const char* TIME_ZONE_ABRV[27][2] =
{ //   _TZ_    _ST_    _DT_
    { "BIT",  "BIT"  }, // -12
    { "SST",  "NUT"  }, // -11
    { "HST",  "SDT"  }, // -10
    { "AKST", "HDT"  }, // -09
    { "PST",  "AKDT" }, // -08
    { "MST",  "PDT"  }, // -07
    { "CST",  "MDT"  }, // -06
    { "EST",  "CDT"  }, // -05
    { "AST",  "EDT"  }, // -04
    { "BRT",  "ADT"  }, // -03
    { "GST",  "GST"  }, // -02
    { "CVT",  "CVT"  }, // -01
    { "GMT",  "GMT"  }, //  00
    { "CET",  "MET"  }, // +01
    { "EET",  "CEST" }, // +02
    { "MSK",  "EEST" }, // +03
    { "GST",  "GST"  }, // +04
    { "PKT",  "PKT"  }, // +05
    { "IOT",  "IOT"  }, // +06
    { "ICT",  "ICT"  }, // +07
    { "CST",  "CST"  }, // +08
    { "JST",  "JST"  }, // +09
    { "AEST", "PGT"  }, // +10
    { "VUT",  "AEDT" }, // +11
    { "NZST", "TVT"  }, // +12
    { "TOT",  "NZDT" }, // +13
    { "LINT", "LINT" }, // +14
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