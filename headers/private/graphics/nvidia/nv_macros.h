/* NV registers definitions and macros for access to */

//new:
/* PCI_config_space */
#define NVCFG_DEVID		0x00
#define NVCFG_DEVCTRL	0x04
#define NVCFG_CLASS		0x08
#define NVCFG_HEADER	0x0c
#define NVCFG_BASE1REGS	0x10
#define NVCFG_BASE2FB	0x14
#define NVCFG_BASE3		0x18
#define NVCFG_BASE4		0x1c //unknown if used
#define NVCFG_BASE5		0x20 //unknown if used
#define NVCFG_BASE6		0x24 //unknown if used
#define NVCFG_BASE7		0x28 //unknown if used
#define NVCFG_SUBSYSID1	0x2c
#define NVCFG_ROMBASE	0x30
#define NVCFG_CFG_0		0x34
#define NVCFG_CFG_1		0x38 //unknown if used
#define NVCFG_INTERRUPT	0x3c
#define NVCFG_SUBSYSID2	0x40
#define NVCFG_AGPREF	0x44
#define NVCFG_AGPSTAT	0x48
#define NVCFG_AGPCMD	0x4c
#define NVCFG_ROMSHADOW	0x50
#define NVCFG_VGA		0x54
#define NVCFG_SCHRATCH	0x58
#define NVCFG_CFG_10	0x5c
#define NVCFG_CFG_11	0x60
#define NVCFG_CFG_12	0x64
#define NVCFG_CFG_13	0x68 //unknown if used
#define NVCFG_CFG_14	0x6c //unknown if used
#define NVCFG_CFG_15	0x70 //unknown if used
#define NVCFG_CFG_16	0x74 //unknown if used
#define NVCFG_CFG_17	0x78 //unknown if used
#define NVCFG_GF2IGPU	0x7c //wrong...
#define NVCFG_CFG_19	0x80 //unknown if used
#define NVCFG_GF4MXIGPU	0x84 //wrong...
#define NVCFG_CFG_21	0x88 //unknown if used
#define NVCFG_CFG_22	0x8c //unknown if used
#define NVCFG_CFG_23	0x90 //unknown if used
#define NVCFG_CFG_24	0x94 //unknown if used
#define NVCFG_CFG_25	0x98 //unknown if used
#define NVCFG_CFG_26	0x9c //unknown if used
#define NVCFG_CFG_27	0xa0 //unknown if used
#define NVCFG_CFG_28	0xa4 //unknown if used
#define NVCFG_CFG_29	0xa8 //unknown if used
#define NVCFG_CFG_30	0xac //unknown if used
#define NVCFG_CFG_31	0xb0 //unknown if used
#define NVCFG_CFG_32	0xb4 //unknown if used
#define NVCFG_CFG_33	0xb8 //unknown if used
#define NVCFG_CFG_34	0xbc //unknown if used
#define NVCFG_CFG_35	0xc0 //unknown if used
#define NVCFG_CFG_36	0xc4 //unknown if used
#define NVCFG_CFG_37	0xc8 //unknown if used
#define NVCFG_CFG_38	0xcc //unknown if used
#define NVCFG_CFG_39	0xd0 //unknown if used
#define NVCFG_CFG_40	0xd4 //unknown if used
#define NVCFG_CFG_41	0xd8 //unknown if used
#define NVCFG_CFG_42	0xdc //unknown if used
#define NVCFG_CFG_43	0xe0 //unknown if used
#define NVCFG_CFG_44	0xe4 //unknown if used
#define NVCFG_CFG_45	0xe8 //unknown if used
#define NVCFG_CFG_46	0xec //unknown if used
#define NVCFG_CFG_47	0xf0 //unknown if used
#define NVCFG_CFG_48	0xf4 //unknown if used
#define NVCFG_CFG_49	0xf8 //unknown if used
#define NVCFG_CFG_50	0xfc //unknown if used

/*    if(pNv->SecondCRTC) {
       pNv->riva.PCIO = pNv->riva.PCIO0 + 0x2000;
       pNv->riva.PCRTC = pNv->riva.PCRTC0 + 0x800;
       pNv->riva.PRAMDAC = pNv->riva.PRAMDAC0 + 0x800;
       pNv->riva.PDIO = pNv->riva.PDIO0 + 0x2000;
    } else {
       pNv->riva.PCIO = pNv->riva.PCIO0;
       pNv->riva.PCRTC = pNv->riva.PCRTC0;
       pNv->riva.PRAMDAC = pNv->riva.PRAMDAC0;
       pNv->riva.PDIO = pNv->riva.PDIO0;
    }

    pNv->riva.PCIO0 = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x00601000,
                                           0x00003000);
    pNv->riva.PDIO0 = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x00681000,
                                           0x00003000);
    pNv->riva.PVIO = (U008 *)xf86MapPciMem(pScrn->scrnIndex, mmioFlags,
                                           pNv->PciTag, regBase+0x000C0000,
                                           0x00001000);
    pNv->riva.PRAMDAC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00680000, 0x00003000);
    pNv->riva.PCRTC0 = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00600000, 0x00003000);

    pNv->riva.FIFO    = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00800000, 0x00010000);

    pNv->riva.PFIFO   = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00002000, 0x00002000);

    pNv->riva.PFB     = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00100000, 0x00001000);

    pNv->riva.PMC     = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00000000, 0x00009000);

    pNv->riva.PTIMER  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00009000, 0x00001000);

    pNv->riva.PRAMIN = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                     regBase+0x00710000, 0x00010000);

    pNv->riva.PGRAPH  = xf86MapPciMem(pScrn->scrnIndex, mmioFlags, pNv->PciTag,
                                      regBase+0x00400000, 0x00002000);
*/

