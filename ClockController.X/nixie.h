#ifndef NIXIE_H
#define	NIXIE_H

#include <xc.h>

extern uint16_t gNixieStatus;

void UpdateNixieDrivers(void);

void RefreshNixies(void);

#endif	/* NIXIE_H */

