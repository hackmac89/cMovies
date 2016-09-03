#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <assert.h>
#include <ctype.h>   // für "toupper"

// SONSTIGES
//#define DEBUGMODE
typedef int bool;
#define true 1
#define false 0

// GLOBALE VARIABLEN
sqlite3 *movieDatabase;   // Unsere Datenbank
sqlite3_stmt *stmtSQL = NULL;   // Statement, welches anhand einer SQL-Query vorbereitet wird ("prepare" und "bind" irgendwie) 

// UNSERE KONTEXTE
typedef struct {
  // Title, genre, jahr, runtime, plot, quelle, rating, comRating, date, archived, director, actors
  char *title;
  char *genre;
  unsigned short year;
  char *runtime;
  char *plot;
  char *sourceQuality;
  unsigned short myRating;
  unsigned short communityRating;
  //time_t entryDate;   as specified in our db-spec, SQLite inserts the current date automatically ;-) 
  bool alreadySeen;
  bool isFavourite;
  char *archived;
  char *directors[3];   // allow up to 2 directors (plus NULL terminator)
  char *actors[11];   // allow up to 10 actors (plus NULL terminator)
  unsigned short cntDirectors;
  unsigned short cntActors;
} ctx_movieInfo;   // our movie context (needed for "insertMovie"-function)
typedef struct {
    // Title, genre, Season, jahr, plot, quelle, rating, comRating, date, archived, actors
    char *title;
    char *genre;
    unsigned short season;
    unsigned short year;
    char *plot;
    char *sourceQuality;
    unsigned short myRating;
    unsigned short communityRating;
    //time_t entryDate;   as specified in our db-spec, SQLite inserts the current date automatically ;-)
    bool alreadySeen;
    bool isFavourite;
    char *archived;
    char *actors[11];   // allow up to 10 actors (plus NULL terminator)
    unsigned short cntActors;
} ctx_seriesInfo;   // our series context (needed for "insertSeries"-function)
//typedef enum {MOVIECTX, SERIESCTX} CONTEXTS;

// SQL-ANFRAGEN
/* INSERT FILM */
static const char *insertMovieArr[] = {"INSERT INTO Movies(Title, Genre, ReleaseYear, Runtime, Plot, Quality, Rating, CommunityRating, AlreadySeen, IsFavourite, ArchiveStr) " \
                                              "VALUES(:Title, (SELECT ID FROM Genres WHERE Genre = :Genre), :Year, :Runtime, " \
                                              ":Plot, (SELECT ID FROM Qualities WHERE Source = :Src), :Rating, :ComRating, :Seen, :Fav, :Archive);", 
                                       "INSERT INTO Directors(Name) VALUES(:RegName);", 
                                       "INSERT INTO directed_movie(MovieID, DirectorID) VALUES((SELECT ID FROM Movies WHERE Title = :Title AND ReleaseYear = :Year), (SELECT ID FROM Directors WHERE Name = :RegName));", 
                                       "INSERT INTO Actors(Name) VALUES(:ActName);", 
                                       "INSERT INTO acted_in_movie(MovieID, ActorsID) VALUES((SELECT ID FROM Movies WHERE Title = :Title AND ReleaseYear = :Year), (SELECT ID FROM Actors WHERE Name = :ActName));"};
/* INSERT SERIE */
static const char *insertSeriesArr[] = {"INSERT INTO Series(Title, Genre, Season, ReleaseYear, Plot, Quality, Rating, CommunityRating, AlreadySeen, isFavourite, ArchiveStr) " \
                                               "VALUES(:Title, (SELECT ID FROM Genres WHERE Genre = :Genre), :Season, " \
                                               ":Year, :Plot, (SELECT ID FROM Qualities WHERE Source = :Src), :Rating, :ComRating, :Seen, :Fav, :Archive);", 
                                          "INSERT INTO Actors(Name) VALUES(:ActName);", 
                                          "INSERT INTO acted_in_series(SeriesID, ActorsID) VALUES((SELECT ID FROM Series WHERE Title = :Title AND Season = :Season), (SELECT ID FROM Actors WHERE Name = :ActName));"};
// ANDERE DEKLARATIONEN
int rc, stepVal;

