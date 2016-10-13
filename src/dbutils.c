/*
File: dbutils.c
Synopsis: implementation of database- and other functions declared in "dbutils.h" (add movie, add series etc. pp.)
		  There´s not much caution taken to "locks" yet, so the database should only be used with one instance of "cmovies"
		  to avoid possible deadlocks.
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 09/27/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies

TODO :	- overthink possible "transactions" and abortion of insert functions 
			(e.g. when is it ok to go on inserting if an error occurs and when is it not !?)
		- given the first TODO, switch to a boolean result value for the insert functions (instead of "void")
		- implement the remaining SQLITE-result codes in function "checkResultCode"
		- add more (silent) logging with "appendToLog" 
		- check "deleteMovie" & "deleteSeries" (don't try to delete the entry if the ID doesn´t even exist)
		- check if it´s better to use the title instead of the id in the deletion functions ("deleteMovie" & "deleteSeries")
			==> probably the question becomes obsolete when we´re using a TUI later ;)
		- PERHAPS instead of using "WHERE NAME = 'THE EXACT NAME'" use regular expressions with "LIKE"
*/ 
// IMPORTS
#include "dbutils.h"
#include "log.h"

// static function prototypes
static int prepareStatementIfNeeded(sqlite3_stmt *, const char *);   // IF the statement is not used yet, prepare it...
static void checkResultCode(int);
static void bind_param_index(sqlite3_stmt *, int *, char *);
//static void updateQuery(char *, char *[]);   // general purpose update function

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
        appendToLog("There was an error and the application gets closed now !");
        closePreparedStatements(stmtSqlInsertMovie);
        closePreparedStatements(stmtSqlInsertSeries);
        closePreparedStatements(stmtSqlUpdate);
        closePreparedStatements(stmtSqlDeletion);
        //sqlite3_close(movieDatabase);   ==> use our own function wrapper
        closeDatabase();
        exit(EXIT_FAILURE);
    }
}

/* 
	function to open the database
	----------------------------------------------------------------
	@return bool - true, if everything went fine, false otherwise
*/
bool openDatabase()
{
	if( sqlite3_open_v2("movies.db", &movieDatabase, SQLITE_OPEN_READWRITE, NULL) ){   // "sqlite3_open" returns "1", if there´s a db error !!!  
		return false;
    }
	else{
        isDBOpened = true;
		return true;
    }
}

/* 
	function to close the database
	----------------------------------------------------------------
	@return bool - true, if everything went fine, false otherwise
*/
bool closeDatabase()
{
	return ((isDBOpened) ? ( (sqlite3_close(movieDatabase) == SQLITE_OK) ? true : false ) : true);
}

/* 
	IF the statement is not used yet, prepare it 
	by compiling the sqlite3 sql statement you 
	want to use into a byte-code representation
	--------------------------------------------
	@param const char *sql - UTF-8 encoded sql query/statement
*/ 
static int prepareStatementIfNeeded(sqlite3_stmt *stmtSQL, const char *sql)
{
	int rc = -1;
	//if(stmtSQL == NULL)
		if( (rc = sqlite3_prepare_v2(movieDatabase, sql, -1, &stmtSQL, 0)) != SQLITE_OK )
			printError("[!] ERROR in \"prepareStatementIfNeeded\"", false);
	return rc;
}

/*
	A function to close all connections (Prepared Statements) before
	closing the database.
	--------------------------------------------
	@param stmtSQL - The statement handle 
					(stmtSqlInsertMovie, stmtSqlInsertSeries & stmtSqlUpdate) with the stmt to finalize
*/
void closePreparedStatements(sqlite3_stmt *stmtSQL)
{
	while( (stmtSQL = (sqlite3_next_stmt(movieDatabase, NULL))) )
		sqlite3_finalize(stmtSQL);
} 

