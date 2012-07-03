/******************************************************************************

    agedriv.c

    Copyright (c) 1996-2007, Nicola Salmoria and the MAME Team. Used under license
    Visit http://mamedev.org for licensing and usage restrictions.

    The list of all available drivers. Drivers have to be included here to be
    recognized by the executable.

    To save some typing, we use a hack here. This file is recursively #included
    twice, with different definitions of the DRIVER() macro. The first one
    declares external references to the drivers; the second one builds an array
    storing all the drivers.

******************************************************************************/

#include "driver.h"


#ifndef DRIVER_RECURSIVE

#define DRIVER_RECURSIVE

/* step 1: declare all external references */
#define DRIVER(NAME) extern game_driver driver_##NAME;
#include "agedriv.c"

/* step 2: define the drivers[] array */
#undef DRIVER
#define DRIVER(NAME) &driver_##NAME,
const game_driver * const drivers[] =
{
#include "agedriv.c"
	0	/* end of array */
};

#else	/* DRIVER_RECURSIVE */

//Ace Coin Electronics (ACE)/////////////////////////////////////////
DRIVER( starspnr )  /* ACE starspinner - highly prelim */

//AWP/ABM////////////////////////////////////////////////////////////
DRIVER( magic10 )
DRIVER( magic10a )
DRIVER( magic102 )

//Barcrest///////////////////////////////////////////////////////////
//MPU3/4 Games
DRIVER( oldtimer )  /* Barcrest - highly prelim*/
DRIVER( ccelbrtn )

DRIVER( connect4 )  /* Dolbeck Systems */
DRIVER( bctvidbs )  /* Barcrest - highly prelim*/
DRIVER( crmaze )    /* Barcrest - highly prelim*/
DRIVER( crmazea )   /* Barcrest - highly prelim*/
DRIVER( crmazeb )   /* Barcrest - highly prelim*/
DRIVER( mating )    /* Barcrest - highly prelim*/
DRIVER( matinga )   /* Barcrest - highly prelim*/
DRIVER( skiltrek )  /* Barcrest - highly prelim*/
DRIVER( turnover )  /* Barcrest - highly prelim*/

//Bellfruit Manufacturing (BFM) and partners/////////////////////////
// System85 games 
DRIVER( bfmsc271 )  /* (c) 198? BFM */
// Scorpion1 games
DRIVER( bfmlotse )  /* (c) 198? BFM Dutch ROMS, prelim*/
DRIVER( bfmroul  )  /* (c) 198? BFM Dutch ROMS, prelim*/
DRIVER( bfmesc   )  /* (c) 19?? BFM Every second counts */
DRIVER( bfmclatt )  /* (c) 1990 BFM,     Game Card 39-370-196*/
DRIVER( toppoker )  /* (c) 1996 BFM/ELAM,Game Card 95-750-899, uses Adder board for feature gfx*/
// Scorpion2 games
DRIVER( bfmdrwho  ) /* (c) 1994 BFM, Game Card 95-750-288*/
DRIVER( bfmcgslm )  /* (c) 1996 BFM, Game Card 95-750-843*/
DRIVER( cpeno1  )   /* (c) 1996 BFM, Game Card 95-750-846*/
DRIVER( luvjub  )   /* (c) 1996 BFM, Game Card 95-750-808*/

// Adder games                              PROM A        GAME CARD
DRIVER( qntoondo  )  /* (c) 1994 BFM/ELAM   PR6097V1      95-750-136*/
DRIVER( quintoon  )  /* (c) 1993 BFM UK     QUIN V3       95-750-203*/
DRIVER( qntoond   )  /* (c) 1994 BFM/ELAM   PR6097V1      95-750-243*/
DRIVER( pokio     )  /* (c) 1994 BFM/ELAM   PR6343V1      95-750-278*/
DRIVER( paradice  )  /* (c) 1995 BFM/ELAM   PR6444V2      95-750-615*/
DRIVER( pyramid   )  /* (c) 1996 BFM/ELAM   PR6644V5      95-750-898*/
DRIVER( slotsnl   )  /* (c) 1995 BFM/ELAM   PR6349V1      95-750-368*/
DRIVER( sltblgtk  )  /* (c) 1996 BFM/ELAM   PR6496V2      95-750-943*/
DRIVER( sltblgpo  )  /* (c) 1996 BFM/ELAM   PR6495V5      95-750-938   Belgian slots cash payout*/
DRIVER( sltblgp1  )  /* (c) 1996 BFM/ELAM   PR6495V5      95-752-008   Belgian slots cash payout*/
DRIVER( gldncrwn  )  /* (c) 1997 BFM/ELAM   PR6750V1      95-752-011*/