// FUNKTIONSPROTOTYPEN
void printError(char *, bool);
static void checkResultCode(int);
/* MOVIEKONTEXT */
ctx_movieInfo* initMovieContext(void);
void freeMovieContext(ctx_movieInfo **);
static bool addBasicInfoToMovieContext(ctx_movieInfo **, char *, char *, char *, char *, char *, char *, 
    unsigned short, unsigned short, unsigned short, bool, bool);
static void addDirectorToMovieContext(ctx_movieInfo **, char *);
static void addActorToMovieContext(ctx_movieInfo **, char *);
static inline unsigned short countDirectorArguments(ctx_movieInfo **);
static inline unsigned short countMovieActorArguments(ctx_movieInfo **);
/* SERIENKONTEXT */
ctx_seriesInfo* initSeriesContext(void);
void freeSeriesContext(ctx_seriesInfo **);
static bool addBasicInfoToSeriesContext(ctx_seriesInfo **, char *, char *, char *, char *, char *, 
    unsigned short, unsigned short, unsigned short, unsigned short, bool, bool);
void addActorToSeriesContext(ctx_seriesInfo **, char *);
static inline unsigned short countSeriesActorArguments(ctx_seriesInfo **);
/* TESTFUNKTIONEN */
bool checkIfQualityExists(char *sourceQuality);
bool checkIfGenreExists(char *genre);

void addMovieUnknownGenre(ctx_movieInfo **);
void addMovieUnknownQuality(ctx_movieInfo **);
void addMovieBothUnknown(ctx_movieInfo **);
void addSeriesUnknownGenre(ctx_seriesInfo **);
void addSeriesUnknownQuality(ctx_seriesInfo **);
void addSeriesBothUnknown(ctx_seriesInfo **);

