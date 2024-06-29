#include "time_zone.h"

int8_t gTimeZoneOffset = -6;

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