/*
	Check the sqlite result code of the 
	respective sqlite operation and print a readable 
	output describing this result code
	--------------------------------------
	@param int code - SQLITE3 Error Code
*/
static void checkResultCode(int code)
{
	switch(code){ 
		case SQLITE_ABORT : break;
		case SQLITE_ABORT_ROLLBACK : break;
		case SQLITE_AUTH : break;
		case SQLITE_BUSY : break;
		case SQLITE_BUSY_RECOVERY : break;
		case SQLITE_CANTOPEN : break;
		case SQLITE_CANTOPEN_ISDIR : break;
		case SQLITE_CANTOPEN_NOTEMPDIR : break;
		case SQLITE_CONSTRAINT :
			printError("[!] [SQLITE3_CONSTRAINT] An error occured at the INSERT statement (is the entry already in the database ?) : %s.\n", false);
			break;  
		case SQLITE_CORRUPT : break;
		case SQLITE_CORRUPT_VTAB : break;
		case SQLITE_DONE : break;
		case SQLITE_EMPTY : break;
		case SQLITE_ERROR : break;
		case SQLITE_FORMAT : break;
		case SQLITE_FULL : break;
		case SQLITE_INTERNAL : break;
		case SQLITE_INTERRUPT : break;
		case SQLITE_IOERR : break;
		case SQLITE_IOERR_ACCESS : break;
		case SQLITE_IOERR_BLOCKED : break;
		case SQLITE_IOERR_CHECKRESERVEDLOCK : break;
		case SQLITE_IOERR_CLOSE : break;
		case SQLITE_IOERR_DELETE : break;
		case SQLITE_IOERR_DIR_CLOSE : break;
		case SQLITE_IOERR_DIR_FSYNC  : break;
		case SQLITE_IOERR_FSTAT : break;
		case SQLITE_IOERR_FSYNC : break;
		case SQLITE_IOERR_LOCK : break;
		case SQLITE_IOERR_NOMEM : break;
		case SQLITE_IOERR_RDLOCK : break;
		case SQLITE_IOERR_READ : break;
		case SQLITE_IOERR_SEEK : break;
		case SQLITE_IOERR_SHMLOCK : break;
		case SQLITE_IOERR_SHMMAP : break;
		case SQLITE_IOERR_SHMOPEN : break;
		case SQLITE_IOERR_SHMSIZE : break;
		case SQLITE_IOERR_SHORT_READ : break;
		case SQLITE_IOERR_TRUNCATE : break;
		case SQLITE_IOERR_UNLOCK : break;
		case SQLITE_IOERR_WRITE : break;
		case SQLITE_LOCKED : break;
		case SQLITE_LOCKED_SHAREDCACHE : break;
		case SQLITE_MISMATCH : break;
		case SQLITE_MISUSE : break;
		case SQLITE_NOLFS : break;
		case SQLITE_NOMEM : break;
		case SQLITE_NOTADB : break;
		case SQLITE_NOTFOUND : break;
		case SQLITE_OK : break;
		case SQLITE_PERM : break;
		case SQLITE_PROTOCOL : break;
		case SQLITE_RANGE : break;
		case SQLITE_READONLY : break;
		case SQLITE_READONLY_CANTLOCK : break;
		case SQLITE_READONLY_RECOVERY : break;
		case SQLITE_ROW : break;
		case SQLITE_SCHEMA : break;
		case SQLITE_TOOBIG : break;
		default : // alles derzeit NICHT EXPLIZIT behandelte
			printError("[!] An error occured at the INSERT statement : %s.\n", false);
			break;
	}
}

/*
	Bind the given parameter 
    to the given index.
	--------------------------------------
	@param sqlite3_stmt *stmt - pointer to the sqlite3 statement
    @param int *idx - pointer to the index on which to bind
    @param char *param - the prepared stmt placeholder (e.g. ":title")
*/
static void bind_param_index(sqlite3_stmt *stmt, int *idx, char *param)
{
    if( (*idx = sqlite3_bind_parameter_index(stmt, param)) == 0 )
        printError("[!] BINDING-ERROR: %s\n", false);
}

