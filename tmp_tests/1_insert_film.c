/*
	- http://stackoverflow.com/questions/14766683/pointing-dereference-inside-a-struct-error
	- http://stackoverflow.com/questions/26953163/c-pointer-being-freed-was-not-allocated-even-though-i-mallocd-it-and-thats-a
	- http://stackoverflow.com/questions/3840582/still-reachable-leak-detected-by-valgrind
*/
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include <assert.h>

// SONSTIGES
//#define DEBUGMODE
typedef int bool;
#define true 1
#define false 0

// GLOBALE VARIABLEN
sqlite3 *movieDatabase;   // Unsere Datenbank
sqlite3_stmt *stmtSQL = NULL;   // Statement, welches anhand einer SQL-Query vorbereitet wird ("prepare" und "bind" irgendwie) 

// UNSER KONTEXT
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

// FUNKTIONSPROTOTYPEN
void printError(char *, bool);
static void checkResultCode(int);
ctx_movieInfo* initMovieContext(void);
void freeMovieContext(ctx_movieInfo **);
static bool addBasicInfoToMovieContext(ctx_movieInfo **, char *, char *, char *, char *, char *, char *, 
	unsigned short, unsigned short, unsigned short, bool, bool);
static void addDirectorToMovieContext(ctx_movieInfo **, char *);
static void addActorToMovieContext(ctx_movieInfo **, char *);
static inline unsigned short countDirectorArguments(ctx_movieInfo **);
static inline unsigned short countActorArguments(ctx_movieInfo **);

