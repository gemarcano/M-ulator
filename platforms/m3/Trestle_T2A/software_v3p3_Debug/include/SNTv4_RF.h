//************************************************************
// Desciption: SNTv4 Register File Header File
//      Generated by genRF (Version 1.30) 04/30/2019 12:06:47
//************************************************************

#ifndef SNTV4_RF_H
#define SNTV4_RF_H

// Register 0x00
typedef union sntv4_r00{
  struct{
    unsigned LDO_EN_LDO		: 1;
    unsigned LDO_EN_IREF		: 1;
    unsigned LDO_EN_VREF		: 1;
    unsigned LDO_SEL_VOUT		: 3;
    unsigned LDO_VREF_I_AMP		: 4;
  };
  uint32_t as_int;
} sntv4_r00_t;
#define SNTv4_R00_DEFAULT {{0x0, 0x0, 0x0, 0x4, 0x0}}
_Static_assert(sizeof(sntv4_r00_t) == 4, "Punned Structure Size");

// Register 0x01
typedef union sntv4_r01{
  struct{
    unsigned TSNS_RESETn		: 1;
    unsigned TSNS_ISOLATE		: 1;
    unsigned TSNS_SEL_V1P2		: 1;
    unsigned TSNS_SEL_LDO		: 1;
    unsigned TSNS_EN_SENSOR_V1P2		: 1;
    unsigned TSNS_EN_SENSOR_LDO		: 1;
    unsigned TSNS_BURST_MODE		: 1;
    unsigned TSNS_CONT_MODE		: 1;
    unsigned TSNS_EN_IRQ		: 1;
    unsigned TSNS_EN_CLK_SENS		: 1;
    unsigned TSNS_EN_CLK_REF		: 1;
    unsigned TSNS_FORCE_CLR_IRQ_IF_EN_IRQ_0		: 1;
  };
  uint32_t as_int;
} sntv4_r01_t;
#define SNTv4_R01_DEFAULT {{0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1}}
_Static_assert(sizeof(sntv4_r01_t) == 4, "Punned Structure Size");

// Register 0x02
typedef union sntv4_r02{
  struct{
    unsigned TSNS_I_SOSC		: 4;
    unsigned TSNS_I_MIRROR		: 2;
    unsigned TSNS_I_CMP		: 4;
    unsigned TSNS_I_BUF2		: 4;
    unsigned TSNS_I_BUF		: 4;
    unsigned TSNS_R_REF		: 4;
  };
  uint32_t as_int;
} sntv4_r02_t;
#define SNTv4_R02_DEFAULT {{0x0, 0x1, 0x1, 0x0, 0x0, 0x8}}
_Static_assert(sizeof(sntv4_r02_t) == 4, "Punned Structure Size");

// Register 0x03
typedef union sntv4_r03{
  struct{
    unsigned TSNS_SEL_CONV_TIME		: 4;
    unsigned TSNS_SEL_REF_STB_TIME		: 2;
    unsigned TSNS_SEL_STB_TIME		: 3;
    unsigned TSNS_SEL_VVDD		: 4;
    unsigned TSNS_MOM		: 3;
    unsigned TSNS_MIM		: 3;
  };
  uint32_t as_int;
} sntv4_r03_t;
#define SNTv4_R03_DEFAULT {{0x6, 0x2, 0x1, 0xE, 0x4, 0x4}}
_Static_assert(sizeof(sntv4_r03_t) == 4, "Punned Structure Size");

// Register 0x04
typedef union sntv4_r04{
  struct{
    unsigned TSNS_PDIFF		: 15;
  };
  uint32_t as_int;
} sntv4_r04_t;
#define SNTv4_R04_DEFAULT {{0x0200}}
_Static_assert(sizeof(sntv4_r04_t) == 4, "Punned Structure Size");

// Register 0x05
typedef union sntv4_r05{
  struct{
    unsigned TSNS_POLY		: 24;
  };
  uint32_t as_int;
} sntv4_r05_t;
#define SNTv4_R05_DEFAULT {{0x004000}}
_Static_assert(sizeof(sntv4_r05_t) == 4, "Punned Structure Size");

// Register 0x06
// -- READ-ONLY --

// Register 0x07
typedef union sntv4_r07{
  struct{
    unsigned TSNS_INT_RPLY_REG_ADDR		: 8;
    unsigned TSNS_INT_RPLY_SHORT_ADDR		: 8;
  };
  uint32_t as_int;
} sntv4_r07_t;
#define SNTv4_R07_DEFAULT {{0x00, 0x10}}
_Static_assert(sizeof(sntv4_r07_t) == 4, "Punned Structure Size");

// Register 0x08
typedef union sntv4_r08{
  struct{
    unsigned TMR_EN_SELF_CLK		: 1;
    unsigned TMR_RESETB_DCDC		: 1;
    unsigned TMR_RESETB_DIV		: 1;
    unsigned TMR_EN_OSC		: 1;
    unsigned TMR_RESETB		: 1;
    unsigned TMR_ISOLATE		: 1;
    unsigned TMR_SLEEP		: 1;
  };
  uint32_t as_int;
} sntv4_r08_t;
#define SNTv4_R08_DEFAULT {{0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1}}
_Static_assert(sizeof(sntv4_r08_t) == 4, "Punned Structure Size");