/* used NV INT registers for vblank */
#define NV32_MAIN_INTE		0x00000140
#define NV32_CRTC_INTS		0x00600100
#define NV32_CRTC_INTE		0x00600140

/* NV ACCeleration registers */
/* engine initialisation registers */
#define NVACC_FORMATS		0x00400618
#define NVACC_OFFSET0		0x00400640
#define NVACC_OFFSET1		0x00400644
#define NVACC_OFFSET2		0x00400648
#define NVACC_OFFSET3		0x0040064c
#define NVACC_OFFSET4		0x00400650
#define NVACC_OFFSET5		0x00400654
#define NVACC_BBASE0		0x00400658
#define NVACC_BBASE1		0x0040065c
#define NVACC_BBASE2		0x00400660
#define NVACC_BBASE3		0x00400664
#define NVACC_NV10_BBASE4	0x00400668
#define NVACC_NV10_BBASE5	0x0040066c
#define NVACC_PITCH0		0x00400670
#define NVACC_PITCH1		0x00400674
#define NVACC_PITCH2		0x00400678
#define NVACC_PITCH3		0x0040067c
#define NVACC_PITCH4		0x00400680
#define NVACC_BLIMIT0		0x00400684
#define NVACC_BLIMIT1		0x00400688
#define NVACC_BLIMIT2		0x0040068c
#define NVACC_BLIMIT3		0x00400690
#define NVACC_NV10_BLIMIT4	0x00400694
#define NVACC_NV10_BLIMIT5	0x00400698
#define NVACC_BPIXEL		0x00400724
#define NVACC_NV20_OFFSET0	0x00400820
#define NVACC_NV20_OFFSET1	0x00400824
#define NVACC_NV20_OFFSET2	0x00400828
#define NVACC_NV20_OFFSET3	0x0040082c
#define NVACC_STRD_FMT		0x00400830
#define NVACC_NV20_PITCH0	0x00400850
#define NVACC_NV20_PITCH1	0x00400854
#define NVACC_NV20_PITCH2	0x00400858
#define NVACC_NV20_PITCH3	0x0040085c
#define NVACC_NV20_BLIMIT6	0x00400864
#define NVACC_NV20_BLIMIT7	0x00400868
#define NVACC_NV20_BLIMIT8	0x0040086c
#define NVACC_NV20_BLIMIT9	0x00400870
#define NVACC_NV30_WHAT		0x00400890