/* 
	Add a new movie to the database
	-------------------------------
	@param ctx_movieInfo *movieInfo - the already filled movie-context structure (title, year etc. set)
*/
void insertMovie(ctx_movieInfo *movieInfo)
{
	// declarations
	int idxTitle = 0, idxGenre = 0, idxYear = 0, idxRuntime = 0, idxPlot = 0, idxSourceQuality = 0, 
		idxMyRating = 0, idxCommunityRating = 0, idxSeen, idxFavourite, idxArchived = 0, idxDirectors = 0, idxActors = 0, stepVal = 0, rc; 
	char *str = malloc(sizeof(char *) * 32768);   // temporary string with sufficient space (e.g. for "plot")

	if(isDBOpened)
	{
		// loop through the sql insert statements in "insertMovieArr"
		for(int i = 0; i <= (sizeof(insertMovieArr) / sizeof(char *)) - 1; i++)
		{
			switch(i)
			{
				case 0:	{
						/*********************************************************************************************
     					 ***************						INSERT MOVIE 						   ***************
	 					 *********************************************************************************************/
     					rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

					    if (rc == SQLITE_OK) {
					        #ifdef DEBUGMODE
								printf("\t[DEBUG insertMovie #1] Creating 1st entry (table \"Movies\")...\n");
							#endif
							// get all indixes
                            bind_param_index(stmtSqlInsertMovie, &idxTitle, ":Title");
                            bind_param_index(stmtSqlInsertMovie, &idxGenre, ":Genre");
                            bind_param_index(stmtSqlInsertMovie, &idxRuntime, ":Runtime");
                            bind_param_index(stmtSqlInsertMovie, &idxPlot, ":Plot");
                            bind_param_index(stmtSqlInsertMovie, &idxSourceQuality, ":Src");
                            bind_param_index(stmtSqlInsertMovie, &idxArchived, ":Archive");
                            bind_param_index(stmtSqlInsertMovie, &idxYear, ":Year");
                            bind_param_index(stmtSqlInsertMovie, &idxMyRating, ":Rating");
                            bind_param_index(stmtSqlInsertMovie, &idxCommunityRating, ":ComRating");
                            bind_param_index(stmtSqlInsertMovie, &idxSeen, ":Seen");
                            bind_param_index(stmtSqlInsertMovie, &idxFavourite, ":Fav");

							// bind values to indexes
							if( snprintf(str, strlen(movieInfo->title) + 1, "%s", movieInfo->title) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxTitle, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(movieInfo->genre) + 1, "%s", movieInfo->genre) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxGenre, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(movieInfo->runtime) + 1, "%s", movieInfo->runtime) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxRuntime, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(movieInfo->plot) + 1, "%s", movieInfo->plot) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxPlot, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(movieInfo->sourceQuality) + 1, "%s", movieInfo->sourceQuality) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxSourceQuality, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(movieInfo->archived) + 1, "%s", movieInfo->archived) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxArchived, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSqlInsertMovie, idxYear, movieInfo->year);
							sqlite3_bind_int(stmtSqlInsertMovie, idxMyRating, movieInfo->myRating);
							sqlite3_bind_int(stmtSqlInsertMovie, idxCommunityRating, movieInfo->communityRating);
							sqlite3_bind_int(stmtSqlInsertMovie, idxSeen, movieInfo->alreadySeen);
							sqlite3_bind_int(stmtSqlInsertMovie, idxFavourite, movieInfo->isFavourite);
					                        
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
								printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertMovie));
							#endif 
					    } else {
					        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    }
					    stepVal = sqlite3_step(stmtSqlInsertMovie);
					    
					    if (stepVal == SQLITE_ROW || stepVal == SQLITE_DONE) {        
					        printf("Entry #1 successful (table \"Movies\") !\n"); 
					    }
						break;
				}
				case 1:	{
						/*********************************************************************************************
     					 ***************					INSERT Directors 						   ***************
	 					 *********************************************************************************************/
	 					// check if theres more than 1 director
					    for(int j = 0; j < COUNTDIRECTORS(&movieInfo); j++)
						{
							//rc = prepareStatementIfNeeded(*(insertMovieArr + i));
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #2] Creating 2nd entry (table \"Directors\")...\n");
								#endif
								// get all indexes
                                bind_param_index(stmtSqlInsertMovie, &idxDirectors, ":RegName");

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #2] IDX-Directors : %d\n", idxDirectors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertMovie));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}
								
							if( snprintf(str, strlen((*(movieInfo->directors + j))) + 1, "%s", (*(movieInfo->directors + j))) )
							{
								sqlite3_bind_text(stmtSqlInsertMovie, idxDirectors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSqlInsertMovie)) == SQLITE_DONE ){
					           		printf("Entry #2 successful (table \"Directors\") !\n");
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
     					 ***************						DIRECTED MOVIE 						   ***************
	 					 *********************************************************************************************/
	 					for(int j = 0; j < COUNTDIRECTORS(&movieInfo); j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #3] Creating 3rd entry (table \"directed_movie\")...\n");
								#endif
								// get all indexes
                                bind_param_index(stmtSqlInsertMovie, &idxTitle, ":Title");
                                bind_param_index(stmtSqlInsertMovie, &idxYear, ":Year");
                                bind_param_index(stmtSqlInsertMovie, &idxDirectors, ":RegName");

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #3] IDX-Title : %d\n", idxTitle);
									printf("\t[DEBUG insertMovie #3] IDX-Year : %d\n", idxYear);
									printf("\t[DEBUG insertMovie #3] IDX-Directors : %d\n", idxDirectors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertMovie));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}

							if( snprintf(str, strlen(movieInfo->title) + 1, "%s", movieInfo->title) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxTitle, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSqlInsertMovie, idxYear, movieInfo->year);

							if( snprintf(str, strlen((*(movieInfo->directors + j))) + 1, "%s", (*(movieInfo->directors + j))) )
							{
								sqlite3_bind_text(stmtSqlInsertMovie, idxDirectors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSqlInsertMovie)) == SQLITE_DONE ){
					           		printf("Entry #3 successful (table \"directed_movie\") !\n");
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
     					 ***************						INSERT Actors						   ***************
	 					 *********************************************************************************************/
						// check if theres more than 1 actor
					    for(int j = 0; j < COUNTACTORS(&movieInfo); j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #4] Creating 4th entry (table \"Actors\")...\n");
								#endif
								// get all indexes
                                bind_param_index(stmtSqlInsertMovie, &idxActors, ":ActName");

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #4] IDX-Actors : %d\n", idxActors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertMovie));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}
								
							if( snprintf(str, strlen((*(movieInfo->actors + j))) + 1, "%s", (*(movieInfo->actors + j))) )
							{
								sqlite3_bind_text(stmtSqlInsertMovie, idxActors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSqlInsertMovie)) == SQLITE_DONE ){
					           		printf("Entry #4 successful (table \"Actors\") !\n");
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
     					 ***************							ACTED IN MOVIE					   ***************
						 *********************************************************************************************/
						for(int j = 0; j < COUNTACTORS(&movieInfo); j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #5] Creating 5th entry (table \"acted_in_movie\")...\n");
								#endif
								// get all indexes
                                bind_param_index(stmtSqlInsertMovie, &idxTitle, ":Title");
                                bind_param_index(stmtSqlInsertMovie, &idxYear, ":Year");
                                bind_param_index(stmtSqlInsertMovie, &idxActors, ":ActName");

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #5] IDX-Title : %d\n", idxTitle);
									printf("\t[DEBUG insertMovie #5] IDX-Year : %d\n", idxYear);
									printf("\t[DEBUG insertMovie #5] IDX-Actors : %d\n", idxActors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertMovie));
								#endif
							} else {
					        	fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
					    	}

							if( snprintf(str, strlen(movieInfo->title) + 1, "%s", movieInfo->title) )
								sqlite3_bind_text(stmtSqlInsertMovie, idxTitle, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSqlInsertMovie, idxYear, movieInfo->year);

							if( snprintf(str, strlen((*(movieInfo->actors + j))) + 1, "%s", (*(movieInfo->actors + j))) )
							{
								sqlite3_bind_text(stmtSqlInsertMovie, idxActors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSqlInsertMovie)) == SQLITE_DONE ){
					           		printf("Entry #5 successful (table \"acted_in_movie\") !\n");
					       		}
					       		else{
					           		checkResultCode(stepVal);
					       		}	
							}
						}
						break;
				}
				default:	printError("[!] ERROR : Index out of Bounds in function \"insertMovie\"\n", false);	
			}

			sqlite3_reset(stmtSqlInsertMovie);   // reset the statement
		}

		// Free resources
		free(str);	
	}
}

