// Taken from the debug ps2_screenshot source

#define GIF_AD    0x0e

#define GIFTAG(NLOOP,EOP,PRE,PRIM,FLG,NREG) \
    ((u64)(NLOOP) << 0)   | \
    ((u64)(EOP)   << 15)  | \
    ((u64)(PRE)   << 46)  | \
    ((u64)(PRIM)  << 47)  | \
    ((u64)(FLG)   << 58)  | \
    ((u64)(NREG)  << 60)

#define GSBITBLTBUF_SET(sbp, sbw, spsm, dbp, dbw, dpsm) \
  ((u64)(sbp)         | ((u64)(sbw) << 16) | \
  ((u64)(spsm) << 24) | ((u64)(dbp) << 32) | \
  ((u64)(dbw) << 48)  | ((u64)(dpsm) << 56))

#define GSTRXREG_SET(rrw, rrh) \
  ((u64)(rrw) | ((u64)(rrh) << 32))

#define GSTRXPOS_SET(ssax, ssay, dsax, dsay, dir) \
  ((u64)(ssax)        | ((u64)(ssay) << 16) | \
  ((u64)(dsax) << 32) | ((u64)(dsay) << 48) | \
  ((u64)(dir) << 59))

#define GSTRXDIR_SET(xdr) ((u64)(xdr))

#define GSBITBLTBUF         0x50
#define GSFINISH            0x61
#define GSTRXPOS            0x51
#define GSTRXREG            0x52
#define GSTRXDIR            0x53

#define GSPSMCT32           0
#define GSPSMCT24           1
#define GSPSMCT16           2

#define D1_CHCR             ((volatile unsigned int *)(0x10009000))
#define D1_MADR             ((volatile unsigned int *)(0x10009010))
#define D1_QWC              ((volatile unsigned int *)(0x10009020))
#define D1_TADR             ((volatile unsigned int *)(0x10009030))
#define D1_ASR0             ((volatile unsigned int *)(0x10009040))
#define D1_ASR1             ((volatile unsigned int *)(0x10009050))

#define CSR_FINISH          (1 << 1)
#define GS_CSR              ((volatile u64 *)(0x12001000))
#define GS_BUSDIR           ((volatile u64 *)(0x12001040))

#define VIF1_STAT           ((volatile u32 *)(0x10003c00))
#define VIF1_STAT_FDR       (1<< 23)
#define VIF1_MSKPATH3(mask) ((u32)(mask) | ((u32)0x06 << 24))
#define VIF1_NOP            0
#define VIF1_FLUSHA         (((u32)0x13 << 24))
#define VIF1_DIRECT(count)  ((u32)(count) | ((u32)(0x50) << 24))
#define VIF1_FIFO           ((volatile u128 *)(0x10005000))
