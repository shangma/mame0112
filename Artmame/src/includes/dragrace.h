/*************************************************************************

    Atari Drag Race hardware

*************************************************************************/

#include "sound/discrete.h"

/* Discrete Sound Input Nodes */
#define DRAGRACE_SCREECH1_EN	NODE_01
#define DRAGRACE_SCREECH2_EN	NODE_02
#define DRAGRACE_LOTONE_EN		NODE_03
#define DRAGRACE_HITONE_EN		NODE_04
#define DRAGRACE_EXPLODE1_EN	NODE_05
#define DRAGRACE_EXPLODE2_EN	NODE_06
#define DRAGRACE_MOTOR1_DATA	NODE_07
#define DRAGRACE_MOTOR2_DATA	NODE_08
#define DRAGRACE_MOTOR1_EN		NODE_80
#define DRAGRACE_MOTOR2_EN		NODE_81
#define DRAGRACE_KLEXPL1_EN		NODE_82
#define DRAGRACE_KLEXPL2_EN		NODE_83
#define DRAGRACE_ATTRACT_EN		NODE_09


/*----------- defined in sndhrdw/dragrace.c -----------*/

extern discrete_sound_block dragrace_discrete_interface[];

/*----------- defined in vidhrdw/dragrace.c -----------*/

VIDEO_START( dragrace );
VIDEO_UPDATE( dragrace );

extern UINT8* dragrace_playfield_ram;
extern UINT8* dragrace_position_ram;
