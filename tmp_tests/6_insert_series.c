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

// FUNKTIONSPROTOTYPEN
void printError(char *, bool);
static void checkResultCode(int);
ctx_seriesInfo* initSeriesContext(void);
void freeSeriesContext(ctx_seriesInfo **);
static bool addBasicInfoToSeriesContext(ctx_seriesInfo **, char *, char *, char *, char *, char *, 
	unsigned short, unsigned short, unsigned short, unsigned short, bool, bool);
void addActorToSeriesContext(ctx_seriesInfo **, char *);
static inline unsigned short countActorArguments(ctx_seriesInfo **);

int main(int argc, char** argv)
{
	// Deklarationen / Initialisierungen
	static const char *insertSeriesArr[] = {"INSERT INTO Series(Title, Genre, Season, ReleaseYear, Plot, Quality, Rating, CommunityRating, AlreadySeen, isFavourite, ArchiveStr) " \
                                               "VALUES(:Title, (SELECT ID FROM Genres WHERE Genre = :Genre), :Season, " \
                                               ":Year, :Plot, (SELECT ID FROM Qualities WHERE Source = :Src), :Rating, :ComRating, :Seen, :Fav, :Archive);", 
                                          	"INSERT INTO Actors(Name) VALUES(:ActName);", 
                                          	"INSERT INTO acted_in_series(SeriesID, ActorsID) VALUES((SELECT ID FROM Series WHERE Title = :Title AND Season = :Season), (SELECT ID FROM Actors WHERE Name = :ActName));"};

  	char *str = malloc(sizeof(char *) * 32768);   // temporary string with sufficient space (e.g. for "plot")
  	// Unser Kontext
	ctx_seriesInfo *test = initSeriesContext();

  	/* Add basic info (e.g. title, plot, release year etc.) about the series to the context */
	if( !(addBasicInfoToSeriesContext(&test, "Malcolm mittendrin", "Sitcom",
			"\"Malcolm mittendrin\" erzählt die Geschichte einer Familie der US-amerikanischen unteren Mittelschicht; Mutter, Vater und vier (später fünf) Söhne aus dem fiktiven Vorort Newcastle. "\
    		"Neben einer Reihe von Parallelhandlungen ist vor allem in den ersten Staffeln das Hauptthema die Auseinandersetzung des drittältesten Sohnes Malcolm mit seiner Hochbegabung und deren Auswirkung auf sein Leben.",
			"DVD-RIP", "DVD0xM0x", 1, 2000, 8, 8, true, true)) )
		printError("[!] FEHLER beim eintragen der Basisinformationen. Breche ab...", true);
	/* Add actor(s) to the series context */
	addActorToSeriesContext(&test, "Frankie Muniz");
  	addActorToSeriesContext(&test, "Erik Per Sullivan");
  	addActorToSeriesContext(&test, "Justin Berfield");
  	addActorToSeriesContext(&test, "Christopher Kennedy Masterson");
	addActorToSeriesContext(&test, "Bryan Cranston");
	addActorToSeriesContext(&test, "Jane Kaczmarek");
	
	#ifdef DEBUGMODE
		printf("DEBUG : \n-----\n");
		printf("title : %s\n", test->title);
		printf("genre : %s\n", test->genre);
		printf("plot : %s\n", test->plot);
		printf("sourceQuality : %s\n", test->sourceQuality);
		printf("archived : %s\n", test->archived);
		printf("season : %d\n", test->season);
		printf("year : %d\n", test->year);
		printf("myRating : %d\n", test->myRating);
		printf("communityRating : %d\n", test->communityRating);
		printf("alreadySeen : %d\n", test->alreadySeen);
		printf("isFavourite : %d\n", test->isFavourite);
		printf("Actors : \n");
		for(int i = 0; i < countActorArguments(&test); i++)
			printf("\t%s\n", test->actors[i]);
	#endif

	// SQLite Stuff
  	int rc = sqlite3_open("movies.db", &movieDatabase);
  	int idxTitle = 0, idxGenre = 0, idxSeason = 0, idxYear = 0, idxPlot = 0, idxSourceQuality = 0, 
    	idxMyRating = 0, idxCommunityRating = 0, idxSeen, idxFavourite, idxArchived = 0, idxActors = 0, stepVal = 0; 
  	unsigned short cnt = 0;

	if (rc != SQLITE_OK) {
		fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(movieDatabase));
		sqlite3_close(movieDatabase);
	        
		return 1;
	}

	// loop through the sql insert statements in "insertSeriesArr"
	for(int i = 0; i <= (sizeof(insertSeriesArr) / sizeof(char *)) - 1; i++)
	{
		switch(i)
		{
			case 0: {
	            /*********************************************************************************************
	               ***************            INSERT SERIE               ***************
	             *********************************************************************************************/
				rc = sqlite3_prepare_v2(movieDatabase, *(insertSeriesArr + i), -1, &stmtSQL, 0);
	    
				if (rc == SQLITE_OK) {
					#ifdef DEBUGMODE
						printf("\t[DEBUG insertSeries #1] Erstelle 1. Eintrag...\n");
					#endif
					// get all indixes
					if( (idxTitle = sqlite3_bind_parameter_index(stmtSQL, ":Title")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxGenre = sqlite3_bind_parameter_index(stmtSQL, ":Genre")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxSeason = sqlite3_bind_parameter_index(stmtSQL, ":Season")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxYear = sqlite3_bind_parameter_index(stmtSQL, ":Year")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxPlot = sqlite3_bind_parameter_index(stmtSQL, ":Plot")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxSourceQuality = sqlite3_bind_parameter_index(stmtSQL, ":Src")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxMyRating = sqlite3_bind_parameter_index(stmtSQL, ":Rating")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxCommunityRating = sqlite3_bind_parameter_index(stmtSQL, ":ComRating")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxSeen = sqlite3_bind_parameter_index(stmtSQL, ":Seen")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxFavourite = sqlite3_bind_parameter_index(stmtSQL, ":Fav")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);
					if( (idxArchived = sqlite3_bind_parameter_index(stmtSQL, ":Archive")) == 0 )
						printError("[!] FEHLER in \"insertSeries #1\": %s\n", false);       

					// bind values to indexes
					if( snprintf(str, strlen(test->title) + 1, "%s", test->title) )
						sqlite3_bind_text(stmtSQL, idxTitle, str, -1, SQLITE_TRANSIENT);
					if( snprintf(str, strlen(test->genre) + 1, "%s", test->genre) )
						sqlite3_bind_text(stmtSQL, idxGenre, str, -1, SQLITE_TRANSIENT);
					if( snprintf(str, strlen(test->plot) + 1, "%s", test->plot) )
						sqlite3_bind_text(stmtSQL, idxPlot, str, -1, SQLITE_TRANSIENT);
					if( snprintf(str, strlen(test->sourceQuality) + 1, "%s", test->sourceQuality) )
						sqlite3_bind_text(stmtSQL, idxSourceQuality, str, -1, SQLITE_TRANSIENT);
					if( snprintf(str, strlen(test->archived) + 1, "%s", test->archived) )
						sqlite3_bind_text(stmtSQL, idxArchived, str, -1, SQLITE_TRANSIENT);
					sqlite3_bind_int(stmtSQL, idxSeason, test->season);
					sqlite3_bind_int(stmtSQL, idxYear, test->year);
					sqlite3_bind_int(stmtSQL, idxMyRating, test->myRating);
					sqlite3_bind_int(stmtSQL, idxCommunityRating, test->communityRating);
					sqlite3_bind_int(stmtSQL, idxSeen, test->alreadySeen);
					sqlite3_bind_int(stmtSQL, idxFavourite, test->isFavourite);
		                                  
					#ifdef DEBUGMODE
						printf("\t[DEBUG insertSeries #1] IDX-Title : %d\n", idxTitle);
						printf("\t[DEBUG insertSeries #1] IDX-Genre : %d\n", idxGenre);
						printf("\t[DEBUG insertSeries #1] IDX-Season : %d\n", idxSeason);
						printf("\t[DEBUG insertSeries #1] IDX-Year : %d\n", idxYear);
						printf("\t[DEBUG insertSeries #1] IDX-Plot : %d\n", idxPlot);
						printf("\t[DEBUG insertSeries #1] IDX-SourceQuality : %d\n", idxSourceQuality);
						printf("\t[DEBUG insertSeries #1] IDX-MyRating : %d\n", idxMyRating);
						printf("\t[DEBUG insertSeries #1] IDX-CommunityRating : %d\n", idxCommunityRating);
						printf("\t[DEBUG insertSeries #1] IDX-Seen : %d\n", idxSeen);
						printf("\t[DEBUG insertSeries #1] IDX-Favourite : %d\n", idxFavourite);
						printf("\t[DEBUG insertSeries #1] IDX-Archived : %d\n", idxArchived);
						printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
					#endif 
				} else {
					fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
				}

				printf("\tWHOOP-WHOOP : %s\n", sqlite3_sql(stmtSQL));
	              
				// begin SQL transaction
				sqlite3_exec(movieDatabase, "BEGIN TRANSACTION;", NULL, NULL, NULL);
				stepVal = sqlite3_step(stmtSQL);
	              
				if (stepVal == SQLITE_ROW) {        
					printf("Eintrag #1 erfolgreich !\n"); 
				}
				// end SQL transaction
				sqlite3_exec(movieDatabase, "END TRANSACTION;", NULL, NULL, NULL);   // "sqlite3_exec(movieDatabase, "COMMIT;", NULL, NULL, NULL);" would be ok, too 
				break;
			}
			case 1: {
	            /*********************************************************************************************
	               ***************          INSERT Actors              ***************
	             *********************************************************************************************/
				cnt = countActorArguments(&test);   // check if there´s more than 1 actor ==> TODO : use the function 
				for(int j = 0; j <= cnt - 1; j++)
				{
					rc = sqlite3_prepare_v2(movieDatabase, *(insertSeriesArr + i), -1, &stmtSQL, 0);

					if (rc == SQLITE_OK) {
						#ifdef DEBUGMODE
							printf("\t[DEBUG insertSeries #2] Erstelle 2. Eintrag...\n");
						#endif
						// get all indexes
						if( (idxActors = sqlite3_bind_parameter_index(stmtSQL, ":ActName")) == 0 )
							printError("[!] FEHLER beim binden von \":ActName\": %s\n", false);

						#ifdef DEBUGMODE
							printf("\t[DEBUG insertSeries #2] IDX-Actors : %d\n", idxActors);
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
							printf("Eintrag #2 erfolgreich !\n");
						}
						else{
							checkResultCode(stepVal);
						} 
					}
				}
				break;
			}
			case 2: {
	            /*********************************************************************************************
	               ***************            SPIELTEN IN Serie              ***************
	             *********************************************************************************************/
				for(int j = 0; j <= cnt - 1; j++)
				{
					rc = sqlite3_prepare_v2(movieDatabase, *(insertSeriesArr + i), -1, &stmtSQL, 0);

					if (rc == SQLITE_OK) {
						#ifdef DEBUGMODE
							printf("\t[DEBUG insertSeries #3] Erstelle 3. Eintrag...\n");
						#endif
						// get all indexes
						if( (idxTitle = sqlite3_bind_parameter_index(stmtSQL, ":Title")) == 0 )
							printError("[!] FEHLER beim binden von \":Title\": %s\n", false);
						if( (idxSeason = sqlite3_bind_parameter_index(stmtSQL, ":Season")) == 0 )
							printError("[!] FEHLER beim binden von \":Season\": %s\n", false);
						if( (idxActors = sqlite3_bind_parameter_index(stmtSQL, ":ActName")) == 0 )
							printError("[!] FEHLER beim binden von \":ActName\": %s\n", false);

						#ifdef DEBUGMODE
							printf("\t[DEBUG insertSeries #3] IDX-Title : %d\n", idxTitle);
							printf("\t[DEBUG insertSeries #3] IDX-Season : %d\n", idxSeason);
							printf("\t[DEBUG insertSeries #3] IDX-Actors : %d\n", idxActors);
							printf("\t[*] %s\n", sqlite3_sql(stmtSQL));
						#endif
					} else {
						fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					}

					if( snprintf(str, strlen(test->title) + 1, "%s", test->title) )
						sqlite3_bind_text(stmtSQL, idxTitle, str, -1, SQLITE_TRANSIENT);
					sqlite3_bind_int(stmtSQL, idxSeason, test->season);

					if( snprintf(str, strlen((*(test->actors + j))) + 1, "%s", (*(test->actors + j))) )
					{
						sqlite3_bind_text(stmtSQL, idxActors, str, -1, SQLITE_TRANSIENT);
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
			default:  printError("[!] FEHLER : Index out of Bounds in \"insertSeries #4\"\n", false); 
		}
	}  

	// Freigaben
	free(str);
	freeSeriesContext(&test);
	sqlite3_finalize(stmtSQL);
	sqlite3_close(movieDatabase);

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
    
    /*
    if( (seriesInfo->actors = malloc( 10 * sizeof(char *) )) == NULL)   // allow up to 10 actors
        printError("[!] FEHLER : Konnte keinen Speicherplatz für Actors in \"initSeriesContext\" reservieren !\n", true);
    */

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
    //free((*seriesInfo)->actors);
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
static inline unsigned short countActorArguments(ctx_seriesInfo **seriesInfo/*char **args*/)
{
	return (*seriesInfo)->cntActors;
}
