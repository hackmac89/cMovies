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

// SQL-ANFRAGEN
/* SELECT ALL ALREADY SEEN MOVIES */
const static char *strSelectSeenMovies = "SELECT Movies.Title FROM Movies WHERE Movies.AlreadySeen = 1;";
/* SELECT ALL FAVOURITE MOVIES */
const static char *strSelectFavouriteMovies = "SELECT Movies.Title FROM Movies WHERE Movies.IsFavourite = 1;";  
/* SELECT ALL ALREADY SEEN SERIES */
const static char *strSelectSeenSeries = "SELECT Series.Title, Series.Season FROM Series WHERE Series.AlreadySeen = 1;";
/* SELECT ALL FAVOURITE SERIES */
const static char *strSelectFavouriteSeries = "SELECT Series.Title, Series.Season FROM Series WHERE Series.IsFavourite = 1;";
/* SELECT ALL CURRENTLY UNSEEN MOVIES */
const static char *strSelectUnseenMovies = "SELECT Movies.Title FROM Movies WHERE Movies.AlreadySeen = 0;"; 
/* SELECT ALL CURRENTLY UNSEEN SERIES */
const static char *strSelectUnseenSeries = "SELECT Series.Title, Series.Season FROM Series WHERE Series.AlreadySeen = 0;";   
/* DELETE MOVIE (directors and actors entries remain untouched) */
const static char *strDeleteMovie[] = {"DELETE FROM acted_in_movie WHERE MovieID = :movieID;", 
                                       "DELETE FROM directed_movie WHERE MovieID = :movieID;",
                                       "DELETE FROM Movies WHERE ID = :movieID;"};   
/* DELETE SERIES (actors entries remain untouched) */
const static char *strDeleteSeries[] = {"DELETE FROM acted_in_series WHERE SeriesID = :seriesID;", 
                                        "DELETE FROM Series WHERE ID = :seriesID;"};
/* DELETE ALL (RESET TO RELEASE-DATABASE) */  
const static char *strResetDatabase[] = {"DELETE FROM acted_in_movie;", 
                                         "DELETE FROM acted_in_series;",
                                         "DELETE FROM Actors;", 
                                         "DELETE FROM directed_movie;", 
                                         "DELETE FROM Directors;", 
                                         "DELETE FROM Genres WHERE ID > 26;", 
                                         "DELETE FROM Movies;", 
                                         "DELETE FROM Qualities WHERE ID > 11;", 
                                         "DELETE FROM Series;"};
// ANDERE DEKLARATIONEN
int rc, stepVal;

// FUNKTIONSPROTOTYPEN
void printError(char *, bool);
static void checkResultCode(int);
void selectSeenMovies(void);
void selectFavouriteMovies(void);
// select both ?
//...	
void selectSeenSeries(void);
void selectFavouriteSeries(void);
void selectUnseenMovies(void);
void selectUnseenSeries(void);
void deleteMovie(unsigned int);   // evtl. etwas weniger allgemein gestalten und "Filmtitel" und "Erscheinungsjahr" als Parameter verwenden und anhand derer die ID ermitteln ?!?!?!
void deleteSeries(unsigned int);   // dito.
void resetDatabase(void);