int main(int argc, char** argv)
{
	// Deklarationen
    ctx_movieInfo *film = initMovieContext();
    ctx_seriesInfo *serie = initSeriesContext();

    /* Add basic info (e.g. title, plot, release year etc.) about the movie to the context */
    if( !(addBasicInfoToMovieContext(&film, "Baskin", "Splatter", "01:37:00", 
            "Baskin is a 2015 surreal horror film by director Can Evrenol",
            "BR-RIP", "Stream", 2015, 7, 6, true, false)) )
        printError("[!] FEHLER beim eintragen der Basisinformationen. Breche ab...", true);
    /* Add director(s) and actor(s) to the movie context */
    addDirectorToMovieContext(&film, "Can Evrenol");
    addActorToMovieContext(&film, "Mehmet Cerrahoğlu");
    addActorToMovieContext(&film, "Ergun Kuyucu");
    addActorToMovieContext(&film, "Görkem Kasal");
    addActorToMovieContext(&film, "Muharrem Bayrak");

    /* Add basic info (e.g. title, plot, release year etc.) about the series to the context */
    if( !(addBasicInfoToSeriesContext(&serie, "The Big Bang Theory", "Sitcom",
            "Die Serie handelt von den hochintelligenten jungen Physikern Leonard Hofstadter und Sheldon Cooper, deren WG gegenüber der Wohnung der hübschen Kellnerin Penny liegt. " \
            "Dabei wird die geekhafte Art der beiden Wissenschaftler durch die Naivität, aber auch die Sozialkompetenz bzw. den gesunden Menschenverstand der Nachbarin, einer klischeehaften Blondine, kontrastiert",
            "DVD-RIP", "TV", 10, 2016, 9, 9, false, true)) )
        printError("[!] FEHLER beim eintragen der Basisinformationen. Breche ab...", true);
    /* Add actor(s) to the series context */
    addActorToSeriesContext(&serie, "Johnny Galecki");
    addActorToSeriesContext(&serie, "Jim Parsons");
    addActorToSeriesContext(&serie, "Kaley Cuoco");
    addActorToSeriesContext(&serie, "Simon Helberg");
    addActorToSeriesContext(&serie, "Kunal Nayyar");
    addActorToSeriesContext(&serie, "Sara Gilbert");
    addActorToSeriesContext(&serie, "Melissa Rauch");
    addActorToSeriesContext(&serie, "Mayim Bialik");
    addActorToSeriesContext(&serie, "Kevin Sussman");
    addActorToSeriesContext(&serie, "Laura Spencer");

    // SQLite Stuff
    rc = sqlite3_open("movies.db", &movieDatabase);

    if (rc != SQLITE_OK) {
        printError("Cannot open database: %s\n", true);
        sqlite3_close(movieDatabase);
        
        return EXIT_FAILURE;
    }

    /*
     -----------------------------------
    |				Auswahl 			|
     -----------------------------------
    |	1 = INSERT MOVIE UNKNOWNGENRE   |
    |	2 = INSERT MOVIE UNKNOWNQUALITY	|
    |	3 = INSERT MOVIE BOTHUNKNOWN 	|
    |	4 = INSERT SERIES UNKNOWNGENRE 	|
    |   5 = INSERT SERIES UNKNOWNQUALITY|
    |   6 = INSERT MOVIE BOTHUNKNOWN    |
    |   q = QUIT                        |
    |                                   |
     -----------------------------------
    */
	printf(" ------------------------------------------- \n|\t\t   Auswahl\t\t    |\n|-------------------------------------------|\n" \
		"|\t1 = INSERT MOVIE UNKNOWNGENRE\t    |\n|\t2 = INSERT MOVIE UNKNOWNQUALITY\t    |\n|\t3 = INSERT MOVIE BOTHUNKNOWN\t    |\n" \
		"|\t4 = INSERT SERIES UNKNOWNGENRE\t    |\n|\t5 = INSERT SERIES UNKNOWNQUALITY    |\n|\t6 = INSERT SERIES BOTHUNKNOWN\t    |\n" \
        "|\tq = QUIT\t\t\t    |\n|\t\t\t\t\t    |\n ------------------------------------------- \n");
    
	int choice = -1;
    do
    {              
    	if(choice != '\n')   // Besonderheit bei dieser Implementierung von "getchar", falls es mich weiterhin stört, zu "scanf" oder anderem wechseln
    	{
    		switch(choice)
    		{
    			case '1' : {addMovieUnknownGenre(&film); break;}
    			case '2' : {addMovieUnknownQuality(&film); break;}
    			case '3' : {addMovieBothUnknown(&film); break;}
    			case '4' : {addSeriesUnknownGenre(&serie); break;}
                case '5' : {addSeriesUnknownQuality(&serie); break;}
                case '6' : {addSeriesBothUnknown(&serie); break;}
                default : break;
    		}	
    	}
    	fflush(stdin);
    }
    while(toupper(choice = getchar()) != 'Q' /*&& choice != EOF && choice != '\n'*/);
    
    sqlite3_finalize(stmtSQL);
    sqlite3_close(movieDatabase);

    // Freigaben
    freeMovieContext(&film);
    freeSeriesContext(&serie);

	return EXIT_SUCCESS;
}

/* 
    Method to indicate an error and an optional program termination (gets used by the other modules too)
    ----------------------------------------------------------------
    @param char *ErrMsg - the error message to be printed at the output stream
    @param bool Quit - optionally quit the program at critical errors
*/
void printError(char *ErrMsg, bool Quit)
{
    fprintf(stderr, ErrMsg, sqlite3_errmsg(movieDatabase));
    if(Quit)
    {
        sqlite3_reset(stmtSQL);
        sqlite3_finalize(stmtSQL);
        sqlite3_close(movieDatabase);
        exit(EXIT_FAILURE);
    }
}

/*
	...
	--------------------------------------
	@param int code - SQLITE3 Error Code
*/
static void checkResultCode(int code)
{
	switch(code){
		case SQLITE_CONSTRAINT : 
			printError("[!] Fehler : %s.\n", false);   
			break;   
		//...
		default : // alles derzeit NICHT EXPLIZIT behandelte
			printError("[!] Fehler : %s.\n", false);
			break;
	}
}

