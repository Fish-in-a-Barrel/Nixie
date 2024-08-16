#ifndef PPS_INPUTS_H
#define	PPS_INPUTS_H

#define PPS_PORT_A 00
#define PPS_PORT_B 01
#define PPS_PORT_C 02

// PPS_PORT_X | PIN #
#define PPS_INPUT(port, pin) ((port << 3) | (pin))

#endif	/* PPS_INPUTS_H */
