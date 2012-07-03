###########################################################################
#
#   age.mak
#
#   AGEMAME target makefile
#
#   Code used under license by AGEMAME Development.
#   Copyright (c) 1996-2006, Nicola Salmoria and the MAME Team.
#   Visit http://mamedev.org for licensing and usage restrictions.
#
###########################################################################

#-------------------------------------------------
# specify available CPU cores; some of these are
# unnecessary and so aren't included
#-------------------------------------------------

CPUS += Z80
#CPUS += Z180
#CPUS += 8080
CPUS += 8085A
CPUS += M6502
CPUS += M65C02
CPUS += M65SC02
#CPUS += M65CE02
#CPUS += M6509
#CPUS += M6510
#CPUS += M6510T
#CPUS += M7501
#CPUS += M8502
#CPUS += N2A03
#CPUS += DECO16
#CPUS += M4510
#CPUS += H6280
#CPUS += I86
#CPUS += I88
#CPUS += I186
#CPUS += I188
#CPUS += I286
#CPUS += V20
#CPUS += V30
#CPUS += V33
#CPUS += V60
#CPUS += V70
#CPUS += I8035
#CPUS += I8039
#CPUS += I8048
#CPUS += N7751
#CPUS += I8X41
CPUS += I8051
CPUS += I8052
CPUS += I8751
CPUS += I8752
#CPUS += M6800
#CPUS += M6801
CPUS += M6802
#CPUS += M6803
CPUS += M6808
#CPUS += HD63701
#CPUS += NSC8105
#CPUS += M6805
#CPUS += M68705
#CPUS += HD63705
CPUS += HD6309
CPUS += M6809
CPUS += M6809E
#CPUS += KONAMI
CPUS += M68000
#CPUS += M68010
CPUS += M68EC020
#CPUS += M68020
#CPUS += M68040
#CPUS += T11
#CPUS += S2650
#CPUS += TMS34010
#CPUS += TMS34020
#CPUS += TMS9900
#CPUS += TMS9940
#CPUS += TMS9980
#CPUS += TMS9985
#CPUS += TMS9989
#CPUS += TMS9995
#CPUS += TMS99105A
#CPUS += TMS99110A
#CPUS += Z8000
#CPUS += TMS32010
#CPUS += TMS32025
#CPUS += TMS32026
#CPUS += TMS32031
#CPUS += TMS32051
#CPUS += CCPU
#CPUS += ADSP2100
#CPUS += ADSP2101
#CPUS += ADSP2104
#CPUS += ADSP2105
#CPUS += ADSP2115
#CPUS += ADSP2181
#CPUS += PSXCPU
#CPUS += ASAP
#CPUS += UPD7810
#CPUS += UPD7807
#CPUS += ARM
#CPUS += ARM7
#CPUS += JAGUAR
#CPUS += R3000
#CPUS += R4600
#CPUS += R4650
#CPUS += R4700
#CPUS += R5000
#CPUS += QED5271
#CPUS += RM7000
#CPUS += SH2
#CPUS += DSP32C
#CPUS += PIC16C54
#CPUS += PIC16C55
#CPUS += PIC16C56
#CPUS += PIC16C57
#CPUS += PIC16C58
#CPUS += G65816
#CPUS += SPC700
#CPUS += E116T
#CPUS += E116XT
#CPUS += E116XS
#CPUS += E116XSR
#CPUS += E132N
#CPUS += E132T
#CPUS += E132XN
#CPUS += E132XT
#CPUS += E132XS
#CPUS += E132XSR
#CPUS += GMS30C2116
#CPUS += GMS30C2132
#CPUS += GMS30C2216
#CPUS += GMS30C2232
#CPUS += I386
#CPUS += I486
#CPUS += PENTIUM
#CPUS += MEDIAGX
#CPUS += I960
CPUS += H83002 
#CPUS += V810
#CPUS += M37702
#CPUS += M37710
#CPUS += PPC403
#CPUS += PPC602
#CPUS += PPC603
#CPUS += SE3208
#CPUS += MC68HC11
#CPUS += ADSP21062
#CPUS += DSP56156
#CPUS += RSP
#CPUS += ALPHA8201
#CPUS += ALPHA8301
#CPUS += CDP1802
#CPUS += COP420
#CPUS += COP410
#CPUS += TLCS90
# CPUS += APEXC
# CPUS += CP1610
# CPUS += F8
# CPUS += LH5801
# CPUS += PDP1
# CPUS += SATURN
# CPUS += SC61860
# CPUS += TX0_64KW
# CPUS += TX0_8KW
# CPUS += Z80GB
# CPUS += TMS7000
# CPUS += TMS7000_EXL
# CPUS += SM8500
# CPUS += V30MZ