/* 
	Add a new series to the database
	--------------------------------
	@param ctx_seriesInfo *seriesInfo - the already filled series-context structure (title, year etc. set) 
*/
void insertSeries(ctx_seriesInfo *seriesInfo)
{
	int idxTitle = 0, idxGenre = 0, idxSeason = 0, idxYear = 0, idxPlot = 0, idxSourceQuality = 0, 
    	idxMyRating = 0, idxCommunityRating = 0, idxSeen, idxFavourite, idxArchived = 0, idxActors = 0, stepVal = 0, rc; 
	char *str = malloc(sizeof(char *) * 32768);   // temporary string with sufficient space (e.g. for "plot")

	if(isDBOpened)
	{
		// loop through the sql insert statements in "insertSeriesArr"
		for(int i = 0; i <= (sizeof(insertSeriesArr) / sizeof(char *)) - 1; i++)
		{
			switch(i)
			{
				case 0:	{
						/*********************************************************************************************
     					 ***************						INSERT SERIES 						   ***************
	 					 *********************************************************************************************/
	 					rc = sqlite3_prepare_v2(movieDatabase, *(insertSeriesArr + i), -1, &stmtSqlInsertSeries, 0);
	    
						if (rc == SQLITE_OK) {
							#ifdef DEBUGMODE
								printf("\t[DEBUG insertSeries #1] Creating 1st entry (table \"Series\")...\n");
							#endif
							// get all indixes
                            bind_param_index(stmtSqlInsertSeries, &idxTitle, ":Title");
                            bind_param_index(stmtSqlInsertSeries, &idxGenre, ":Genre");
                            bind_param_index(stmtSqlInsertSeries, &idxSeason, ":Season");
                            bind_param_index(stmtSqlInsertSeries, &idxYear, ":Year");
                            bind_param_index(stmtSqlInsertSeries, &idxPlot, ":Plot");
                            bind_param_index(stmtSqlInsertSeries, &idxSourceQuality, ":Src");
                            bind_param_index(stmtSqlInsertSeries, &idxMyRating, ":Rating");
                            bind_param_index(stmtSqlInsertSeries, &idxCommunityRating, ":ComRating");
                            bind_param_index(stmtSqlInsertSeries, &idxSeen, ":Seen");
                            bind_param_index(stmtSqlInsertSeries, &idxFavourite, ":Fav");
                            bind_param_index(stmtSqlInsertSeries, &idxArchived, ":Archive");

							// bind values to indexes
							if( snprintf(str, strlen(seriesInfo->title) + 1, "%s", seriesInfo->title) )
								sqlite3_bind_text(stmtSqlInsertSeries, idxTitle, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(seriesInfo->genre) + 1, "%s", seriesInfo->genre) )
								sqlite3_bind_text(stmtSqlInsertSeries, idxGenre, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(seriesInfo->plot) + 1, "%s", seriesInfo->plot) )
								sqlite3_bind_text(stmtSqlInsertSeries, idxPlot, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(seriesInfo->sourceQuality) + 1, "%s", seriesInfo->sourceQuality) )
								sqlite3_bind_text(stmtSqlInsertSeries, idxSourceQuality, str, -1, SQLITE_TRANSIENT);
							if( snprintf(str, strlen(seriesInfo->archived) + 1, "%s", seriesInfo->archived) )
								sqlite3_bind_text(stmtSqlInsertSeries, idxArchived, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSqlInsertSeries, idxSeason, seriesInfo->season);
							sqlite3_bind_int(stmtSqlInsertSeries, idxYear, seriesInfo->year);
							sqlite3_bind_int(stmtSqlInsertSeries, idxMyRating, seriesInfo->myRating);
							sqlite3_bind_int(stmtSqlInsertSeries, idxCommunityRating, seriesInfo->communityRating);
							sqlite3_bind_int(stmtSqlInsertSeries, idxSeen, seriesInfo->alreadySeen);
							sqlite3_bind_int(stmtSqlInsertSeries, idxFavourite, seriesInfo->isFavourite);
		                	                  
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
								printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertSeries));
							#endif 
						} else {
							fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
						}

						stepVal = sqlite3_step(stmtSqlInsertSeries);
	            
						if (stepVal == SQLITE_ROW || stepVal == SQLITE_DONE) {        
							printf("Entry #1 successful (table \"Series\") !\n"); 
						}
						break;
				}
				case 1:	{
						/*********************************************************************************************
     					 ***************						INSERT ACTORS						   ***************
	 					 *********************************************************************************************/
						// check if theres more than 1 actor
						for(int j = 0; j < COUNTACTORS(&seriesInfo); j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertSeriesArr + i), -1, &stmtSqlInsertSeries, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertSeries #2] Creating 2nd entry (table \"Actors\")...\n");
								#endif
								// get all indexes
                                bind_param_index(stmtSqlInsertSeries, &idxActors, ":ActName");

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertSeries #2] IDX-Actors : %d\n", idxActors);
	 								printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertSeries));
								#endif
							} else {
								fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
							}

							if( snprintf(str, strlen((*(seriesInfo->actors + j))) + 1, "%s", (*(seriesInfo->actors + j))) )
							{
								sqlite3_bind_text(stmtSqlInsertSeries, idxActors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSqlInsertSeries)) == SQLITE_DONE ){
									printf("Entry #2 successful (table \"Actors\") !\n");
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
     					 ***************						ACTED IN SERIES 					   ***************
						 *********************************************************************************************/
						for(int j = 0; j < COUNTACTORS(&seriesInfo); j++)
						{
							rc = sqlite3_prepare_v2(movieDatabase, *(insertSeriesArr + i), -1, &stmtSqlInsertSeries, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertSeries #3] Creating 3rd entry (table \"acted_in_series\")...\n");
								#endif
								// get all indexes
                                bind_param_index(stmtSqlInsertSeries, &idxTitle, ":Title");
                                bind_param_index(stmtSqlInsertSeries, &idxSeason, ":Season");
                                bind_param_index(stmtSqlInsertSeries, &idxActors, ":ActName");

								#ifdef DEBUGMODE
									printf("\t[DEBUG insertSeries #3] IDX-Title : %d\n", idxTitle);
									printf("\t[DEBUG insertSeries #3] IDX-Season : %d\n", idxSeason);
									printf("\t[DEBUG insertSeries #3] IDX-Actors : %d\n", idxActors);
									printf("\t[*] %s\n", sqlite3_sql(stmtSqlInsertSeries));
								#endif
							} else {
								fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(movieDatabase));
							}

							if( snprintf(str, strlen(seriesInfo->title) + 1, "%s", seriesInfo->title) )
								sqlite3_bind_text(stmtSqlInsertSeries, idxTitle, str, -1, SQLITE_TRANSIENT);
							sqlite3_bind_int(stmtSqlInsertSeries, idxSeason, seriesInfo->season);

							if( snprintf(str, strlen((*(seriesInfo->actors + j))) + 1, "%s", (*(seriesInfo->actors + j))) )
							{
								sqlite3_bind_text(stmtSqlInsertSeries, idxActors, str, -1, SQLITE_TRANSIENT);
								// do sql insertion
								if( (stepVal = sqlite3_step(stmtSqlInsertSeries)) == SQLITE_DONE ){
									printf("Entry #3 successful (table \"acted_in_series\") !\n");
								}
								else{
									checkResultCode(stepVal);
								} 
							}
						}
						break;
				}
				default:	printError("[!] ERROR : Index out of Bounds in function \"insertSeries\"\n", false);	
			}

			sqlite3_reset(stmtSqlInsertSeries);   // reset the statement
		}

		// Free resources
		free(str);
	}
}

