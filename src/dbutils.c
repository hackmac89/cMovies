/*
File: dbutils.c
Synopsis: implementation of database- and other functions declared in "dbutils.h" (add movie, add series etc. pp.)
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 08/23/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies

TODO : - Die Abfragen statt mit "WHERE NAME = 'DER GENAUE NAME'" lieber mit regul. Ausdrücken und "LIKE" machen.
*/ 
// IMPORTS
#include "dbutils.h"
#include "log.h"

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
        closePreparedStatements(stmtSQLUpdate);
        sqlite3_close(movieDatabase);
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
		case SQLITE_CONSTRAINT : 
			printError("[!] An error occured at the INSERT statement (is the entry already in the database ?) : %s.\n", false);   
			break;   
		//...
		default : // alles derzeit NICHT EXPLIZIT behandelte
			printError("[!] An error occured at the INSERT statement : %s.\n", false);
			break;
	}
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
						//rc = prepareStatementIfNeeded(*(insertMovieArr + i));
     					rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

					    if (rc == SQLITE_OK) {
					        #ifdef DEBUGMODE
								printf("\t[DEBUG insertMovie #1] Erstelle 1. Eintrag...\n");
							#endif
							// get all indixes
							if( (idxTitle = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Title")) == 0 )
								printError("[!] BINDING-ERROR (Title) in \"insertMovie #1\": %s\n", false);
							if( (idxGenre = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Genre")) == 0 )
								printError("[!] BINDING-ERROR (Genre) in \"insertMovie #1\": %s\n", false);
							if( (idxRuntime = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Runtime")) == 0 )
								printError("[!] BINDING-ERROR (Runtime) in \"insertMovie #1\": %s\n", false);
							if( (idxPlot = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Plot")) == 0 )
								printError("[!] BINDING-ERROR (Plot) in \"insertMovie #1\": %s\n", false);
							if( (idxSourceQuality = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Src")) == 0 )
								printError("[!] BINDING-ERROR (Src) in \"insertMovie #1\": %s\n", false);
							if( (idxArchived = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Archive")) == 0 )
								printError("[!] BINDING-ERROR (Archive) in \"insertMovie #1\": %s\n", false);
							if( (idxYear = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Year")) == 0 )
								printError("[!] BINDING-ERROR (Year) in \"insertMovie #1\": %s\n", false);
							if( (idxMyRating = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Rating")) == 0 )
								printError("[!] BINDING-ERROR (Rating) in \"insertMovie #1\": %s\n", false);
							if( (idxCommunityRating = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":ComRating")) == 0 )
								printError("[!] BINDING-ERROR (ComRating) in \"insertMovie #1\": %s\n", false);
							if( (idxSeen = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Seen")) == 0 )
								printError("[!] BINDING-ERROR (AlreadySeen) in \"insertMovie #1\": %s\n", false);
							if( (idxFavourite = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Fav")) == 0 )
								printError("[!] BINDING-ERROR (IsFavourite) in \"insertMovie #1\": %s\n", false);

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
					    
					    if (stepVal == SQLITE_ROW) {        
					        printf("Eintrag #1 erfolgreich !\n"); 
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
									printf("\t[DEBUG insertMovie #2] Erstelle 2. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxDirectors = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":RegName")) == 0 )
									printError("[!] FEHLER beim binden von \":RegName\": %s\n", false);

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
     					 ***************						DIRECTED MOVIE 						   ***************
	 					 *********************************************************************************************/
	 					for(int j = 0; j < COUNTDIRECTORS(&movieInfo); j++)
						{
							//rc = prepareStatementIfNeeded(*(insertMovieArr + i));
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #3] Erstelle 3. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxTitle = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Title")) == 0 )
									printError("[!] FEHLER beim binden von \":Title\": %s\n", false);
								if( (idxYear = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Year")) == 0 )
									printError("[!] FEHLER beim binden von \":Year\": %s\n", false);
								if( (idxDirectors = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":RegName")) == 0 )
									printError("[!] FEHLER beim binden von \":RegName\": %s\n", false);

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
						// check if theres more than 1 actor
					    for(int j = 0; j < COUNTACTORS(&movieInfo); j++)
						{
							//rc = prepareStatementIfNeeded(*(insertMovieArr + i));
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #4] Erstelle 4. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxActors = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":ActName")) == 0 )
									printError("[!] FEHLER beim binden von \":ActName\": %s\n", false);

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
     					 ***************							ACTED IN 						   ***************
						 *********************************************************************************************/
						for(int j = 0; j < COUNTACTORS(&movieInfo); j++)
						{
							//rc = prepareStatementIfNeeded(*(insertMovieArr + i));
							rc = sqlite3_prepare_v2(movieDatabase, *(insertMovieArr + i), -1, &stmtSqlInsertMovie, 0);

							if (rc == SQLITE_OK) {
								#ifdef DEBUGMODE
									printf("\t[DEBUG insertMovie #5] Erstelle 5. Eintrag...\n");
								#endif
								// get all indexes
								if( (idxTitle = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Title")) == 0 )
									printError("[!] FEHLER beim binden von \":Title\": %s\n", false);
								if( (idxYear = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":Year")) == 0 )
									printError("[!] FEHLER beim binden von \":Year\": %s\n", false);
								if( (idxActors = sqlite3_bind_parameter_index(stmtSqlInsertMovie, ":ActName")) == 0 )
									printError("[!] FEHLER beim binden von \":ActName\": %s\n", false);

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

			sqlite3_reset(stmtSqlInsertMovie);
		}

		// Free ressources
		free(str);	
		//closePreparedStatements();
	}
}

/* 
	Add a new series to the database
	--------------------------------
	@param ctx_seriesInfo *seriesInfo - the already filled series-context structure (title, year etc. set) 
*/
void insertSeries(ctx_seriesInfo *seriesInfo)
{
	//;
	// uninitialize sql statement (call "prepareStatementIfNeeded" before you use it again)
	stmtSqlInsertSeries = NULL;
}

/* 
	Update entries in the database
	------------------------------
	@param char *queryString - the desired update query within the "updateQueriesArr"-array
	@param *bindings[] - array with the correct parameters to bind to the particular query
*/
void updateQuery(char *queryString, char *bindings[])
{
	//;
	// uninitialize sql statement (call "prepareStatementIfNeeded" before you use it again)
	stmtSQLUpdate = NULL;
}

//...