/* specials */
#define	NVACC_DEBUG0 		0x00400080
#define	NVACC_DEBUG1 		0x00400084
#define	NVACC_DEBUG2		0x00400088
#define	NVACC_DEBUG3		0x0040008c
#define	NVACC_NV10_DEBUG4 	0x00400090
#define NVACC_ACC_INTS		0x00400100
#define NVACC_ACC_INTE		0x00400140
#define NVACC_NV10_CTX_CTRL	0x00400144
#define NVACC_STATUS		0x00400700
#define NVACC_NV04_SURF_TYP	0x0040070c
#define NVACC_NV10_SURF_TYP	0x00400710
#define NVACC_NV04_ACC_STAT	0x00400710
#define NVACC_NV10_ACC_STAT	0x00400714
#define NVACC_FIFO_EN		0x00400720
#define NVACC_PAT_SHP		0x00400810
#define NVACC_NV10_XFMOD0	0x00400f40
#define NVACC_NV10_XFMOD1	0x00400f44
#define NVACC_NV10_PIPEADR	0x00400f50
#define NVACC_NV10_PIPEDAT	0x00400f54
/* PGRAPH cache registers */
#define	NVACC_CACHE1_1		0x00400160
#define	NVACC_CACHE1_2		0x00400180
#define	NVACC_CACHE1_3		0x004001a0
#define	NVACC_CACHE1_4		0x004001c0
#define	NVACC_CACHE1_5		0x004001e0
#define	NVACC_CACHE2_1		0x00400164
#define	NVACC_CACHE2_2		0x00400184
#define	NVACC_CACHE2_3		0x004001a4
#define	NVACC_CACHE2_4		0x004001c4
#define	NVACC_CACHE2_5		0x004001e4
#define	NVACC_CACHE3_1		0x00400168
#define	NVACC_CACHE3_2		0x00400188
#define	NVACC_CACHE3_3		0x004001a8
#define	NVACC_CACHE3_4		0x004001c8
#define	NVACC_CACHE3_5		0x004001e8
#define	NVACC_CACHE4_1		0x0040016c
#define	NVACC_CACHE4_2		0x0040018c
#define	NVACC_CACHE4_3		0x004001ac
#define	NVACC_CACHE4_4		0x004001cc
#define	NVACC_CACHE4_5		0x004001ec
#define	NVACC_NV10_CACHE5_1	0x00400170
#define	NVACC_NV04_CTX_CTRL	0x00400170
#define	NVACC_CACHE5_2		0x00400190
#define	NVACC_CACHE5_3		0x004001b0
#define	NVACC_CACHE5_4		0x004001d0
#define	NVACC_CACHE5_5		0x004001f0
#define	NVACC_NV10_CACHE6_1	0x00400174
#define	NVACC_CACHE6_2		0x00400194
#define	NVACC_CACHE6_3		0x004001b4
#define	NVACC_CACHE6_4		0x004001d4
#define	NVACC_CACHE6_5		0x004001f4
#define	NVACC_NV10_CACHE7_1	0x00400178
#define	NVACC_CACHE7_2		0x00400198
#define	NVACC_CACHE7_3		0x004001b8
#define	NVACC_CACHE7_4		0x004001d8
#define	NVACC_CACHE7_5		0x004001f8
#define	NVACC_NV10_CACHE8_1	0x0040017c
#define	NVACC_CACHE8_2		0x0040019c
#define	NVACC_CACHE8_3		0x004001bc
#define	NVACC_CACHE8_4		0x004001dc
#define	NVACC_CACHE8_5		0x004001fc
#define	NVACC_NV10_CTX_SW1	0x0040014c
#define	NVACC_NV10_CTX_SW2	0x00400150
#define	NVACC_NV10_CTX_SW3	0x00400154
#define	NVACC_NV10_CTX_SW4	0x00400158
#define	NVACC_NV10_CTX_SW5	0x0040015c
/* engine tile registers src */
#define NVACC_NV20_FBWHAT0	0x00100200
#define NVACC_NV20_FBWHAT1	0x00100204
#define NVACC_NV10_FBTIL0AD	0x00100240
#define NVACC_NV10_FBTIL0ED	0x00100244
#define NVACC_NV10_FBTIL0PT	0x00100248
#define NVACC_NV10_FBTIL0ST	0x0010024c
#define NVACC_NV10_FBTIL1AD	0x00100250
#define NVACC_NV10_FBTIL1ED	0x00100254
#define NVACC_NV10_FBTIL1PT	0x00100258
#define NVACC_NV10_FBTIL1ST	0x0010025c
#define NVACC_NV10_FBTIL2AD	0x00100260
#define NVACC_NV10_FBTIL2ED	0x00100264
#define NVACC_NV10_FBTIL2PT	0x00100268
#define NVACC_NV10_FBTIL2ST	0x0010026c
#define NVACC_NV10_FBTIL3AD	0x00100270
#define NVACC_NV10_FBTIL3ED	0x00100274
#define NVACC_NV10_FBTIL3PT	0x00100278
#define NVACC_NV10_FBTIL3ST	0x0010027c
#define NVACC_NV10_FBTIL4AD	0x00100280
#define NVACC_NV10_FBTIL4ED	0x00100284
#define NVACC_NV10_FBTIL4PT	0x00100288
#define NVACC_NV10_FBTIL4ST	0x0010028c
#define NVACC_NV10_FBTIL5AD	0x00100290
#define NVACC_NV10_FBTIL5ED	0x00100294
#define NVACC_NV10_FBTIL5PT	0x00100298
#define NVACC_NV10_FBTIL5ST	0x0010029c
#define NVACC_NV10_FBTIL6AD	0x001002a0
#define NVACC_NV10_FBTIL6ED	0x001002a4
#define NVACC_NV10_FBTIL6PT	0x001002a8
#define NVACC_NV10_FBTIL6ST	0x001002ac
#define NVACC_NV10_FBTIL7AD	0x001002b0
#define NVACC_NV10_FBTIL7ED	0x001002b4
#define NVACC_NV10_FBTIL7PT	0x001002b8
#define NVACC_NV10_FBTIL7ST	0x001002bc
/* engine tile registers dst */
#define NVACC_NV20_WHAT0	0x004009a4
#define NVACC_NV20_WHAT1	0x004009a8
#define NVACC_NV10_TIL0AD	0x00400b00
#define NVACC_NV10_TIL0ED	0x00400b04
#define NVACC_NV10_TIL0PT	0x00400b08
#define NVACC_NV10_TIL0ST	0x00400b0c
#define NVACC_NV10_TIL1AD	0x00400b10
#define NVACC_NV10_TIL1ED	0x00400b14
#define NVACC_NV10_TIL1PT	0x00400b18
#define NVACC_NV10_TIL1ST	0x00400b1c
#define NVACC_NV10_TIL2AD	0x00400b20
#define NVACC_NV10_TIL2ED	0x00400b24
#define NVACC_NV10_TIL2PT	0x00400b28
#define NVACC_NV10_TIL2ST	0x00400b2c
#define NVACC_NV10_TIL3AD	0x00400b30
#define NVACC_NV10_TIL3ED	0x00400b34
#define NVACC_NV10_TIL3PT	0x00400b38
#define NVACC_NV10_TIL3ST	0x00400b3c
#define NVACC_NV10_TIL4AD	0x00400b40
#define NVACC_NV10_TIL4ED	0x00400b44
#define NVACC_NV10_TIL4PT	0x00400b48
#define NVACC_NV10_TIL4ST	0x00400b4c
#define NVACC_NV10_TIL5AD	0x00400b50
#define NVACC_NV10_TIL5ED	0x00400b54
#define NVACC_NV10_TIL5PT	0x00400b58
#define NVACC_NV10_TIL5ST	0x00400b5c
#define NVACC_NV10_TIL6AD	0x00400b60
#define NVACC_NV10_TIL6ED	0x00400b64
#define NVACC_NV10_TIL6PT	0x00400b68
#define NVACC_NV10_TIL6ST	0x00400b6c
#define NVACC_NV10_TIL7AD	0x00400b70
#define NVACC_NV10_TIL7ED	0x00400b74
#define NVACC_NV10_TIL7PT	0x00400b78
#define NVACC_NV10_TIL7ST	0x00400b7c
/* cache setup registers */
#define NVACC_PF_INTSTAT	0x00002100
#define NVACC_PF_INTEN		0x00002140
#define NVACC_PF_RAMHT		0x00002210
#define NVACC_PF_RAMFC		0x00002214
#define NVACC_PF_RAMRO		0x00002218
#define NVACC_PF_CACHES		0x00002500
#define NVACC_PF_SIZE		0x0000250c
#define NVACC_PF_CACH0_PSH0	0x00003000
#define NVACC_PF_CACH0_PUL0	0x00003050
#define NVACC_PF_CACH0_PUL1	0x00003054
#define NVACC_PF_CACH1_PSH0	0x00003200
#define NVACC_PF_CACH1_PSH1	0x00003204
#define NVACC_PF_CACH1_DMAI	0x0000322c
#define NVACC_PF_CACH1_PUL0	0x00003250
#define NVACC_PF_CACH1_PUL1 0x00003254
#define NVACC_PF_CACH1_HASH	0x00003258
/* Ptimer registers */
#define NVACC_PT_INTSTAT	0x00009100
#define NVACC_PT_INTEN		0x00009140
#define NVACC_PT_NUMERATOR	0x00009200
#define NVACC_PT_DENOMINATR	0x00009210
/* used PRAMIN registers */
#define NVACC_PR_CTX0_R		0x00711400
#define NVACC_PR_CTX1_R		0x00711404
#define NVACC_PR_CTX2_R		0x00711408
#define NVACC_PR_CTX3_R		0x0071140c
#define NVACC_PR_CTX0_0		0x00711420
#define NVACC_PR_CTX1_0		0x00711424
#define NVACC_PR_CTX2_0		0x00711428
#define NVACC_PR_CTX3_0		0x0071142c
#define NVACC_PR_CTX0_1		0x00711430
#define NVACC_PR_CTX1_1		0x00711434
#define NVACC_PR_CTX2_1		0x00711438
#define NVACC_PR_CTX3_1		0x0071143c
#define NVACC_PR_CTX0_2		0x00711440
#define NVACC_PR_CTX1_2		0x00711444
#define NVACC_PR_CTX2_2		0x00711448
#define NVACC_PR_CTX3_2		0x0071144c
#define NVACC_PR_CTX0_3		0x00711450
#define NVACC_PR_CTX1_3		0x00711454
#define NVACC_PR_CTX2_3		0x00711458
#define NVACC_PR_CTX3_3		0x0071145c
#define NVACC_PR_CTX0_4		0x00711460
#define NVACC_PR_CTX1_4		0x00711464
#define NVACC_PR_CTX2_4		0x00711468
#define NVACC_PR_CTX3_4		0x0071146c
#define NVACC_PR_CTX0_5		0x00711470
#define NVACC_PR_CTX1_5		0x00711474
#define NVACC_PR_CTX2_5		0x00711478
#define NVACC_PR_CTX3_5		0x0071147c
#define NVACC_PR_CTX0_6		0x00711480
#define NVACC_PR_CTX1_6		0x00711484
#define NVACC_PR_CTX2_6		0x00711488
#define NVACC_PR_CTX3_6		0x0071148c
#define NVACC_PR_CTX0_7		0x00711490
#define NVACC_PR_CTX1_7		0x00711494
#define NVACC_PR_CTX2_7		0x00711498
#define NVACC_PR_CTX3_7		0x0071149c
#define NVACC_PR_CTX0_8		0x007114a0
#define NVACC_PR_CTX1_8		0x007114a4
#define NVACC_PR_CTX2_8		0x007114a8
#define NVACC_PR_CTX3_8		0x007114ac
#define NVACC_PR_CTX0_9		0x007114b0
#define NVACC_PR_CTX1_9		0x007114b4
#define NVACC_PR_CTX2_9		0x007114b8
#define NVACC_PR_CTX3_9		0x007114bc
#define NVACC_PR_CTX0_A		0x007114c0
#define NVACC_PR_CTX1_A		0x007114c4 /* not used */
#define NVACC_PR_CTX2_A		0x007114c8
#define NVACC_PR_CTX3_A		0x007114cc
#define NVACC_PR_CTX0_B		0x007114d0
#define NVACC_PR_CTX1_B		0x007114d4
#define NVACC_PR_CTX2_B		0x007114d8
#define NVACC_PR_CTX3_B		0x007114dc
#define NVACC_PR_CTX0_C		0x007114e0
#define NVACC_PR_CTX1_C		0x007114e4
#define NVACC_PR_CTX2_C		0x007114e8
#define NVACC_PR_CTX3_C		0x007114ec
#define NVACC_PR_CTX0_D		0x007114f0
#define NVACC_PR_CTX1_D		0x007114f4
#define NVACC_PR_CTX2_D		0x007114f8
#define NVACC_PR_CTX3_D		0x007114fc
#define NVACC_PR_CTX0_E		0x00711500
#define NVACC_PR_CTX1_E		0x00711504
#define NVACC_PR_CTX2_E		0x00711508
#define NVACC_PR_CTX3_E		0x0071150c
/* used RAMHT registers (hash-table(?)) */
#define NVACC_HT_HANDL_00	0x00710000
#define NVACC_HT_VALUE_00	0x00710004
#define NVACC_HT_HANDL_01	0x00710008
#define NVACC_HT_VALUE_01	0x0071000c
#define NVACC_HT_HANDL_02	0x00710010
#define NVACC_HT_VALUE_02	0x00710014
#define NVACC_HT_HANDL_03	0x00710018
#define NVACC_HT_VALUE_03	0x0071001c
#define NVACC_HT_HANDL_04	0x00710020
#define NVACC_HT_VALUE_04	0x00710024
#define NVACC_HT_HANDL_05	0x00710028
#define NVACC_HT_VALUE_05	0x0071002c
#define NVACC_HT_HANDL_06	0x00710030
#define NVACC_HT_VALUE_06	0x00710034
#define NVACC_HT_HANDL_10	0x00710080
#define NVACC_HT_VALUE_10	0x00710084
#define NVACC_HT_HANDL_11	0x00710088
#define NVACC_HT_VALUE_11	0x0071008c
#define NVACC_HT_HANDL_12	0x00710090
#define NVACC_HT_VALUE_12	0x00710094
#define NVACC_HT_HANDL_13	0x00710098
#define NVACC_HT_VALUE_13	0x0071009c
#define NVACC_HT_HANDL_14	0x007100a0
#define NVACC_HT_VALUE_14	0x007100a4
#define NVACC_HT_HANDL_15	0x007100a8
#define NVACC_HT_VALUE_15	0x007100ac
#define NVACC_HT_HANDL_16	0x007100b0
#define NVACC_HT_VALUE_16	0x007100b4
#define NVACC_HT_HANDL_17	0x007100b8
#define NVACC_HT_VALUE_17	0x007100bc

