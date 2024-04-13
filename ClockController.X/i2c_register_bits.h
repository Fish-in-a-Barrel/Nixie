#ifndef I2C_REGISTER_BITS_H
#define	I2C_REGISTER_BITS_H

//
// SSPxSTAT - §25.4.4
//

// bit 7
#define SSPxSTAT_SLEW_RATE_CTL_MASK (1 << 7)
#define SSPxSTAT_SLEW_RATE_CTL_DISABLED (1 << 7)
#define SSPxSTAT_SLEW_RATE_CTL_ENABLED 0

// bit 6
#define SSPxSTAT_CKE_SMBUS_MASK (1 << 6)
#define SSPxSTAT_CKE_SMBUS_ENABLED (1 << 6)
#define SSPxSTAT_CKE_SMBUS_DISABLED 0

// bit 5
#define SSPxSTAT_DOA_MASK (1 << 5)
#define SSPxSTAT_DOA_DATA (1 << 5)
#define SSPxSTAT_DOA_ADDRESS 0

// bit 4
#define SSPxSTAT_STOP_MASK (1 << 4)
#define SSPxSTAT_STOP_SET (1 << 4)
#define SSPxSTAT_STOP_CLR 0

// bit 3
#define SSPxSTAT_START_MASK (1 << 3)
#define SSPxSTAT_START_SET (1 << 3)
#define SSPxSTAT_START_CLR 0

// bit 2
#define SSPxSTAT_RW_MASK (1 << 2)
#define SSPxSTAT_RW_CLIENT_READ (1 << 2)
#define SSPxSTAT_RW_CLIENT_WRITE 0
#define SSPxSTAT_RW_HOST_TX_ACTIVE (1 << 2)
#define SSPxSTAT_RW_HOST_TX_INACTIVE 0

// bit 1
#define SSPxSTAT_ADDR_UPDATE_MASK (1 << 1)
#define SSPxSTAT_ADDR_UPDATE_NEEDED (1 << 1)
#define SSPxSTAT_ADDR_UPDATE_OK

// bit 0
#define SSPxSTAT_TX_BUF_MASK (1 << 0)
#define SSPxSTAT_TX_BUF_FULL (1 << 0)
#define SSPxSTAT_TX_BUF_CLR

//
// SSPxCON1 - §25.4.5
//

// bit 7
#define SSPxCON1_WCOL_MASK (1 << 7)
#define SSPxCON1_WCOL_COL (1 << 7)
#define SSPxCON1_WCOL_OK 0

// bit 6
#define SSPxCON1_SSPOV_MASK (1 << 6)
#define SSPxCON1_SSPOV_OVERFLOW (1 << 6)
#define SSPxCON1_SSPOV_OK 0

// bit 5
#define SSPxCON1_SSPEN_MASK (1 << 5)
#define SSPxCON1_SSPEN_ENABLED (1 << 5)
#define SSPxCON1_SSPEN_DISABLED 0

// bit 4
#define SSPxCON1_CKP_MASK (1 << 5)
#define SSPxCON1_CKP_RELEASE (1 << 5)
#define SSPxCON1_CKP_HOLD 0

// bit 3:0
#define SSPxCON1_SSMP_MASK 0xF
#define SSPxCON1_SSMP_CLIENT_10BIT_WITH_INT 0xF
#define SSPxCON1_SSMP_CLIENT_7BIT_WITH_INT 0xE
#define SSPxCON1_SSMP_HOST_FIRMARE 0xB
#define SSPxCON1_SSMP_HOST 0x8
#define SSPxCON1_SSMP_CLIENT_10BIT 0x7
#define SSPxCON1_SSMP_CLIENT_7BIT 0x6

//
// SSPxCON2 - §25.4.6
//

// bit 7
#define SSPxCON2_GCEN_MASK (1 << 7)
#define SSPxCON2_GCEN_ENABLED (1 << 7)
#define SSPxCON2_GCEN_DISABLED 0