/*
	Remove a movie from the database
	(CAUTION: 
		THE ACTORS AND DIRECTORS TABLES REMAIN UNTOUCHED
		FOR FURTHER INSERTION. ONLY THE ENTRIES INSIDE
		THE "MOVIES"-, "DIRECTED_MOVIE"-, and "ACTED_IN_MOVIE"-TABLES ARE REMOVED)
	--------------------------------
	@param char *title - the title to remove (at the moment, i am using the "id" instead of the title !!!)
	@param unsigned int id - the id of the corresponding movie
*/
void deleteMovie(/*char *title*/ unsigned int movieID)
{
	// declarations / initializations
    int idxMovie = -1, stepVal, rc;

	if( movieID >= 1 )
	{
		printf("[+] Deleting movie with ID \"%d\".\n", movieID);
		sqlite3_exec(movieDatabase, "BEGIN", 0, 0, 0);   // begin transaction

		rc = sqlite3_prepare_v2(movieDatabase, strDeleteMovie, -1, &stmtSqlDeletion, 0);

		if (rc == SQLITE_OK) {
			printf("\tGetting index to delete...\n");
            bind_param_index(stmtSqlDeletion, &idxMovie, ":movieID");
			#ifdef DEBUGMODE
				printf("\t[DEBUG] IDX-Movie : %d\n", idxMovie);
			#endif
			sqlite3_bind_int(stmtSqlDeletion, idxMovie, movieID);
		} else {
			fprintf(stderr, "[!] ERROR : Unable to execute the deletion: %s\n", sqlite3_errmsg(movieDatabase));
		}

		if( (stepVal = sqlite3_step(stmtSqlDeletion)) == SQLITE_DONE )
			printf("Deletion successful.\n");
		else
			checkResultCode(stepVal);

		sqlite3_exec(movieDatabase, "COMMIT", 0, 0, 0);   // Transaktion beenden
	}

	sqlite3_reset(stmtSqlDeletion);   // reset the statement
	return;
}