/* acc engine fifo setup registers (for function_register 'mappings') */
#define	NVACC_FIFO_00800000	0x00800000
#define	NVACC_FIFO_00802000	0x00802000
#define	NVACC_FIFO_00804000	0x00804000
#define	NVACC_FIFO_00806000	0x00806000
#define	NVACC_FIFO_00808000	0x00808000
#define	NVACC_FIFO_0080a000	0x0080a000
#define	NVACC_FIFO_0080c000	0x0080c000
#define	NVACC_FIFO_0080e000	0x0080e000

/* ROP3 registers (Raster OPeration) */
#define NV16_ROP_FIFOFREE	0x00800010 /* little endian */
#define NVACC_ROP_ROP3		0x00800300 /* 'mapped' from 0x00420300 */

/* clip registers */
#define NV16_CLP_FIFOFREE	0x00802010 /* little endian */
#define NVACC_CLP_TOPLEFT	0x00802300 /* 'mapped' from 0x00450300 */
#define NVACC_CLP_WIDHEIGHT	0x00802304 /* 'mapped' from 0x00450304 */

/* pattern registers */
#define NV16_PAT_FIFOFREE	0x00804010 /* little endian */
#define NVACC_PAT_SHAPE		0x00804308 /* 'mapped' from 0x00460308 */
#define NVACC_PAT_COLOR0	0x00804310 /* 'mapped' from 0x00460310 */
#define NVACC_PAT_COLOR1	0x00804314 /* 'mapped' from 0x00460314 */
#define NVACC_PAT_MONO1		0x00804318 /* 'mapped' from 0x00460318 */
#define NVACC_PAT_MONO2		0x0080431c /* 'mapped' from 0x0046031c */