// Register 0x09
typedef union sntv4_r09{
  struct{
    unsigned TMR_EN_TUNE2		: 1;
    unsigned TMR_EN_TUNE1		: 1;
    unsigned TMR_SEL_DCAP		: 6;
    unsigned TMR_SEL_CAP		: 8;
    unsigned TMR_CASCODE_BOOST		: 1;
    unsigned TMR_IBIAS_REF		: 4;
    unsigned TMR_SELF_EN		: 1;
    unsigned TMR_SEL_CLK_OSC		: 1;
    unsigned TMR_SEL_CLK_DIV		: 1;
  };
  uint32_t as_int;
} sntv4_r09_t;
#define SNTv4_R09_DEFAULT {{0x1, 0x1, 0x3F, 0x80, 0x0, 0x4, 0x1, 0x1, 0x0}}
_Static_assert(sizeof(sntv4_r09_t) == 4, "Punned Structure Size");

// Register 0x0A
typedef union sntv4_r0A{
  struct{
    unsigned TMR_AFC		: 3;
    unsigned TMR_SAMPLE_EN		: 1;
    unsigned TMR_EN_TUNE2_RES		: 1;
    unsigned TMR_EN_TUNE1_RES		: 1;
    unsigned NOT_DEFINED_10_6		: 1;
    unsigned TMR_DIFF_CON		: 14;
    unsigned TMR_S		: 3;
  };
  uint32_t as_int;
} sntv4_r0A_t;
#define SNTv4_R0A_DEFAULT {{0x4, 0x1, 0x1, 0x1, 0x0, 0x3FFF, 0x1}}
_Static_assert(sizeof(sntv4_r0A_t) == 4, "Punned Structure Size");

// Register 0x0B
typedef union sntv4_r0B{
  struct{
    unsigned TMR_TFR_CON		: 4;
  };
  uint32_t as_int;
} sntv4_r0B_t;
#define SNTv4_R0B_DEFAULT {{0xF}}
_Static_assert(sizeof(sntv4_r0B_t) == 4, "Punned Structure Size");

// Register 0x0C
// -- EMPTY --

// Register 0x0D
// -- EMPTY --

// Register 0x0E
// -- EMPTY --

// Register 0x0F
// -- EMPTY --

// Register 0x10
// -- EMPTY --

// Register 0x11
// -- EMPTY --

// Register 0x12
// -- EMPTY --

// Register 0x13
// -- EMPTY --

// Register 0x14
// -- EMPTY --

// Register 0x15
// -- EMPTY --

// Register 0x16
// -- EMPTY --

// Register 0x17
typedef union sntv4_r17{
  struct{
    unsigned WUP_INT_RPLY_REG_ADDR		: 8;
    unsigned WUP_INT_RPLY_SHORT_ADDR		: 8;
    unsigned NOT_DEFINED_23_16		: 1;
    unsigned NOT_DEFINED_23_17		: 1;
    unsigned NOT_DEFINED_23_18		: 1;
    unsigned WUP_ENABLE_CLK_SLP_OUT		: 1;
    unsigned WUP_CLK_SEL		: 1;
    unsigned WUP_AUTO_RESET		: 1;
    unsigned WUP_LC_IRQ_EN		: 1;
    unsigned WUP_ENABLE		: 1;
  };
  uint32_t as_int;
} sntv4_r17_t;
#define SNTv4_R17_DEFAULT {{0x07, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x1, 0x0}}
_Static_assert(sizeof(sntv4_r17_t) == 4, "Punned Structure Size");

// Register 0x18
typedef union sntv4_r18{
  struct{
    unsigned WUP_INT_RPLY_PAYLOAD		: 24;
  };
  uint32_t as_int;
} sntv4_r18_t;
#define SNTv4_R18_DEFAULT {{0x023002}}
_Static_assert(sizeof(sntv4_r18_t) == 4, "Punned Structure Size");

// Register 0x19
typedef union sntv4_r19{
  struct{
    unsigned WUP_THRESHOLD_EXT		: 8;
  };
  uint32_t as_int;
} sntv4_r19_t;
#define SNTv4_R19_DEFAULT {{0x00}}
_Static_assert(sizeof(sntv4_r19_t) == 4, "Punned Structure Size");

// Register 0x1A
typedef union sntv4_r1A{
  struct{
    unsigned WUP_THRESHOLD		: 24;
  };
  uint32_t as_int;
} sntv4_r1A_t;
#define SNTv4_R1A_DEFAULT {{0x2DC6C0}}
_Static_assert(sizeof(sntv4_r1A_t) == 4, "Punned Structure Size");

// Register 0x1B
// -- READ-ONLY --

// Register 0x1C
// -- READ-ONLY --

// Register 0x1D
typedef union sntv4_r1D{
  struct{
    unsigned LC_CLK_RING		: 2;
    unsigned LC_CLK_DIV		: 2;
    unsigned NOT_DEFINED_29_4		: 1;
    unsigned MBC_WAKEUP_ON_PEND_REQ		: 1;
  };
  uint32_t as_int;
} sntv4_r1D_t;
#define SNTv4_R1D_DEFAULT {{0x1, 0x2, 0x0, 0x0}}
_Static_assert(sizeof(sntv4_r1D_t) == 4, "Punned Structure Size");

// Register 0x1E
// -- EMPTY --

// Register 0x1F
// -- EMPTY --

// Register 0x20
typedef union sntv4_r20{
  struct{
    unsigned RF2_SLEEP		: 1;
    unsigned RF2_ISOLATE		: 1;
  };
  uint32_t as_int;
} sntv4_r20_t;
#define SNTv4_R20_DEFAULT {{0x0, 0x0}}
_Static_assert(sizeof(sntv4_r20_t) == 4, "Punned Structure Size");

#endif // SNTV4_RF_H