/*
	Remove a series from the database
	(CAUTION: 
		THE ACTORS TABLES REMAINS UNTOUCHED
		FOR FURTHER INSERTION. ONLY THE ENTRIES INSIDE
		THE "SERIES"-, and "ACTED_IN_SERIES"-TABLES ARE REMOVED)
	--------------------------------
	@param char *title - the title to remove (at the moment, i am using the "id" instead of the title !!!)
	@param unsigned int id - the id of the corresponding series
*/
void deleteSeries(/*char *title*/ unsigned int seriesID)
{
	// declarations / initializations
    int idxSeries = -1, stepVal, rc;

	if( seriesID >= 1 )
	{
		printf("[+] Deleting series with ID \"%d\".\n", seriesID);
		sqlite3_exec(movieDatabase, "BEGIN", 0, 0, 0);   // begin transaction 

		rc = sqlite3_prepare_v2(movieDatabase, strDeleteSeries, -1, &stmtSqlDeletion, 0);

		if (rc == SQLITE_OK) {
			printf("\tGetting index to delete...\n");
            bind_param_index(stmtSqlDeletion, &idxSeries, ":seriesID");
			#ifdef DEBUGMODE
				printf("\t[DEBUG] IDX-Series : %d\n", idxSeries);
			#endif
			sqlite3_bind_int(stmtSqlDeletion, idxSeries, seriesID);
		} else {
			fprintf(stderr, "[!] ERROR : Unable to execute the deletion: %s\n", sqlite3_errmsg(movieDatabase));
		}

		if( (stepVal = sqlite3_step(stmtSqlDeletion)) == SQLITE_DONE )
			printf("Deletion successful.\n");
		else
			checkResultCode(stepVal);

		sqlite3_exec(movieDatabase, "COMMIT", 0, 0, 0);   // end transaction
	}

	sqlite3_reset(stmtSqlDeletion);
	return;
}