#-------------------------------------------------
# specify available sound cores; some of these are
# unnecessary and so aren't included
#-------------------------------------------------

SOUNDS += CUSTOM
SOUNDS += SAMPLES
SOUNDS += DAC
#SOUNDS += DMADAC
#SOUNDS += DISCRETE
SOUNDS += AY8910
SOUNDS += YM2203
SOUNDS += YM2151
#SOUNDS += YM2608
SOUNDS += YM2610
SOUNDS += YM2610B
#SOUNDS += YM2612
SOUNDS += YM3438
SOUNDS += YM2413
#SOUNDS += YM3812
SOUNDS += YMZ280B
#SOUNDS += YM3526
#SOUNDS += Y8950
SOUNDS += SN76477
SOUNDS += SN76496
#SOUNDS += POKEY
#SOUNDS += TIA
#SOUNDS += NES
#SOUNDS += ASTROCADE
#SOUNDS += NAMCO
#SOUNDS += NAMCO_15XX
#SOUNDS += NAMCO_CUS30
#SOUNDS += NAMCO_52XX
#SOUNDS += NAMCO_54XX
#SOUNDS += NAMCO_63701X
#SOUNDS += NAMCONA
#SOUNDS += TMS36XX
#SOUNDS += TMS3615
#SOUNDS += TMS5110
#SOUNDS += TMS5220
#SOUNDS += VLM5030
SOUNDS += ADPCM
SOUNDS += OKIM6295
SOUNDS += MSM5205
SOUNDS += MSM5232
SOUNDS += UPD7759
#SOUNDS += HC55516
#SOUNDS += K005289
#SOUNDS += K007232
#SOUNDS += K051649
#SOUNDS += K053260
#SOUNDS += K054539
#SOUNDS += SEGAPCM
SOUNDS += RF5C68
#SOUNDS += CEM3394
#SOUNDS += C140
#SOUNDS += QSOUND
SOUNDS += SAA1099
#SOUNDS += IREMGA20
#SOUNDS += ES5503
#SOUNDS += ES5505
#SOUNDS += ES5506
#SOUNDS += BSMT2000
#SOUNDS += YMF262
SOUNDS += YMF278B
#SOUNDS += GAELCO_CG1V
#SOUNDS += GAELCO_GAE1
#SOUNDS += X1_010
#SOUNDS += MULTIPCM
#SOUNDS += C6280
#SOUNDS += SP0250
#SOUNDS += SCSP
SOUNDS += YMF271
#SOUNDS += PSXSPU
#SOUNDS += CDDA
#SOUNDS += ICS2115
#SOUNDS += ST0016
#SOUNDS += C352
#SOUNDS += VRENDER0
#SOUNDS += VOTRAX
SOUNDS += ES8712
#SOUNDS += RF5C400
#SOUNDS += SPEAKER
#SOUNDS += CDP1869
#SOUNDS += S14001A
#SOUNDS += BEEP
#SOUNDS += WAVE
#SOUNDS += SID6581
#SOUNDS += SID8580
#SOUNDS += SP0256


#-------------------------------------------------
# this is the list of driver libraries that
# comprise AGEMAME
#-------------------------------------------------

DRVLIBS = \
	$(OBJ)/agemame/agedriv.o \
	$(OBJ)/AGE.a \
	$(OBJ)/mameimpt.a \
	$(OBJ)/AGEshared.a \
	$(OBJ)/shared.a \

$(OBJ)/AGE.a: \
	$(OBJ)/x86drc.o \
	$(OBJ)/agemame/vidhrdw/awpvid.o \
	$(OBJ)/agemame/vidhrdw/bfm_dm01.o \
	$(OBJ)/agemame/drivers/bfm_sc1.o \
	$(OBJ)/agemame/drivers/bfm_sc2.o \
	$(OBJ)/agemame/drivers/bfm_sc4.o \
	$(OBJ)/agemame/drivers/bfmsys85.o \
	$(OBJ)/agemame/drivers/gepoker.o \
	$(OBJ)/agemame/drivers/gtuk.o \
	$(OBJ)/vidhrdw/galaxian.o $(OBJ)/sndhrdw/galaxian.o $(OBJ)/agemame/drivers/luctoday.o \
	$(OBJ)/vidhrdw/lvcards.o $(OBJ)/agemame/drivers/lvpoker.o \
	$(OBJ)/agemame/drivers/magicstk.o \
	$(OBJ)/drivers/meritm.o \
	$(OBJ)/drivers/tmaster.o \
	$(OBJ)/agemame/drivers/mpu4.o \
	$(OBJ)/vidhrdw/pingpong.o $(OBJ)/agemame/drivers/merlinmm.o \
	$(OBJ)/machine/neogeo.o \
	$(OBJ)/machine/neocrypt.o $(OBJ)/machine/neoprot.o $(OBJ)/machine/neoboot.o \
	$(OBJ)/vidhrdw/neogeo.o $(OBJ)/agemame/drivers/neogeo.o \