/* blit registers */
#define NV16_BLT_FIFOFREE	0x00808010 /* little endian */
#define NVACC_BLT_TOPLFTSRC	0x00808300 /* 'mapped' from 0x00500300 */
#define NVACC_BLT_TOPLFTDST	0x00808304 /* 'mapped' from 0x00500304 */
#define NVACC_BLT_SIZE		0x00808308 /* 'mapped' from 0x00500308 */

/* used bitmap registers */
#define NV16_BMP_FIFOFREE	0x0080a010 /* little endian */
#define NVACC_BMP_COLOR1A	0x0080a3fc /* 'mapped' from 0x006b03fc */
#define NVACC_BMP_UCRECTL_0	0x0080a400 /* 'mapped' from 0x006b0400 */
#define NVACC_BMP_UCRECSZ_0	0x0080a404 /* 'mapped' from 0x006b0404 */

/* Nvidia PCI direct registers */
#define NV32_PWRUPCTRL		0x00000200
#define NV8_MISCW 			0x000c03c2
#define NV8_MISCR 			0x000c03cc
#define NV8_SEQIND			0x000c03c4
#define NV16_SEQIND			0x000c03c4
#define NV8_SEQDAT			0x000c03c5
#define NV8_GRPHIND			0x000c03ce
#define NV16_GRPHIND		0x000c03ce
#define NV8_GRPHDAT			0x000c03cf

/* bootstrap info registers */
#define NV32_NV4STRAPINFO	0x00100000
#define NV32_PFB_CONFIG_0	0x00100200
#define NV32_NV10STRAPINFO	0x0010020c
#define NV32_NVSTRAPINFO2	0x00101000

/* primary head */
#define NV8_ATTRINDW		0x006013c0
#define NV8_ATTRDATW		0x006013c0
#define NV8_ATTRDATR		0x006013c1
#define NV8_CRTCIND			0x006013d4
#define NV16_CRTCIND		0x006013d4
#define NV8_CRTCDAT			0x006013d5
#define NV8_INSTAT1			0x006013da
#define NV32_NV10FBSTADD32	0x00600800
#define NV32_CONFIG         0x00600804//not yet used (coldstart)...
#define NV32_RASTER			0x00600808
#define NV32_NV10CURADD32	0x0060080c
#define NV32_CURCONF		0x00600810
#define NV32_FUNCSEL		0x00600860