/*
    Initialize our Movies context structure
    by allocating memory etc.
    ----------------------------------------
*/
ctx_movieInfo* initMovieContext(void)
{
    ctx_movieInfo *movieInfo = malloc(sizeof(ctx_movieInfo));
    movieInfo->title = malloc(sizeof(char *) * 101);
    movieInfo->genre = malloc(sizeof(char *) * 51);   // frei Schnauze
    movieInfo->runtime = malloc(sizeof(char *) * 9);   // HH:MM:SS
    movieInfo->plot = malloc(sizeof(char *) * 32768);   // string with sufficient space
    movieInfo->sourceQuality = malloc(sizeof(char *) * 50);
    movieInfo->archived = malloc(sizeof(char *) * 261);   // 260 --> max DOS Pathlength + 1 Nullbyte
    movieInfo->cntDirectors = 0;
    movieInfo->cntActors = 0;
    
    for(int i = 0; i <= 2; i++)
        if( (movieInfo->directors[i] = malloc( 51 * sizeof(char *) )) == NULL)   // max 51 characters per directors name
            printError("[!] FEHLER : Konnte keinen Speicherplatz für Regisseur-String in \"initMovieContext\" reservieren !\n", true);
    for(int i = 0; i <= 9; i++)
        if( (movieInfo->actors[i] = malloc( 51 * sizeof(char *) )) == NULL)   // max 51 characters per actors name
            printError("[!] FEHLER : Konnte keinen Speicherplatz für Actors-String in \"initMovieContext\" reservieren !\n", true);
    movieInfo->directors[2] = NULL;
    movieInfo->actors[10] = NULL;
    
    return movieInfo;
}

/*
    Free the resources used by our Movies context structure
    -------------------------------------------------------
    @param ctx_movieInfo **movieInfo - The structure to be freed
    (call with "&")
*/
void freeMovieContext(ctx_movieInfo **movieInfo)
{
    assert(*movieInfo != NULL);
    free((*movieInfo)->title);
    free((*movieInfo)->genre);
    free((*movieInfo)->runtime);
    free((*movieInfo)->plot);
    free((*movieInfo)->sourceQuality);
    free((*movieInfo)->archived);
    for(int i = 0; i <= 2; i++)
        free((*movieInfo)->directors[i]);
    for(int i = 0; i <= 9; i++)
        free((*movieInfo)->actors[i]);
    free(*movieInfo);
    *movieInfo = NULL;
}

static bool addBasicInfoToMovieContext(ctx_movieInfo **movieInfo, char *title, char *genre, char *runtime, char *plot, 
    char *sourceQuality, char *archived, unsigned short year, unsigned short myRating, unsigned short communityRating, bool seen, bool favourite)
{
    bool ret = true;
    if( *movieInfo != NULL ){
        ( title == NULL ) ? ret = false : (int)strncpy((*movieInfo)->title, title, strlen(title) + 1);
        ( genre == NULL ) ? ret = false : (int)strncpy((*movieInfo)->genre, genre, strlen(genre) + 1);
        ( runtime == NULL ) ? ret = false : (int)strncpy((*movieInfo)->runtime, runtime, strlen(runtime) + 1);
        ( plot == NULL ) ? ret = false : (int)strncpy((*movieInfo)->plot, plot, strlen(plot) + 1);
        ( sourceQuality == NULL ) ? ret = false : (int)strncpy((*movieInfo)->sourceQuality, sourceQuality, strlen(sourceQuality) + 1);
        ( archived == NULL ) ? ret = false : (int)strncpy((*movieInfo)->archived, archived, strlen(archived) + 1);
        (*movieInfo)->year = year;
        (*movieInfo)->myRating = myRating;
        (*movieInfo)->communityRating = communityRating;
        (*movieInfo)->alreadySeen = seen;
        (*movieInfo)->isFavourite = favourite;

        // check if genre or quality exists...
        //... add code later after testing
    }else {ret=false;}

    return ret;
}

static void addDirectorToMovieContext(ctx_movieInfo **movieInfo, char *director)
{
    if( *movieInfo != NULL ){
        if( (*movieInfo)->cntDirectors < 2 ){
            strncpy((*movieInfo)->directors[(*movieInfo)->cntDirectors], director, strlen(director) + 1);
            (*movieInfo)->cntDirectors++;
        };
    }
}

static void addActorToMovieContext(ctx_movieInfo **movieInfo, char *actor)
{
    if( *movieInfo != NULL ){
        if( (*movieInfo)->cntActors < 10 ){
            strncpy((*movieInfo)->actors[(*movieInfo)->cntActors], actor, strlen(actor) + 1);
            (*movieInfo)->cntActors++;
        };
    }
}