/* 
	A general purpose update functions which handles all possibilities 
	is possible, but not that user friendly.

	Update entries in the database
	------------------------------
	@param char *queryString - the desired update query within the "updateQueriesArr"-array
	@param *bindings[] - array with the correct parameters to bind to the particular query

static void updateQuery(char *queryString, char *bindings[])
{
	//;
	sqlite3_reset(stmtSqlUpdate);   // reset the statement
}
*/

/**
	Update the title of a given movie
	---------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param char *title - the new title to set
*/
void updateMovieTitle(unsigned int movieID, char *title)
{
	//...

	return;
}

/**
	Update the title of a given series
	----------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param char *title - the new title to set
*/
void updateSeriesTitle(unsigned int seriesID, char *title)
{
	//...

	return;
}

/**
	Update the genre of a given movie
	---------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param TGenres genre - the new genre to set
*/
void updateMovieGenre(unsigned int movieID, TGenres genre)
{
	//...

	return;
}

/**
	Update the genre of a given series
	----------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param TGenres genre - the new genre to set
*/
void updateSeriesGenre(unsigned int seriesID, TGenres genre)
{
	//...

	return;
}

/**
	Update release year of a given movie
	------------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param unsigned short year - the new release year to set
*/
void updateMovieReleaseYear(unsigned int movieID, unsigned short year)
{
	//...

	return;
}