/* secondary head */
#define NV8_ATTR2INDW		0x006033c0
#define NV8_ATTR2DATW		0x006033c0
#define NV8_ATTR2DATR		0x006033c1
#define NV8_CRTC2IND		0x006033d4
#define NV16_CRTC2IND		0x006033d4
#define NV8_CRTC2DAT		0x006033d5
#define NV8_2INSTAT1		0x006033da//verify!!!
#define NV32_NV10FB2STADD32	0x00602800//verify!!!
#define NV32_RASTER2		0x00602808//verify!!!
#define NV32_NV10CUR2ADD32	0x0060280c//verify!!!
#define NV32_2CURCONF		0x00602810//verify!!!
#define NV32_2FUNCSEL		0x00602860

/* Nvidia DAC direct registers (standard VGA palette RAM registers) */
/* primary head */
#define NV8_PALMASK			0x006813c6
#define NV8_PALINDR			0x006813c7
#define NV8_PALINDW			0x006813c8
#define NV8_PALDATA			0x006813c9
/* secondary head */
#define NV8_PAL2MASK		0x006833c6//verify!!!
#define NV8_PAL2INDR		0x006833c7//verify!!!
#define NV8_PAL2INDW		0x006833c8//verify!!!
#define NV8_PAL2DATA		0x006833c9//verify!!!

/* Nvidia PCI direct DAC registers (32bit) */
/* primary head */
#define NVDAC_CURPOS		0x00680300
#define NVDAC_PIXPLLC		0x00680508
#define NVDAC_PLLSEL		0x0068050c
#define NVDAC_GENCTRL		0x00680600
/* secondary head */
#define NVDAC2_CURPOS		0x00680b00
#define NVDAC2_PIXPLLC		0x00680d20//verify!!!
#define NVDAC2_PLLSEL		0x00680d0c//verify!!!
#define NVDAC2_GENCTRL		0x00680e00//verify!!!

/* Nvidia CRTC indexed registers */
/* VGA standard registers: */
#define NVCRTCX_HTOTAL		0x00
#define NVCRTCX_HDISPE		0x01
#define NVCRTCX_HBLANKS		0x02
#define NVCRTCX_HBLANKE		0x03
#define NVCRTCX_HSYNCS		0x04
#define NVCRTCX_HSYNCE		0x05
#define NVCRTCX_VTOTAL		0x06
#define NVCRTCX_OVERFLOW	0x07
#define NVCRTCX_PRROWSCN	0x08
#define NVCRTCX_MAXSCLIN	0x09
#define NVCRTCX_VGACURCTRL	0x0a
#define NVCRTCX_FBSTADDH	0x0c //confirmed
#define NVCRTCX_FBSTADDL	0x0d //confirmed
#define NVCRTCX_VSYNCS		0x10
#define NVCRTCX_VSYNCE		0x11
#define NVCRTCX_VDISPE		0x12
#define NVCRTCX_PITCHL		0x13 //confirmed
#define NVCRTCX_VBLANKS		0x15
#define NVCRTCX_VBLANKE		0x16
#define NVCRTCX_MODECTL		0x17
#define NVCRTCX_LINECOMP	0x18
/* Nvidia specific registers: */
#define NVCRTCX_REPAINT0	0x19
#define NVCRTCX_REPAINT1	0x1a
#define NVCRTCX_LOCK		0x1f
#define NVCRTCX_LSR			0x25
#define NVCRTCX_PIXEL		0x28
#define NVCRTCX_HEB			0x2d
#define NVCRTCX_CURCTL2		0x2f
#define NVCRTCX_CURCTL1		0x30
#define NVCRTCX_CURCTL0		0x31
#define NVCRTCX_INTERLACE	0x39
#define NVCRTCX_EXTRA		0x41

/* Nvidia ATTRIBUTE indexed registers */
/* VGA standard registers: */
#define NVATBX_MODECTL		0x10
#define NVATBX_OSCANCOLOR	0x11
#define NVATBX_COLPLANE_EN	0x12
#define NVATBX_HORPIXPAN	0x13 //confirmed
#define NVATBX_COLSEL		0x14

/* Nvidia SEQUENCER indexed registers */
/* VGA standard registers: */
#define NVSEQX_RESET		0x00
#define NVSEQX_CLKMODE		0x01
#define NVSEQX_MEMMODE		0x04

/* Nvidia GRAPHICS indexed registers */
/* VGA standard registers: */
#define NVGRPHX_ENSETRESET	0x01
#define NVGRPHX_DATAROTATE	0x03
#define NVGRPHX_READMAPSEL	0x04
#define NVGRPHX_MODE		0x05
#define NVGRPHX_MISC		0x06
#define NVGRPHX_BITMASK		0x08