// Scorpion3 (scorpion2 casino) games
DRIVER( bfmfocus  )  /* (c) 1995 BFM/ELAM, Game Card 95-750-347, prelim*/
// Scorpion4 games 
DRIVER( thegame   )	 /* (c) 199? BFM/EUROCOIN Dutch ROMS, prelim*/ 
DRIVER( maz_pole  )	 /* (c) 199? Mazooma Dutch ROMS, prelim*/

//BS Electronics//////////////////////////////////////////////////////
DRIVER( carrera )	/* (c) 19?? BS Electronics */

//BrezzaSoft//////////////////////////////////////////////////////////
DRIVER( neogeo )
DRIVER( vliner )
DRIVER( vlinero )

//Casino Electronics Inc//////////////////////////////////////////////
DRIVER( vp906iii )   /* (c) 1985 Casino Electronics Inc.*/

//Coinmaster////////////////////////////////////////////////////////
DRIVER( quizmstr )   /* (c) 1985 Coinmaster */
DRIVER( trailblz )   /* (c) 1987 Coinmaster */
DRIVER( supnudg2 )   /* (c) 1989 Coinmaster */

//Corsica/////////////////////////////////////////////////////////////
DRIVER( cmv801 )     /* (c) 198? Corsica */

//Dyna Electronics////////////////////////////////////////////////////
DRIVER( cm2v841 )    /* (c) 198? Dyna Electronics */
DRIVER( cm2841a )    /* (c) 198? Dyna Electronics */
DRIVER( rcasino )    /* (c) 1984 Dyna Electronics */

//Electrocoin/////////////////////////////////////////////////////////
DRIVER( dmndrby )	/* (c) 1986 Electrocoin */
DRIVER( dmndrbya )	/* (c) 1986 Electrocoin */

//Electronic Products/////////////////////////////////////////////////
DRIVER( jackpool )

//Falcon//////////////////////////////////////////////////////////////
DRIVER( lucky8 )     /* (c) 1989 Falcon */

//Fun World hardware//////////////////////////////////////////////////
DRIVER( cuoreuno )	/* (c) 1996 C.M.C. */
DRIVER( elephfam )	/* (c) 1996 C.M.C. */
DRIVER( elephfmb )	/* (c) 1996 C.M.C. */
DRIVER( pool10 )	/* (c) 1996 C.M.C. */
DRIVER( pool10b )	/* (c) 1996 C.M.C. */
DRIVER( tortufam )	/* (c) 1997 C.M.C. */

DRIVER( bigdeal  )  /* (c) 1986 Fun World */
DRIVER( bigdealb )  /* (c) 1986 Fun World */

DRIVER( jollycrd )  /* (c) 1985 TAB Austria */
DRIVER( jolycdae )	/* (c) 1985 TAB-Austria */
DRIVER( jolycdcr )	/* (c) 1993 Soft Design */
DRIVER( jolycdit )	/* 199? bootleg */
DRIVER( jolycdat )	/* (c) 1986 Fun World */
DRIVER( jolycdab )	/* (c) 1990 Inter Games */

DRIVER( royalcrd )  /* (c) 1991 TAB-Austria */
DRIVER( royalcdb )  /* (c) 1991 TAB-Austria */
DRIVER( royalcdc )	/* (c) 199? Evona Electronic */

DRIVER( magiccrd )  /* (c) 1996 'JEZ' */
DRIVER( jokercrd )	/* (c) 1993 Vesely Svet */

DRIVER( monglfir )	/* (c) 199? bootleg */
DRIVER( soccernw )	/* (c) 199? bootleg */