static inline unsigned short countDirectorArguments(ctx_movieInfo **movieInfo)
{
    return (*movieInfo)->cntDirectors;
}

static inline unsigned short countMovieActorArguments(ctx_movieInfo **movieInfo)
{
    return (*movieInfo)->cntActors;
}

/*
    Initialize our series context structure
    by allocating memory etc.
    ----------------------------------------
    @param ctx_seriesInfo *seriesInfo - the series context structure to be initialized
*/
ctx_seriesInfo* initSeriesContext(void)
{
    ctx_seriesInfo *seriesInfo = malloc(sizeof(ctx_seriesInfo));
    seriesInfo->title = malloc(sizeof(char *) * 101);
    seriesInfo->genre = malloc(sizeof(char *) * 51);   // frei Schnauze
    seriesInfo->plot = malloc(sizeof(char *) * 32768);   // string with sufficient space
    seriesInfo->sourceQuality = malloc(sizeof(char *) * 50);
    seriesInfo->archived = malloc(sizeof(char *) * 261);   // 260 --> max DOS Pathlength + 1 Nullbyte
    seriesInfo->cntActors = 0;
    
    for(int i = 0; i <= 9; i++)
        if( (seriesInfo->actors[i] = malloc( 51 * sizeof(char *) )) == NULL)   // max 51 characters per actors name
            printError("[!] FEHLER : Konnte keinen Speicherplatz für Actors-String in \"initSeriesContext\" reservieren !\n", true);
    seriesInfo->actors[10] = NULL;

    return seriesInfo;
}

/*
    Free the resources used by our series context structure
    -------------------------------------------------------
    @param ctx_seriesInfo **seriesInfo - The structure to be freed
    (call with "&")
*/
void freeSeriesContext(ctx_seriesInfo **seriesInfo)
{
    assert(*seriesInfo != NULL);
    free((*seriesInfo)->title);
    free((*seriesInfo)->genre);
    free((*seriesInfo)->plot);
    free((*seriesInfo)->sourceQuality);
    free((*seriesInfo)->archived);
    for(int i = 0; i <= 9; i++)
      free((*seriesInfo)->actors[i]);
    free(*seriesInfo);
    *seriesInfo = NULL;
}

static bool addBasicInfoToSeriesContext(ctx_seriesInfo **seriesInfo, char *title, char *genre, char *plot, char *sourceQuality, char *archived, 
    unsigned short season, unsigned short year, unsigned short myRating, unsigned short communityRating, bool seen, bool favourite)
{
    bool ret = true;
    if( *seriesInfo != NULL ){
        ( title == NULL ) ? ret = false : (int)strncpy((*seriesInfo)->title, title, strlen(title) + 1);
        ( genre == NULL ) ? ret = false : (int)strncpy((*seriesInfo)->genre, genre, strlen(genre) + 1);
        ( plot == NULL ) ? ret = false : (int)strncpy((*seriesInfo)->plot, plot, strlen(plot) + 1);
        ( sourceQuality == NULL ) ? ret = false : (int)strncpy((*seriesInfo)->sourceQuality, sourceQuality, strlen(sourceQuality) + 1);
        ( archived == NULL ) ? ret = false : (int)strncpy((*seriesInfo)->archived, archived, strlen(archived) + 1);
        (*seriesInfo)->season = season;
        (*seriesInfo)->year = year;
        (*seriesInfo)->myRating = myRating;
        (*seriesInfo)->communityRating = communityRating;
        (*seriesInfo)->alreadySeen = seen;
        (*seriesInfo)->isFavourite = favourite;
    }else {ret=false;}

    return ret;
}

void addActorToSeriesContext(ctx_seriesInfo **seriesInfo, char *actor)
{
    if( (*seriesInfo)->cntActors < 10 ){
        strncpy((*seriesInfo)->actors[(*seriesInfo)->cntActors], actor, strlen(actor) + 1);
        (*seriesInfo)->cntActors++;
    }
}