/* Nvidia BES (Back End Scaler) registers (< NV10, including NV03, so RIVA128(ZX)) */
#define NVBES_NV04_INTE		0x00680140
#define NVBES_NV04_ISCALVH	0x00680200
#define NVBES_NV04_CTRL_V	0x00680204
#define NVBES_NV04_CTRL_H	0x00680208
#define NVBES_NV04_OE_STATE	0x00680224
#define NVBES_NV04_SU_STATE	0x00680228
#define NVBES_NV04_RM_STATE	0x0068022c
#define NVBES_NV04_DSTREF	0x00680230
#define NVBES_NV04_DSTSIZE	0x00680234
#define NVBES_NV04_FIFOTHRS	0x00680238
#define NVBES_NV04_FIFOBURL	0x0068023c
#define NVBES_NV04_COLKEY	0x00680240
#define NVBES_NV04_GENCTRL	0x00680244
#define NVBES_NV04_RED_AMP	0x00680280
#define NVBES_NV04_GRN_AMP	0x00680284
#define NVBES_NV04_BLU_AMP	0x00680288
#define NVBES_NV04_SAT		0x0068028c
/* buffer 0 */
#define NVBES_NV04_0BUFADR	0x0068020c
#define NVBES_NV04_0SRCPTCH	0x00680214
#define NVBES_NV04_0OFFSET	0x0068021c
/* buffer 1 */
#define NVBES_NV04_1BUFADR	0x00680210
#define NVBES_NV04_1SRCPTCH	0x00680218
#define NVBES_NV04_1OFFSET	0x00680220

/* Nvidia BES (Back End Scaler) registers (>= NV10) */
#define NVBES_NV10_INTE		0x00008140
#define NVBES_NV10_BUFSEL	0x00008700
#define NVBES_NV10_GENCTRL	0x00008704
#define NVBES_NV10_COLKEY	0x00008b00
/* buffer 0 */
#define NVBES_NV10_0BUFADR	0x00008900
#define NVBES_NV10_0MEMMASK	0x00008908
#define NVBES_NV10_0BRICON	0x00008910
#define NVBES_NV10_0SAT		0x00008918
#define NVBES_NV10_0OFFSET	0x00008920
#define NVBES_NV10_0SRCSIZE	0x00008928
#define NVBES_NV10_0SRCREF	0x00008930
#define NVBES_NV10_0ISCALH	0x00008938
#define NVBES_NV10_0ISCALV	0x00008940
#define NVBES_NV10_0DSTREF	0x00008948
#define NVBES_NV10_0DSTSIZE	0x00008950
#define NVBES_NV10_0SRCPTCH	0x00008958
/* buffer 1 */
#define NVBES_NV10_1BUFADR	0x00008904
#define NVBES_NV10_1MEMMASK	0x0000890c
#define NVBES_NV10_1BRICON	0x00008914
#define NVBES_NV10_1SAT		0x0000891c
#define NVBES_NV10_1OFFSET	0x00008924
#define NVBES_NV10_1SRCSIZE	0x0000892c
#define NVBES_NV10_1SRCREF	0x00008934
#define NVBES_NV10_1ISCALH	0x0000893c
#define NVBES_NV10_1ISCALV	0x00008944
#define NVBES_NV10_1DSTREF	0x0000894c
#define NVBES_NV10_1DSTSIZE	0x00008954
#define NVBES_NV10_1SRCPTCH	0x0000895c
/* Nvidia MPEG2 hardware decoder (GeForce4MX only) */
#define NVBES_DEC_GENCTRL	0x00001588

/* NV 2nd CRTC registers (>= G400) */
#define NVCR2_CTL           0x3C10
#define NVCR2_HPARAM        0x3C14
#define NVCR2_HSYNC         0x3C18
#define NVCR2_VPARAM        0x3C1C
#define NVCR2_VSYNC         0x3C20
#define NVCR2_PRELOAD       0x3C24
#define NVCR2_STARTADD0     0x3C28
#define NVCR2_STARTADD1     0x3C2C
#define NVCR2_OFFSET        0x3C40
#define NVCR2_MISC          0x3C44
#define NVCR2_VCOUNT        0x3C48
#define NVCR2_DATACTL       0x3C4C

/*MAVEN registers (<= G400) */
#define NVMAV_PGM            0x3E
#define NVMAV_PIXPLLM        0x80
#define NVMAV_PIXPLLN        0x81
#define NVMAV_PIXPLLP        0x82
#define NVMAV_GAMMA1         0x83
#define NVMAV_GAMMA2         0x84
#define NVMAV_GAMMA3         0x85
#define NVMAV_GAMMA4         0x86
#define NVMAV_GAMMA5         0x87
#define NVMAV_GAMMA6         0x88
#define NVMAV_GAMMA7         0x89
#define NVMAV_GAMMA8         0x8A
#define NVMAV_GAMMA9         0x8B
#define NVMAV_MONSET         0x8C
#define NVMAV_TEST           0x8D
#define NVMAV_WREG_0X8E_L    0x8E
#define NVMAV_WREG_0X8E_H    0x8F
#define NVMAV_HSCALETV       0x90
#define NVMAV_TSCALETVL      0x91
#define NVMAV_TSCALETVH      0x92
#define NVMAV_FFILTER        0x93
#define NVMAV_MONEN          0x94
#define NVMAV_RESYNC         0x95
#define NVMAV_LASTLINEL      0x96
#define NVMAV_LASTLINEH      0x97
#define NVMAV_WREG_0X98_L    0x98
#define NVMAV_WREG_0X98_H    0x99
#define NVMAV_HSYNCLENL      0x9A
#define NVMAV_HSYNCLENH      0x9B
#define NVMAV_HSYNCSTRL      0x9C
#define NVMAV_HSYNCSTRH      0x9D
#define NVMAV_HDISPLAYL      0x9E
#define NVMAV_HDISPLAYH      0x9F
#define NVMAV_HTOTALL        0xA0
#define NVMAV_HTOTALH        0xA1
#define NVMAV_VSYNCLENL      0xA2
#define NVMAV_VSYNCLENH      0xA3
#define NVMAV_VSYNCSTRL      0xA4
#define NVMAV_VSYNCSTRH      0xA5
#define NVMAV_VDISPLAYL      0xA6
#define NVMAV_VDISPLAYH      0xA7
#define NVMAV_VTOTALL        0xA8
#define NVMAV_VTOTALH        0xA9
#define NVMAV_HVIDRSTL       0xAA
#define NVMAV_HVIDRSTH       0xAB
#define NVMAV_VVIDRSTL       0xAC
#define NVMAV_VVIDRSTH       0xAD
#define NVMAV_VSOMETHINGL    0xAE
#define NVMAV_VSOMETHINGH    0xAF
#define NVMAV_OUTMODE        0xB0
#define NVMAV_LOCK           0xB3
#define NVMAV_LUMA           0xB9
#define NVMAV_VDISPLAYTV     0xBE
#define NVMAV_STABLE         0xBF
#define NVMAV_HDISPLAYTV     0xC2
#define NVMAV_BREG_0XC6      0xC6

