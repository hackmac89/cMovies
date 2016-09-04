/*
File: cmovies.h
Synopsis: header file for the main application (structure & function prototypes)
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 08/23/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies
*/
#ifndef CMOVIES_H
#define CMOVIES_H

// OTHER
// DEBUG
#define DEBUGMODE 
#define VERSION "20160904"

// IMPORTS
#include <assert.h>   // for "assert" within the "free***Context" functions
#include <ctype.h>   // for "toupper" !
// import own headers
#include "dbutils.h"
#include "log.h"

// other function prototypes
/* initialization routines */
ctx_movieInfo* initMovieContext(void);
ctx_seriesInfo* initSeriesContext(void);
/* freeing routines */
void freeMovieContext(ctx_movieInfo **);
void freeSeriesContext(ctx_seriesInfo **);
/* information-to-database routines */
static char *setGenre(TGenres);
static char *setQuality(TQualities);
static bool addBasicInfoToMovieContext(ctx_movieInfo **, char *, const char *, char *, char *, char *, char *,
	unsigned short, unsigned short, unsigned short, bool, bool);
static void addDirectorToMovieContext(ctx_movieInfo **, char *);
static void addActorToMovieContext(ctx_movieInfo **, char *);
static bool addBasicInfoToSeriesContext(ctx_seriesInfo **, char *, const char *, char *, char *, char *,
	unsigned short, unsigned short, unsigned short, unsigned short, bool, bool);
static void addActorToSeriesContext(ctx_seriesInfo **, char *);
static /*void*/ ctx_movieInfo* reinitializeMovieContext(ctx_movieInfo **);

//...
#endif