int main(int argc, char** argv)
{
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
    |	1 = SELECT SEEN MOVIES 			|
    |	2 = SELECT FAVOURITE MOVIES 	|
    |	3 = SELECT SEEN SERIES 			|
    |	4 = SELECT FAVOURITE SERIES 	|
    |   5 = SELECT UNSEEN MOVIES        |
    |   6 = SELECT UNSEEN SERIES        |
    |   7 = DELETE MOVIE                |
    |   8 = DELETE SERIES               |
    |   9 = RESET DATABASE              |
    |   q = QUIT                        |
    |                                   |
     -----------------------------------
    */
	printf(" ----------------------------------- \n|\t\tAuswahl\t\t    |\n|-----------------------------------|\n" \
		"|\t1 = SELECT SEEN MOVIES\t    |\n|\t2 = SELECT FAVOURITE MOVIES |\n|\t3 = SELECT SEEN SERIES\t    |\n" \
		"|\t4 = SELECT FAVOURITE SERIES |\n|\t5 = SELECT UNSEEN MOVIES    |\n|\t6 = SELECT UNSEEN SERIES    |\n" \
        "|\t7 = DELETE MOVIE\t    |\n|\t8 = DELETE SERIES\t    |\n|\t9 = RESET DATABASE\t    |\n|\tq = QUIT\t\t    |\n|\t\t\t\t    |\n ----------------------------------- \n");
    
	int choice = -1;
    do
    {              
    	if(choice != '\n')   // Besonderheit bei dieser Implementierung von "getchar", falls es mich weiterhin stört, zu "scanf" oder anderem wechseln
    	{
    		//printf("\nIhre Auswahl (1-..., q für Abbruch) : ");
    		switch(choice)
    		{
    			case '1' : {selectSeenMovies(); break;}
    			case '2' : {selectFavouriteMovies(); break;}
    			case '3' : {selectSeenSeries(); break;}
    			case '4' : {selectFavouriteSeries(); break;}
                case '5' : {selectUnseenMovies(); break;}
                case '6' : {selectUnseenSeries(); break;}
                case '7' : {deleteMovie(3); break;}
                case '8' : {deleteSeries(4); break;}
                case '9' : {resetDatabase(); break;}
                default : break;
    		}	
    	}
    	fflush(stdin);
    }
    while(toupper(choice = getchar()) != 'Q' /*&& choice != EOF && choice != '\n'*/);
    
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

void selectSeenMovies(void)
{
	printf("\n[+] Beginne mit der Auswahlabfrage (SELECT-SEEN-MOVIES).\n");

    rc = sqlite3_prepare_v2(movieDatabase, strSelectSeenMovies, -1, &stmtSQL, 0);
    if (rc != SQLITE_OK) {
        printError("Failed to execute statement: %s\n", true);
    }

    while( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
        printf("%s\n", sqlite3_column_text(stmtSQL, 0));

	return;
}

void selectFavouriteMovies(void)
{
	printf("\n[+] Beginne mit der Auswahlabfrage (SELECT-FAVOURITE-MOVIES).\n");

    rc = sqlite3_prepare_v2(movieDatabase, strSelectFavouriteMovies, -1, &stmtSQL, 0);
    if (rc != SQLITE_OK) {
        printError("Failed to execute statement: %s\n", true);
    }

    while( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW ) 
        printf("%s\n", sqlite3_column_text(stmtSQL, 0));

	return;
}

void selectSeenSeries(void)
{
	printf("\n[+] Beginne mit der Auswahlabfrage (SELECT-SEEN-SERIES).\n");

    rc = sqlite3_prepare_v2(movieDatabase, strSelectSeenSeries, -1, &stmtSQL, 0);
    if (rc != SQLITE_OK) {
        printError("Failed to execute statement: %s\n", true);
    }

    while( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
        printf("%s - Staffel %d\n", sqlite3_column_text(stmtSQL, 0), sqlite3_column_int(stmtSQL, 1));

	return;
}

void selectFavouriteSeries(void)
{
	printf("\n[+] Beginne mit der Auswahlabfrage (SELECT-FAVOURITE-SERIES).\n");

    rc = sqlite3_prepare_v2(movieDatabase, strSelectFavouriteSeries, -1, &stmtSQL, 0);
    if (rc != SQLITE_OK) {
        printError("Failed to execute statement: %s\n", true);
    }

    while( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
        printf("%s - Staffel %d\n", sqlite3_column_text(stmtSQL, 0), sqlite3_column_int(stmtSQL, 1));

	return;
}

void selectUnseenMovies(void)
{  
    printf("\n[+] Beginne mit der Auswahlabfrage (SELECT-UNSEEN-MOVIES).\n");

    rc = sqlite3_prepare_v2(movieDatabase, strSelectUnseenMovies, -1, &stmtSQL, 0);
    if (rc != SQLITE_OK) {
        printError("Failed to execute statement: %s\n", true);
    }

    while( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW )
        printf("%s\n", sqlite3_column_text(stmtSQL, 0));

    return;
}

void selectUnseenSeries(void)
{
    printf("\n[+] Beginne mit der Auswahlabfrage (SELECT-UNSEEN-SERIES).\n");

    rc = sqlite3_prepare_v2(movieDatabase, strSelectUnseenSeries, -1, &stmtSQL, 0);
    if (rc != SQLITE_OK) {
        printError("Failed to execute statement: %s\n", true);
    }

    while( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_ROW ) 
        printf("%s - Staffel %d\n", sqlite3_column_text(stmtSQL, 0), sqlite3_column_int(stmtSQL, 1));

    return;
}

void deleteMovie(unsigned int movieID)
{
    // Deklaratinen
    int idxMovie = -1;

    if( movieID >= 1 )
    {
        printf("[+] Entferne den gewählten Film aus der Datenbank.\n");
        sqlite3_exec(movieDatabase, "BEGIN", 0, 0, 0);   // Transaktion starten 

        for(int i = 0; i <= (sizeof(strDeleteMovie) / sizeof(char *)) - 1; i++)
        {
            rc = sqlite3_prepare_v2(movieDatabase, *(strDeleteMovie + i), -1, &stmtSQL, 0);
        
            if (rc == SQLITE_OK) {
                switch(i)
                {
                    // Der Index bleinbt gleich, da es nur ":movieID" in den Abfragen gibt
                    case 0 :
                    case 1 :
                    case 2 : {  // (Tabelle : acted_in_movie, directed_movie und Movies)
                                printf("\t[DEBUG] Hole Index zum löschen...\n");
                                if( (idxMovie = sqlite3_bind_parameter_index(stmtSQL, ":movieID")) == 0 )
                                    printError("[!] FEHLER in \"deleteMovie #1\": %s\n", false);
                                #ifdef DEBUGMODE
                                    printf("\t[DEBUG] IDX-Movie : %d\n", idxMovie);
                                #endif
                                sqlite3_bind_int(stmtSQL, idxMovie, movieID);
                                break;
                            }
                    default : printError("[!] FEHLER : Index out of Bounds in \"deleteMovie #2\"\n", true);
                }
            } else {
                fprintf(stderr, "[!] FEHLER : Konnte den Löschbefehl NICHT ausführen: %s\n", sqlite3_errmsg(movieDatabase));
            }

            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
                printf("[DEBUG] Löschen erfolgreich.\n");
            }
            else{
                checkResultCode(stepVal);
            }
            
            // Statement zurücksetzen
            sqlite3_reset(stmtSQL);
        }

        sqlite3_exec(movieDatabase, "COMMIT", 0, 0, 0);   // Transaktion beenden
    }

    return;
}

void deleteSeries(unsigned int seriesID)
{
    // Deklaratinen
    int idxSeries = -1;

    if( seriesID >= 1 )
    {
        printf("[+] Entferne die gewählte Serie aus der Datenbank.\n");
        sqlite3_exec(movieDatabase, "BEGIN", 0, 0, 0);   // Transaktion starten 

        for(int i = 0; i <= (sizeof(strDeleteSeries) / sizeof(char *)) - 1; i++)
        {
            rc = sqlite3_prepare_v2(movieDatabase, *(strDeleteSeries + i), -1, &stmtSQL, 0);
        
            if (rc == SQLITE_OK) {
                switch(i)
                {
                    // Der Index bleinbt gleich, da es nur ":seriesID" in den Abfragen gibt
                    case 0 :
                    case 1 : {  // (Tabelle : acted_in_series und Series)
                                printf("\t[DEBUG] Hole Index zum löschen...\n");
                                if( (idxSeries = sqlite3_bind_parameter_index(stmtSQL, ":seriesID")) == 0 )
                                    printError("[!] FEHLER in \"deleteSeries #1\": %s\n", false);
                                #ifdef DEBUGMODE
                                    printf("\t[DEBUG] IDX-Series : %d\n", idxSeries);
                                #endif
                                sqlite3_bind_int(stmtSQL, idxSeries, seriesID);
                                break;
                            }
                    default : printError("[!] FEHLER : Index out of Bounds in \"deleteSeries #2\"\n", true);
                }
            } else {
                fprintf(stderr, "[!] FEHLER : Konnte den Löschbefehl NICHT ausführen: %s\n", sqlite3_errmsg(movieDatabase));
            }

            if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
                printf("[DEBUG] Löschen erfolgreich.\n");
            }
            else{
                checkResultCode(stepVal);
            }
            
            // Statement zurücksetzen
            sqlite3_reset(stmtSQL);
        }

        sqlite3_exec(movieDatabase, "COMMIT", 0, 0, 0);   // Transaktion beenden
    }

    return;
}

void resetDatabase(void)
{
    printf("[+] Setze die Datenbank auf den Releasezustand zurück.\n");
    sqlite3_exec(movieDatabase, "BEGIN", 0, 0, 0);   // Transaktion starten 

    for(int i = 0; i <= (sizeof(strResetDatabase) / sizeof(char *)) - 1; i++)
    {
        rc = sqlite3_prepare_v2(movieDatabase, *(strResetDatabase + i), -1, &stmtSQL, 0);
        
        if (rc != SQLITE_OK) {
            fprintf(stderr, "[!] FEHLER : Konnte den Löschbefehl NICHT ausführen: %s\n", sqlite3_errmsg(movieDatabase));  
        } 

        if( (stepVal = sqlite3_step(stmtSQL)) == SQLITE_DONE ){
            printf("[DEBUG] Löschen erfolgreich.\n");
        }
        else{
            checkResultCode(stepVal);
        }
            
        // Statement zurücksetzen
        sqlite3_reset(stmtSQL);
    }

    sqlite3_exec(movieDatabase, "COMMIT", 0, 0, 0);   // Transaktion beenden

    return;
}