//new:
/* Macros for convenient accesses to the NV chips */
#define NV_REG8(r_)  ((vuint8  *)regs)[(r_)]
#define NV_REG16(r_) ((vuint16 *)regs)[(r_) >> 1]
#define NV_REG32(r_) ((vuint32 *)regs)[(r_) >> 2]

/* read and write to PCI config space */
#define CFGR(A)   (nv_pci_access.offset=NVCFG_##A, ioctl(fd,NV_GET_PCI, &nv_pci_access,sizeof(nv_pci_access)), nv_pci_access.value)
#define CFGW(A,B) (nv_pci_access.offset=NVCFG_##A, nv_pci_access.value = B, ioctl(fd,NV_SET_PCI,&nv_pci_access,sizeof(nv_pci_access)))

/* read and write from the dac registers */
#define DACR(A)   (NV_REG32(NVDAC_##A))
#define DACW(A,B) (NV_REG32(NVDAC_##A)=B)
#define DAC2R(A)   (NV_REG32(NVDAC2_##A))
#define DAC2W(A,B) (NV_REG32(NVDAC2_##A)=B)

/* read and write from the backend scaler registers */
#define BESR(A)   (NV_REG32(NVBES_##A))
#define BESW(A,B) (NV_REG32(NVBES_##A)=B)

/* read and write from CRTC indexed registers */
#define CRTCW(A,B)(NV_REG16(NV16_CRTCIND) = ((NVCRTCX_##A) | ((B) << 8)))
#define CRTCR(A)  (NV_REG8(NV8_CRTCIND) = (NVCRTCX_##A), NV_REG8(NV8_CRTCDAT))

/* read and write from second CRTC indexed registers */
#define CRTC2W(A,B)(NV_REG16(NV16_CRTC2IND) = ((NVCRTCX_##A) | ((B) << 8)))
#define CRTC2R(A)  (NV_REG8(NV8_CRTC2IND) = (NVCRTCX_##A), NV_REG8(NV8_CRTC2DAT))

/* read and write from ATTRIBUTE indexed registers */
#define ATBW(A,B)(NV_REG8(NV8_INSTAT1), NV_REG8(NV8_ATTRINDW) = ((NVATBX_##A) | 0x20), NV_REG8(NV8_ATTRDATW) = (B))
#define ATBR(A)  (NV_REG8(NV8_INSTAT1), NV_REG8(NV8_ATTRINDW) = ((NVATBX_##A) | 0x20), NV_REG8(NV8_ATTRDATR))

/* read and write from ATTRIBUTE indexed registers */
#define ATB2W(A,B)(NV_REG8(NV8_INSTAT1), NV_REG8(NV8_ATTR2INDW) = ((NVATBX_##A) | 0x20), NV_REG8(NV8_ATTR2DATW) = (B))
#define ATB2R(A)  (NV_REG8(NV8_INSTAT1), NV_REG8(NV8_ATTR2INDW) = ((NVATBX_##A) | 0x20), NV_REG8(NV8_ATTR2DATR))

/* read and write from SEQUENCER indexed registers */
#define SEQW(A,B)(NV_REG16(NV16_SEQIND) = ((NVSEQX_##A) | ((B) << 8)))
#define SEQR(A)  (NV_REG8(NV8_SEQIND) = (NVSEQX_##A), NV_REG8(NV8_SEQDAT))

/* read and write from PCI GRAPHICS indexed registers */
#define GRPHW(A,B)(NV_REG16(NV16_GRPHIND) = ((NVGRPHX_##A) | ((B) << 8)))
#define GRPHR(A)  (NV_REG8(NV8_GRPHIND) = (NVGRPHX_##A), NV_REG8(NV8_GRPHDAT))

/* read and write from the acceleration engine registers */
#define ACCR(A)    (NV_REG32(NVACC_##A))
#define ACCW(A,B)  (NV_REG32(NVACC_##A)=B)
//end new.

/* read and write from maven (<= G400) */
#define MAVR(A)     (i2c_maven_read (NVMAV_##A ))
#define MAVW(A,B)   (i2c_maven_write(NVMAV_##A ,B))
#define MAVRW(A)    (i2c_maven_read (NVMAV_##A )|(i2c_maven_read(NVMAV_##A +1)<<8))
#define MAVWW(A,B)  (i2c_maven_write(NVMAV_##A ,B &0xFF),i2c_maven_write(NVMAV_##A +1,B >>8))