$(OBJ)/mameimpt.a: \
	$(OBJ)/drivers/ampoker.o \
	$(OBJ)/vidhrdw/bfm_adr2.o \
	$(OBJ)/drivers/cardline.o \
	$(OBJ)/drivers/carrera.o \
	$(OBJ)/drivers/cherrym.o \
	$(OBJ)/drivers/cherrym2.o \
	$(OBJ)/drivers/coinmstr.o \
	$(OBJ)/vidhrdw/csk.o $(OBJ)/drivers/csk.o \
	$(OBJ)/drivers/darkhors.o \
	$(OBJ)/drivers/dmndrby.o \
	$(OBJ)/drivers/fortecar.o \
	$(OBJ)/drivers/funworld.o $(OBJ)/vidhrdw/funworld.o \
	$(OBJ)/vidhrdw/goldstar.o $(OBJ)/drivers/goldstar.o \
	$(OBJ)/drivers/jackpool.o \
	$(OBJ)/drivers/lucky8.o \
	$(OBJ)/drivers/magic10.o \
	$(OBJ)/drivers/murogem.o \
	$(OBJ)/drivers/pmpoker.o \
	$(OBJ)/drivers/rcasino.o \
	$(OBJ)/vidhrdw/sderby.o $(OBJ)/drivers/sderby.o \
	$(OBJ)/drivers/starspnr.o \
	$(OBJ)/drivers/vp906iii.o \
	$(OBJ)/drivers/vroulet.o \
	$(OBJ)/drivers/witch.o \


#-------------------------------------------------
# the following files are general components and
# shared across a number of drivers
#-------------------------------------------------
$(OBJ)/AGEshared.a: \
	$(OBJ)/machine/steppers.o \
	$(OBJ)/machine/lamps.o \
	$(OBJ)/machine/vacfdisp.o \
	$(OBJ)/machine/mmtr.o \

$(OBJ)/shared.a: \
	$(OBJ)/machine/6522via.o \
	$(OBJ)/machine/6821pia.o \
	$(OBJ)/machine/6840ptm.o \
	$(OBJ)/machine/7474.o \
	$(OBJ)/machine/74123.o \
	$(OBJ)/machine/74148.o \
	$(OBJ)/machine/74153.o \
	$(OBJ)/machine/74181.o \
	$(OBJ)/machine/8255ppi.o \
	$(OBJ)/machine/adc083x.o \
	$(OBJ)/machine/pd4990a.o \
 	$(OBJ)/machine/smc91c9x.o \
 	$(OBJ)/machine/ticket.o \
	$(OBJ)/machine/z80ctc.o \
	$(OBJ)/machine/z80pio.o \
	$(OBJ)/machine/z80sio.o \
	$(OBJ)/vidhrdw/v9938.o \
	$(OBJ)/vidhrdw/crtc6845.o \
	$(OBJ)/vidhrdw/res_net.o \



#-------------------------------------------------
# layout dependencies
#-------------------------------------------------

$(OBJ)/agemame/drivers/bfm_sc1.o:	$(OBJ)/agemame/layout/bfm_sc1.lh

$(OBJ)/agemame/drivers/bfm_sc2.o:		$(OBJ)/layout/bfm_sc2.lh \
						$(OBJ)/layout/gldncrwn.lh \
						$(OBJ)/layout/quintoon.lh \
						$(OBJ)/layout/paradice.lh \
						$(OBJ)/layout/pyramid.lh \
						$(OBJ)/layout/pokio.lh \
						$(OBJ)/layout/slots.lh \
						$(OBJ)/layout/sltblgpo.lh \
						$(OBJ)/layout/sltblgtk.lh \
						$(OBJ)/agemame/layout/awpdmd.lh \
						$(OBJ)/agemame/layout/drwho.lh

$(OBJ)/drivers/cardline.o:		$(OBJ)/layout/cardline.lh

$(OBJ)/drivers/funworld.o:		$(OBJ)/layout/funworld.lh

$(OBJ)/agemame/drivers/mpu4.o:		$(OBJ)/layout/mpu4.lh
$(OBJ)/agemame/vidhrdw/awpvid.o:	$(OBJ)/agemame/layout/awpvid.lh