// bit 6
#define SSPxCON2_ACKSTAT_MASK (1 << 6)
#define SSPxCON2_ACKSTAT_NACK (1 << 6)
#define SSPxCON2_ACKSTAT_ACK 0

// bit 5
#define SSPxCON2_ACKDT_MASK (1 << 5)
#define SSPxCON2_ACKDT_NACK (1 << 5)
#define SSPxCON2_ACKDT_ACK 0

// bit 4
#define SSPxCON2_ACKEN_MASK (1 << 4)
#define SSPxCON2_ACKEN_ENABLED (1 << 4)
#define SSPxCON2_ACKE_DISABLED 0

// bit 3
#define SSPxCON2_RCEN_MASK (1 << 3)
#define SSPxCON2_RCEN_ENABLED (1 << 3)
#define SSPxCON2_RCEN_DISABLED 0

// bit 2
#define SSPxCON2_PEN_MASK (1 << 2)
#define SSPxCON2_PEN_ENABLED (1 << 2)
#define SSPxCON2_PEN_DISABLED 0

// bit 1
#define SSPxCON2_RSEN_MASK (1 << 1)
#define SSPxCON2_RSEN_ENABLED (1 << 1)
#define SSPxCON2_RSEN_DISABLED 0

// bit 0
#define SSPxCON2_SEN_MASK (1 << 0)
#define SSPxCON2_SEN_ENABLED (1 << 0)
#define SSPxCON2_SEN_DISABLED 0

//
// SSPxCON3 - §25.4.7
//

// bit 7
#define SSPxCON3_ACTIM_MASK (1 << 7)
#define SSPxCON3_ACTIM_ACTIVE (1 << 7)
#define SSPxCON3_ACTIM_NOT_ACTIVE 0

// bit 6
#define SSPxCON3_PCIE_MASK (1 << 6)
#define SSPxCON3_PCIE_ENABLED (1 << 6)
#define SSPxCON3_PCIE_DISABLED 0

// bit 5
#define SSPxCON3_SCIE_MASK (1 << 5)
#define SSPxCON3_SCIE_ENABLED (1 << 5)
#define SSPxCON3_SCIE_DISABLED 0

// bit 4
#define SSPxCON3_BOEN_MASK (1 << 4)
#define SSPxCON3_BOEN_UPDATE_ON_NEW_BYTE (1 << 4)
#define SSPxCON3_BOEN_UPDATE_ON_SSPOV 0

// bit 3
#define SSPxCON3_SDAHT_MASK (1 << 3)
#define SSPxCON3_SDAHT_300_NS_MIN (1 << 3)
#define SSPxCON3_SDAHT_100_NS_MIN 0

// bit 2
#define SSPxCON3_SBCDE_MASK (1 << 2)
#define SSPxCON3_SBCDE_ENABLED (1 << 2)
#define SSPxCON3_SBCDE_DISABLED 0

// bit 1
#define SSPxCON3_AHEN_MASK (1 << 1)
#define SSPxCON3_AHEN_ENABLED (1 << 1)
#define SSPxCON3_AHEN_DISABLED 0

// bit 0
#define SSPxCON3_DHEN_MASK 1
#define SSPxCON3_DHEN_ENABLED 1
#define SSPxCON3_DHEN_DISABLED 0

//
// SSPxMSK - §25.4.3
//

// bit 7:1
#define SSPxMSK_ADDR_MASK_MASK 0xFE

// bit 0
#define SSPxMSK_10BIT_MASK 0x01
#define SSPxMSK_10BIT_ENABLED 1
#define SSPxMSK_10BIT_DISABLED 0

//
// RxxI2C - §16.14.9
//

// bit 7:6
#define SLEW_FAST_PLUS (3 << 6)
#define SLEW_FAST (2 << 6)
#define SLEW_STANDARD 0

// bit 5:4
#define PU_10X (2 << 4)
#define PU_2X (1 << 4)
#define PU_STANDARD 0

// 2:1
#define TH_SMBUS 2
#define TH_I2C 1
#define TH_STANDARD 0

#endif	/* I2C_REGISTER_BITS_H */