/**
	Update the season of a given series
	-----------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param unsigned short season - the new season to set
*/
void updateSeriesSeason(unsigned int seriesID, unsigned short season)
{
	//...

	return;
}

/**
	Update the runtime of a given movie
	-----------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param const char *runtime - the new runtime to set
*/
void updateMovieRuntime(unsigned int movieID, const char *runtime)
{
	//...

	return;
}

/**
	Update the release year of a given series
	-----------------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param unsigned short year - the new release year to set
*/
void updateSeriesReleaseYear(unsigned int seriesID, unsigned short year)
{
	//...

	return;
}

/**
	Update the quality of a given movie
	-----------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param TQualities quality - the new quality to set
*/
void updateMovieQuality(unsigned int movieID, TQualities quality)
{
	//...

	return;
}

/**
	Update the quality of a given series
	------------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param TQualities quality - the new quality to set
*/
void updateSeriesQuality(unsigned int seriesID, TQualities quality)
{
	//...

	return;
}

/**
	Update the rating of a given movie
	----------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param unsigned short rating - the new rating to set
*/
void updateMovieRating(unsigned int movieID, unsigned short rating)
{
	//...

	return;
}

/**
	Update the rating of a given series
	-----------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param unsigned short rating - the new rating to set
*/
void updateSeriesRating(unsigned int seriesID, unsigned short rating)
{
	//...

	return;
}

/**
	Update the community rating of a given movie
	--------------------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param unsigned short communityRating - the new community rating to set
*/
void updateMovieCommunityRating(unsigned int movieID, unsigned short communityRating)
{
	//...

	return;
}

/**
	Update the community rating of a given series
	---------------------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param unsigned short communityRating - the new community rating to set
*/
void updateSeriesCommunityRating(unsigned int seriesID, unsigned short communityRating)
{
	//...

	return;
}

/**
	Update the "seen"-flag of a given movie
	---------------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param bool alreadySeen - the new flag (true/false) to set
*/
void updateMovieAlreadySeen(unsigned int movieID, bool alreadySeen)
{
	//...

	return;
}

/**
	Update the "seen"-flag of a given series
	----------------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param bool alreadySeen - the new flag (true/false) to set
*/
void updateSeriesAlreadySeen(unsigned int seriesID, bool alreadySeen)
{
	//...

	return;
}

/**
	Update the "isFavourite"-flag of a given movie
	----------------------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param bool isFavourite - the new flag (true/false) to set
*/
void updateMovieIsFavourite(unsigned int movieID, bool isFavourite)
{
	//...

	return;
}

/**
	Update the "isFavourite"-flag of a given series
	-----------------------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param bool isFavourite - the new flag (true/false) to set
*/
void updateSeriesIsFavourite(unsigned int seriesID, bool isFavourite)
{
	//...

	return;
}

/**
	Update the "archive" string of a given movie
	--------------------------------------------
	@param unsigned int movieID - the movies´ ID in table "Movies"
	@param const char *archived - the new archive-path/string to set
*/
void updateMovieArchive(unsigned int movieID, const char *archived)
{
	//...

	return;
}

/**
	Update the "archive" string of a given series
	---------------------------------------------
	@param unsigned int seriesID - the series´ ID in table "Series"
	@param const char *archived - the new archive-path/string to set
*/
void updateSeriesArchive(unsigned int seriesID, const char *archived)
{
	//...

	return;
}

//...