//Greyhound Electronics///////////////////////////////////////////////
DRIVER( m075 )		/* (c) 1982 Greyhound Electronics */
DRIVER( superbwl )	/* (c) 1982 Greyhound Electronics */
DRIVER( gs4002 )	/* (c) 1982 G.E.I. */
DRIVER( gs4002a )	/* (c) 1982 G.E.I. */

DRIVER( gepoker )	/* (c) 1984 Greyhound Electronics */
DRIVER( gepoker1 )	/* (c) 1984 Greyhound Electronics */
DRIVER( gepoker2 )	/* (c) 1984 Greyhound Electronics */
DRIVER( gepoker3 )	/* (c) 1984 Greyhound Electronics */

//IGS/////////////////////////////////////////////////////////////////
DRIVER( csk227it )   /* (c) 198? IGS */
DRIVER( csk234it )   /* (c) 198? IGS */
DRIVER( goldstar )   /* (c) 198? IGS */
DRIVER( goldstbl )   /* (c) 198? IGS */
DRIVER( moonlght )   /* bootleg */

//Maygay//////////////////////////////////////////////////////////////

//Merit///////////////////////////////////////////////////////////////
DRIVER( pitboss2 )	/* (c) 1988 Merit */
DRIVER( spitboss )	/* (c) 198? Merit */
DRIVER( pitbossm )	/* (c) 1994 Merit */
DRIVER( megat3 )	/* (c) 1995 Merit */
DRIVER( megat5 )	/* (c) 1997 Merit */

//Midway Touchmaster games////////////////////////////////////////////
DRIVER( tm )		/* (c) 1996 Midway Games */
DRIVER( tm3k )		/* (c) 1996 Midway Games */
DRIVER( tm4k )		/* (c) 1996 Midway Games */

//Novomatic///////////////////////////////////////////////////////////
DRIVER( ampoker2 )   /* (c) 198? */
DRIVER( ampokr2a )   /* (c) 198? */
DRIVER( ampokr2b )   /* (c) 198? */
DRIVER( ampokr2c )   /* (c) 198? */

//Playmark////////////////////////////////////////////////////////////
DRIVER( sderby )     /* (c) 1996 Playmark */
DRIVER( pmroulet )   /* (c) 1997 Playmark */
DRIVER( magicstk )   /* (c) 1997 Playmark */

//Sega////////////////////////////////////////////////////////////////
DRIVER( witch )     /* (c) 1992 Sega / Vic Tokai / Excellent Systems */

//Sigma///////////////////////////////////////////////////////////////
DRIVER( luctoday )   /* (c) 1980 Sigma */
DRIVER( chewing )    /* Unknown */

//Tehkan//////////////////////////////////////////////////////////////
DRIVER( lvpoker )    /* (c) 1985 Tehkan */
DRIVER( ponttehk )   /* (c) 1985 Tehkan */

//Veltmeijer//////////////////////////////////////////////////////////
DRIVER( cardline )   /* (c) 199? Veltmeijer */
DRIVER( pbchmp95 )   /* (c) 1995 Veltmeijer Automaten */

//World Game//////////////////////////////////////////////////////////
DRIVER( vroulet )    /* (c) 1989 World Game */

//Zilec - Zenitone////////////////////////////////////////////////////
DRIVER( merlinmm )  /* (c) 1986 Zilec-Zenitone */
DRIVER( cashquiz )	/* (c) 1986 Zilec-Zenitone */
//Unknown/////////////////////////////////////////////////////////////
DRIVER( fortecar )
DRIVER( murogem )	/* (c) 198? */
DRIVER( murogema )	/* ??? */
DRIVER( lasvegas )	/* hack */
DRIVER( pmpoker )	/* (c) 198? PlayMan?*/
DRIVER( goldnpkr )	/* (c) 198? Bonanza?*/
DRIVER( goldnpkb )	/* (c) 1981 Bonanza */
DRIVER( jokerpkr )	/* (c) 198? Coinmaster-Gaming*/
DRIVER( pottnpkr )	/* (c) 198? ????*/
//Bootlegs////////////////////////////////////////////////////////////
DRIVER( darkhors )	/* (c) 2001 bootleg of Jockey Club II */
DRIVER( stellecu )	/* (c) 1998 */
#endif	/* DRIVER_RECURSIVE */