/*
  count the arguments of the char ** arrays ("ctx_seriesInfo" & "ctx_seriesInfo")
  within the movie- and series-context structure.
  -----------------------------------------------
  @param char **args - argument vector (filled with directors or actors)
*/
static inline unsigned short countSeriesActorArguments(ctx_seriesInfo **seriesInfo/*char **args*/)
{
    return (*seriesInfo)->cntActors;
}

bool checkIfGenreExists(char *genre)
{
    // Deklarationen
    int idxGenre;

    if( genre != NULL )
    {
        rc = sqlite3_prepare_v2(movieDatabase, "SELECT COUNT(ID) FROM Genres WHERE Genre LIKE :genre", -1, &stmtSQL, 0);
        if (rc == SQLITE_OK) {
            if( (idxGenre = sqlite3_bind_parameter_index(stmtSQL, ":genre")) == 0 )
                printError("[!] FEHLER in \"addMovieUnknownGenre #1\": %s\n", false);
            sqlite3_bind_text(stmtSQL, idxGenre, genre, -1, SQLITE_TRANSIENT);
            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
                if( (int)(sqlite3_column_int(stmtSQL, 0)) == 1 )
                    return true;
                else
                    return false;
            else
                checkResultCode(stepVal);
        }else {
            printError("Failed to execute statement: %s\n", true);
        }
    }
    //else return false;
    return false;
}

bool checkIfQualityExists(char *sourceQuality)
{
    // Deklarationen
    int idxSource;

    if( sourceQuality != NULL )
    {
        rc = sqlite3_prepare_v2(movieDatabase, "SELECT COUNT(ID) FROM Qualities WHERE Source LIKE :src", -1, &stmtSQL, 0);
        if (rc == SQLITE_OK) {
            if( (idxSource = sqlite3_bind_parameter_index(stmtSQL, ":src")) == 0 )
                printError("[!] FEHLER in \"addMovieUnknownQuality #1\": %s\n", false);
            sqlite3_bind_text(stmtSQL, idxSource, sourceQuality, -1, SQLITE_TRANSIENT);
            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
                if( (int)(sqlite3_column_int(stmtSQL, 0)) == 1 )
                    return true;
                else
                    return false;
            else
                checkResultCode(stepVal);
        }else {
            printError("Failed to execute statement: %s\n", true);
        }
    }
    //else return false;
    return false;
}


/* 
    ****************************************************************
    FOLGENDES IST NUR ZUM TESTEN GEDACHT !!! 
    DIE 2 OBIGEN FUNKTIONEN WERDEN WOHL IN DAS HAUPTPROGRAMM ÜBERNOMMEN WERDEN (SEPARAT ODER IN EINER FUNKTION INKL. EINFÜGEN, MAL SEHEN)
    ****************************************************************
*/

void addMovieUnknownGenre(ctx_movieInfo **movieInfo)
{
    // Deklarationen
    int idxGenre;

    if( *movieInfo != NULL )
    {
        rc = sqlite3_prepare_v2(movieDatabase, "SELECT COUNT(ID) FROM Genres WHERE Genre LIKE :genre", -1, &stmtSQL, 0);
        if (rc == SQLITE_OK) {
            if( (idxGenre = sqlite3_bind_parameter_index(stmtSQL, ":genre")) == 0 )
                printError("[!] FEHLER in \"addMovieUnknownGenre #1\": %s\n", false);
            sqlite3_bind_text(stmtSQL, idxGenre, (*movieInfo)->genre, -1, SQLITE_TRANSIENT);
            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
                if( (int)(sqlite3_column_int(stmtSQL, 0)) == 1 )
                    printf("GENRE \"%s\" BEREITS VORHANDEN !\n", (*movieInfo)->genre);
                else
                    printf("GENRE \"%s\" EXISTIERT NICHT !\n", (*movieInfo)->genre);
            else
                checkResultCode(stepVal);
        }else {
            printError("Failed to execute statement: %s\n", true);
        }
    }
    return;
}