int main(int argc, char** argv)
{
	// Deklarationen / Initialisierungen
	static const char *insertMovieArr[] = {"INSERT INTO Movies(Title, Genre, ReleaseYear, Runtime, Plot, Quality, Rating, CommunityRating, AlreadySeen, IsFavourite, ArchiveStr) " \
                                          		"VALUES(:Title, (SELECT ID FROM Genres WHERE Genre = :Genre), :Year, :Runtime, " \
                                          		":Plot, (SELECT ID FROM Qualities WHERE Source = :Src), :Rating, :ComRating, :AlreadySeen, :IsFavourite, :Archive);", 
                                       	   "INSERT INTO Directors(Name) VALUES(:RegName);", 
                                       	   "INSERT INTO directed_movie(MovieID, DirectorID) VALUES((SELECT ID FROM Movies WHERE Title = :Title AND ReleaseYear = :Year), (SELECT ID FROM Directors WHERE Name = :RegName));", 
                                       	   "INSERT INTO Actors(Name) VALUES(:ActName);", 
                                       	   "INSERT INTO acted_in_movie(MovieID, ActorsID) VALUES((SELECT ID FROM Movies WHERE Title = :Title AND ReleaseYear = :Year), (SELECT ID FROM Actors WHERE Name = :ActName));"};
	char *str = malloc(sizeof(char *) * 32768);   // temporary string with sufficient space (e.g. for "plot")
	// Unser Kontext
	ctx_movieInfo *test = initMovieContext();

	/* Add basic info (e.g. title, plot, release year etc.) about the movie to the context */
	if( !(addBasicInfoToMovieContext(&test, "Zoomania", "Animation", "01:49:00", 
			"In einer von anthropomorphen Säugetieren bewohnten Welt erfüllt Judy Hopps aus dem ländlichen Dorf Bunnyborrow in Nageria ihren Traum, als erster Hase Polizist zu werden...",
			"BR-RIP", "HDD", 2015, 8, 8, true, true)) )
		printError("[!] FEHLER beim eintragen der Basisinformationen. Breche ab...", true);
	/* Add director(s) and actor(s) to the movie context */
	addDirectorToMovieContext(&test, "Byron Howard");
	addDirectorToMovieContext(&test, "Rich Moore");
	addActorToMovieContext(&test, "Ginnifer Goodwin");
	addActorToMovieContext(&test, "Jason Bateman");
	addActorToMovieContext(&test, "Idris Elba");
	addActorToMovieContext(&test, "Jenny Slate");
	addActorToMovieContext(&test, "Nate Torrence");
	addActorToMovieContext(&test, "Bonnie Hunt");
	addActorToMovieContext(&test, "Don Lake");
	addActorToMovieContext(&test, "Tommy Chong");
	addActorToMovieContext(&test, "J. K. Simmons");
	addActorToMovieContext(&test, "Alan Tudyk");
	
	#ifdef DEBUGMODE
		printf("DEBUG : \n-----\n");
		printf("title : %s\n", test->title);
		printf("genre : %s\n", test->genre);
		printf("runtime : %s\n", test->runtime);
		printf("plot : %s\n", test->plot);
		printf("sourceQuality : %s\n", test->sourceQuality);
		printf("archived : %s\n", test->archived);
		printf("year : %d\n", test->year);
		printf("myRating : %d\n", test->myRating);
		printf("communityRating : %d\n", test->communityRating);
		printf("alreadySeen : %d\n", test->alreadySeen);
		printf("isFavourite : %d\n", test->isFavourite);
		printf("Directors : \n");
		for(int i = 0; i < countDirectorArguments(&test); i++)
			printf("\t%s\n", test->directors[i]);
		printf("Actors : \n");
		for(int i = 0; i < countActorArguments(&test); i++)
			printf("\t%s\n", test->actors[i]);
	#endif

	// SQLite Stuff
	int rc = sqlite3_open("Movies.db", &movieDatabase);
	int idxTitle = 0, idxGenre = 0, idxYear = 0, idxRuntime = 0, idxPlot = 0, idxSourceQuality = 0, 
		idxMyRating = 0, idxCommunityRating = 0, idxSeen, idxFavourite, idxArchived = 0, idxDirectors = 0, idxActors = 0, stepVal = 0; 
    unsigned short cnt = 0;

    if (rc != SQLITE_OK) {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(movieDatabase));
        sqlite3_close(movieDatabase);
        
        return 1;
    }

    // hier erstmal von Hand (ohne Schleifen) die anderen Abfragen (als SEPARATE STRINGS und nicht in einem Array) testen.
    // WENN DANN ALLES ALS EINZELNES KLAPPT, DANN DIE VERSCHIEDENEN ETAPPEN (FILM, REGISSEUR, Actors ETC.) IN 
    // SEPARATE TRANSAKTIONEN PACKEN (VORHER NOCHMAL MIT EINER GESAMTTRANSAKTION VERSUCHEN)

    // loop through the sql insert statements in "insertMovieArr"
	for(int i = 0; i <= (sizeof(insertMovieArr) / sizeof(char *)) - 1; i++)
	{
		switch(i)
		{
			case 0:	{
						/*********************************************************************************************
     					 ***************						INSERT FILM 						   ***************
	 					 *********************************************************************************************/
						rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSQL, 0);
    
					    if (rc == SQLITE_OK) {
					        #ifdef DEBUGMODE
								printf("\t[DEBUG insertMovie #1] Erstelle 1. Eintrag...\n");
							#endif
							// get all indixes
							if( (idxTitle = sqlite3_bind_parameter_index(stmtSQL, ":Title")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxGenre = sqlite3_bind_parameter_index(stmtSQL, ":Genre")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxRuntime = sqlite3_bind_parameter_index(stmtSQL, ":Runtime")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxPlot = sqlite3_bind_parameter_index(stmtSQL, ":Plot")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxSourceQuality = sqlite3_bind_parameter_index(stmtSQL, ":Src")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxArchived = sqlite3_bind_parameter_index(stmtSQL, ":Archive")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxYear = sqlite3_bind_parameter_index(stmtSQL, ":Year")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxMyRating = sqlite3_bind_parameter_index(stmtSQL, ":Rating")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxCommunityRating = sqlite3_bind_parameter_index(stmtSQL, ":ComRating")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxSeen = sqlite3_bind_parameter_index(stmtSQL, ":AlreadySeen")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);
							if( (idxFavourite = sqlite3_bind_parameter_index(stmtSQL, ":IsFavourite")) == 0 )
								printError("[!] FEHLER in \"insertMovie #1\": %s\n", false);

							// bind values to indexes
							if( snprintf(str, strlen(test->title) + 1, "%s", test->title) )
								sqlite3_bind_text(stmtSQL, idxTitle, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(test->genre) + 1, "%s", test->genre) )
								sqlite3_bind_text(stmtSQL, idxGenre, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(test->runtime) + 1, "%s", test->runtime) )
								sqlite3_bind_text(stmtSQL, idxRuntime, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(test->plot) + 1, "%s", test->plot) )
								sqlite3_bind_text(stmtSQL, idxPlot, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(test->sourceQuality) + 1, "%s", test->sourceQuality) )
								sqlite3_bind_text(stmtSQL, idxSourceQuality, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(test->archived) + 1, "%s", test->archived) )
								sqlite3_bind_text(stmtSQL, idxArchived, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSQL, idxYear, test->year);
							sqlite3_bind_int(stmtSQL, idxMyRating, test->myRating);
							sqlite3_bind_int(stmtSQL, idxCommunityRating, test->communityRating);
							sqlite3_bind_int(stmtSQL, idxSeen, test->alreadySeen);
							sqlite3_bind_int(stmtSQL, idxFavourite, test->isFavourite);
					                        
							#ifdef DEBUGMODE
								printf("\t[DEBUG insertMovie #1] IDX-Title : %d\n", idxTitle);
								printf("\t[DEBUG insertMovie #1] IDX-Genre : %d\n", idxGenre);
								printf("\t[DEBUG insertMovie #1] IDX-Year : %d\n", idxYear);
								printf("\t[DEBUG insertMovie #1] IDX-Runtime : %d\n", idxRuntime);
								printf("\t[DEBUG insertMovie #1] IDX-Plot : %d\n", idxPlot);
								printf("\t[DEBUG insertMovie #1] IDX-SourceQuality : %d\n", idxSourceQuality);
								printf("\t[DEBUG insertMovie #1] IDX-MyRating : %d\n", idxMyRating);
								printf("\t[DEBUG insertMovie #1] IDX-CommunityRating : %d\n", idxCommunityRating);
								printf("\t[DEBUG insertMovie #1] IDX-AlreadySeen : %d\n", idxSeen);
								printf("\t[DEBUG insertMovie #1] IDX-IsFavourite : %d\n", idxFavourite);
								printf("\t[DEBUG insertMovie #1] IDX-Archived : %d\n", idxArchived);
								printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
							#endif 
					    } else {
					        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    }

					    printf("\tWHOOP-WHOOP : %s\n", sqlite3_sql(stmtSQL));
					    stepVal = sqlite3_step(stmtSQL);
					    
					    if (stepVal == SQLITE_ROW) {        
					        printf("Eintrag #1 erfolgreich !\n"); 
					    }
						break;
			}
			case 1:	{
						/*********************************************************************************************
     					 ***************					INSERT Directors 						   ***************
	 					 *********************************************************************************************/
	 					cnt = countDirectorArguments(&test);   // check if theres more than 1 director
					    for(int j = 0; j <= cnt - 1; j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSQL, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #2] Erstelle 2. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxDirectors = sqlite3_bind_parameter_index(stmtSQL, ":RegName")) == 0 )
									printError("[!] FEHLER beim binden von \":RegName\": %s\n", false);

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #2] IDX-Directors : %d\n", idxDirectors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}
								
							if( snprintf(str, strlen((*(test->directors + j))) + 1, "%s", (*(test->directors + j))) )
							{
								sqlite3_bind_text(stmtSQL, idxDirectors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
					           		printf("Eintrag #2 erfolgreich !\n");
					       		}
					       		else{
					           		checkResultCode(stepVal);
					       		}	
							}
						}
						break;
			}
			case 2:	{
						/*********************************************************************************************
     					 ***************						HABEN GEDREHT 						   ***************
	 					 *********************************************************************************************/
	 					for(int j = 0; j <= cnt - 1; j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSQL, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #3] Erstelle 3. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxTitle = sqlite3_bind_parameter_index(stmtSQL, ":Title")) == 0 )
									printError("[!] FEHLER beim binden von \":Title\": %s\n", false);
								if( (idxYear = sqlite3_bind_parameter_index(stmtSQL, ":Year")) == 0 )
									printError("[!] FEHLER beim binden von \":Year\": %s\n", false);
								if( (idxDirectors = sqlite3_bind_parameter_index(stmtSQL, ":RegName")) == 0 )
									printError("[!] FEHLER beim binden von \":RegName\": %s\n", false);

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #3] IDX-Title : %d\n", idxTitle);
									printf("\t[DEBUG insertMovie #3] IDX-Year : %d\n", idxYear);
									printf("\t[DEBUG insertMovie #3] IDX-Directors : %d\n", idxDirectors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}

							if( snprintf(str, strlen(test->title) + 1, "%s", test->title) )
								sqlite3_bind_text(stmtSQL, idxTitle, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSQL, idxYear, test->year);

							if( snprintf(str, strlen((*(test->directors + j))) + 1, "%s", (*(test->directors + j))) )
							{
								sqlite3_bind_text(stmtSQL, idxDirectors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
					           		printf("Eintrag #3 erfolgreich !\n");
					       		}
					       		else{
					           		checkResultCode(stepVal);
					       		}	
							}
						}
						break;
			}
			case 3:	{
						/*********************************************************************************************
     					 ***************					INSERT Actors						   ***************
	 					 *********************************************************************************************/
						cnt = countActorArguments(&test);   // check if theres more than 1 director
					    for(int j = 0; j <= cnt - 1; j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSQL, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #4] Erstelle 4. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxActors = sqlite3_bind_parameter_index(stmtSQL, ":ActName")) == 0 )
									printError("[!] FEHLER beim binden von \":ActName\": %s\n", false);

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #4] IDX-Actors : %d\n", idxActors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}
								
							if( snprintf(str, strlen((*(test->actors + j))) + 1, "%s", (*(test->actors + j))) )
							{
								sqlite3_bind_text(stmtSQL, idxActors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
					           		printf("Eintrag #4 erfolgreich !\n");
					       		}
					       		else{
					           		checkResultCode(stepVal);
					       		}	
							}
						}
						break;
			}
			case 4:	{
						/*********************************************************************************************
     					 ***************						SPIELTEN IN 						   ***************
						 *********************************************************************************************/
						for(int j = 0; j <= cnt - 1; j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSQL, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #5] Erstelle 5. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxTitle = sqlite3_bind_parameter_index(stmtSQL, ":Title")) == 0 )
									printError("[!] FEHLER beim binden von \":Title\": %s\n", false);
								if( (idxYear = sqlite3_bind_parameter_index(stmtSQL, ":Year")) == 0 )
									printError("[!] FEHLER beim binden von \":Year\": %s\n", false);
								if( (idxActors = sqlite3_bind_parameter_index(stmtSQL, ":ActName")) == 0 )
									printError("[!] FEHLER beim binden von \":ActName\": %s\n", false);

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #5] IDX-Title : %d\n", idxTitle);
									printf("\t[DEBUG insertMovie #5] IDX-Year : %d\n", idxYear);
									printf("\t[DEBUG insertMovie #5] IDX-Actors : %d\n", idxActors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}

							if( snprintf(str, strlen(test->title) + 1, "%s", test->title) )
								sqlite3_bind_text(stmtSQL, idxTitle, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSQL, idxYear, test->year);

							if( snprintf(str, strlen((*(test->actors + j))) + 1, "%s", (*(test->actors + j))) )
							{
								sqlite3_bind_text(stmtSQL, idxActors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
					           		printf("Eintrag #5 erfolgreich !\n");
					       		}
					       		else{
					           		checkResultCode(stepVal);
					       		}	
							}
						}
						break;
			}
			default:	printError("[!] FEHLER : Index out of Bounds in \"insertMovie #5\"\n", false);	
		}
	}  

	// Freigaben
	free(str);
	freeMovieContext(&test);
    sqlite3_finalize(stmtSQL);
    sqlite3_close(movieDatabase);

	return 0;
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
			printError("[!] Fehler bei der INSERT-Anweisung (Ist der Eintrag bereits vorhanden ?) : %s.\n", false);   
			break;   
		//...
		default : // alles derzeit NICHT EXPLIZIT behandelte
			printError("[!] Fehler bei der INSERT-Anweisung : %s.\n", false);
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
    
    /*
    if( (movieInfo->directors = malloc( 3 * sizeof(char *) )) == NULL)   // allow up to 3 directors ("char" statt "char *" nehmen ?)
        printError("[!] FEHLER : Konnte keinen Speicherplatz für Directors in \"initMovieContext\" reservieren !\n", true);
    if( (movieInfo->actors = malloc( 10 * sizeof(char *) )) == NULL)   // allow up to 10 actors
        printError("[!] FEHLER : Konnte keinen Speicherplatz für Actors in \"initMovieContext\" reservieren !\n", true);
    */

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
	//free((*movieInfo)->directors);
	//free((*movieInfo)->actors);
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

static inline unsigned short countActorArguments(ctx_movieInfo **movieInfo)
{
	return (*movieInfo)->cntActors;
}