void addMovieUnknownQuality(ctx_movieInfo **movieInfo)
{
    // Deklarationen
    int idxSource;

    if( *movieInfo != NULL )
    {
        rc = sqlite3_prepare_v2(movieDatabase, "SELECT COUNT(ID) FROM Qualities WHERE Source LIKE :src", -1, &stmtSQL, 0);
        if (rc == SQLITE_OK) {
            if( (idxSource = sqlite3_bind_parameter_index(stmtSQL, ":src")) == 0 )
                printError("[!] FEHLER in \"addMovieUnknownQuality #1\": %s\n", false);
            sqlite3_bind_text(stmtSQL, idxSource, (*movieInfo)->sourceQuality, -1, SQLITE_TRANSIENT);
            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
                if( (int)(sqlite3_column_int(stmtSQL, 0)) == 1 )
                    printf("QUALITÄT \"%s\" BEREITS VORHANDEN !\n", (*movieInfo)->sourceQuality);
                else
                    printf("QUALITÄT \"%s\" EXISTIERT NICHT !\n", (*movieInfo)->sourceQuality);
            else
                checkResultCode(stepVal);
        }else {
            printError("Failed to execute statement: %s\n", true);
        }
    }
    return;
}

void addMovieBothUnknown(ctx_movieInfo **movieInfo)
{
    ( checkIfGenreExists((*movieInfo)->genre) && checkIfQualityExists((*movieInfo)->sourceQuality) ) ? 
        printf("GENRE UND QUALITÄT BEREITS VORHANDEN !\n") : printf("GENRE ODER QUALITÄT EXISTIERT NICHT !\n");
    return;
}

void addSeriesUnknownGenre(ctx_seriesInfo **seriesInfo)
{
    // Deklarationen
    int idxGenre;

    if( *seriesInfo != NULL )
    {
        rc = sqlite3_prepare_v2(movieDatabase, "SELECT COUNT(ID) FROM Genres WHERE Genre LIKE :genre", -1, &stmtSQL, 0);
        if (rc == SQLITE_OK) {
            if( (idxGenre = sqlite3_bind_parameter_index(stmtSQL, ":genre")) == 0 )
                printError("[!] FEHLER in \"addSeriesUnknownGenre #1\": %s\n", false);
            sqlite3_bind_text(stmtSQL, idxGenre, (*seriesInfo)->genre, -1, SQLITE_TRANSIENT);
            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
                if( (int)(sqlite3_column_int(stmtSQL, 0)) == 1 )
                    printf("GENRE \"%s\" BEREITS VORHANDEN !\n", (*seriesInfo)->genre);
                else
                    printf("GENRE \"%s\" EXISTIERT NICHT !\n", (*seriesInfo)->genre);
            else
                checkResultCode(stepVal);
        }else {
            printError("Failed to execute statement: %s\n", true);
        }
    }
    return;
}

void addSeriesUnknownQuality(ctx_seriesInfo **seriesInfo)
{
    // Deklarationen
    int idxSource;

    if( *seriesInfo != NULL )
    {
        rc = sqlite3_prepare_v2(movieDatabase, "SELECT COUNT(ID) FROM Qualities WHERE Source LIKE :src", -1, &stmtSQL, 0);
        if (rc == SQLITE_OK) {
            if( (idxSource = sqlite3_bind_parameter_index(stmtSQL, ":src")) == 0 )
                printError("[!] FEHLER in \"addSeriesUnknownQuality #1\": %s\n", false);
            sqlite3_bind_text(stmtSQL, idxSource, (*seriesInfo)->sourceQuality, -1, SQLITE_TRANSIENT);
            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
                if( (int)(sqlite3_column_int(stmtSQL, 0)) == 1 )
                    printf("QUALITÄT \"%s\" BEREITS VORHANDEN !\n", (*seriesInfo)->sourceQuality);
                else
                    printf("QUALITÄT \"%s\" EXISTIERT NICHT !\n", (*seriesInfo)->sourceQuality);
            else
                checkResultCode(stepVal);
        }else {
            printError("Failed to execute statement: %s\n", true);
        }
    }
    return;
}

void addSeriesBothUnknown(ctx_seriesInfo **seriesInfo)
{
    ( checkIfGenreExists((*seriesInfo)->genre) && checkIfQualityExists((*seriesInfo)->sourceQuality) ) ? 
        printf("GENRE UND QUALITÄT BEREITS VORHANDEN !\n") : printf("GENRE ODER QUALITÄT EXISTIERT NICHT !\n");
    return;
}
