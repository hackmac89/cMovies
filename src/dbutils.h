/*
File: dbutils.h
Synopsis: header with sql queries & function prototypes
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 09/27/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies
*/
#ifndef DBUTILS_H
#define DBUTILS_H

typedef int bool;
#define true 1
#define false 0

// IMPORTS
#include "log.h"
// SQLITE3 specific import
#include <sqlite3.h>

/*********************************
 ***    global declarations    ***
 *********************************/
static sqlite3 *movieDatabase;   // our database
/*  Instead of having one global sqlite3 statement
    i am using separate statements, each for one specific task.
    This is of advantage so i can just "refresh" the prepared stmts after
    using them instead of finalize it after every usage.
    Given that, i reduce compiler and vdbe overhead. */
static sqlite3_stmt *stmtSqlInsertMovie = NULL;   // sqlite3 statement for inserting movies
static sqlite3_stmt *stmtSqlInsertSeries = NULL;   // sqlite3 statement for inserting series
static sqlite3_stmt *stmtSqlUpdate = NULL;   // sqlite3 statement for update stuff
static sqlite3_stmt *stmtSqlDeletion = NULL;   // sqlite3 statement for deletion stuff
/* our predefined genre types (the corresponding string to insert into the db is done in "cmovies.c --> setGenre") */
typedef enum {
    TACTION = 1,
    TADVENTURE,
    TANIMATION,
    TANIME, 
    TCOMEDY,
    TCRIME,
    TDRAMA,
    TDOCUMENTARY,
    TEASTERN,
    TEPIC,
    TFANTASY,
    TFILMNOIR,
    THISTORY,
    THORROR,
    TMARTIALARTS,
    TMELODRAMA,
    TMOCKUMENTARY,
    TMYSTERY,
    TPORN, 
    TROMANCE,
    TSCIENCEFICTION,
    TSITCOM,
    TTHRILLER,
    TTRAGICOMEDY,
    TWESTERN
} TGenres;
/* our predefined quality types (the corresponding string to insert into the db is done in "cmovies.c --> setQuality") */
typedef enum {
    T2K = 1,
    T4K,
    TBR,
    TBRRIP,
    TCAM,
    TDVD,
    TDVDRIP,
    TDVDSCREENER,
    TTELESYNC,
    TTSLD,
    TTSMD,
    TTVRIP,
    TVHS,
    TWEBRIP
} TQualities;

// all other statements yet to come...
static bool isDBOpened = false;   // db flag
typedef struct {
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

/* count the arguments of the char ** arrays ("ctx_movieInfo" & "ctx_seriesInfo")
   within the movie- and series-context structure. */
#define COUNTDIRECTORS(ctx) ( (*ctx)->cntDirectors )
#define COUNTACTORS(ctx) ( (*ctx)->cntActors )

/*********************************
 ***   "Prepared Statements"   ***
 *********************************/
/* INSERT MOVIE */
static const char *insertMovieArr[] = {"INSERT INTO Movies(Title, Genre, ReleaseYear, Runtime, Plot, Quality, Rating, CommunityRating, AlreadySeen, IsFavourite, ArchiveStr) " \
                                              "VALUES(:Title, (SELECT MIN(ID) FROM Genres WHERE Genre = :Genre OR Genre = 'Other'), :Year, :Runtime, " \
                                              ":Plot, (SELECT MIN(ID) FROM Qualities WHERE Source = :Src OR Source = 'Other'), :Rating, :ComRating, :Seen, :Fav, :Archive);",
                                       "INSERT INTO Directors(Name) VALUES(:RegName);", 
                                       "INSERT INTO directed_movie(MovieID, DirectorID) VALUES((SELECT ID FROM Movies WHERE Title = :Title AND ReleaseYear = :Year), (SELECT ID FROM Directors WHERE Name = :RegName));", 
                                       "INSERT INTO Actors(Name) VALUES(:ActName);", 
                                       "INSERT INTO acted_in_movie(MovieID, ActorsID) VALUES((SELECT ID FROM Movies WHERE Title = :Title AND ReleaseYear = :Year), (SELECT ID FROM Actors WHERE Name = :ActName));"};
/* INSERT SERIES */
static const char *insertSeriesArr[] = {"INSERT INTO Series(Title, Genre, Season, ReleaseYear, Plot, Quality, Rating, CommunityRating, AlreadySeen, isFavourite, ArchiveStr) " \
                                               "VALUES(:Title, (SELECT MIN(ID) FROM Genres WHERE Genre = :Genre OR Genre = 'Other'), :Season, " \
                                               ":Year, :Plot, (SELECT MIN(ID) FROM Qualities WHERE Source = :Src OR Source = 'Other'), :Rating, :ComRating, :Seen, :Fav, :Archive);", 
                                        "INSERT INTO Actors(Name) VALUES(:ActName);", 
                                        "INSERT INTO acted_in_series(SeriesID, ActorsID) VALUES((SELECT ID FROM Series WHERE Title = :Title AND Season = :Season), (SELECT ID FROM Actors WHERE Name = :ActName));"};
/* SELECT BASIC MOVIE RESULT APPEARANCE (FOR ALL MOVIES IN THE DB) */  
static const char *strSelectBasicMovies = "SELECT Movies.Title, Genres.Genre, Movies.releaseYear, Movies.Runtime, Qualities.Source, " \
                                                "Directors.Name, Movies.Rating, Movies.CommunityRating, Movies.Added, Movies.ArchiveStr FROM Movies " \
                                              "INNER JOIN Genres, Qualities, directed_movie, Directors ON (Movies.Genre = Genres.ID) AND " \
                                              "(Movies.Quality = Qualities.ID) AND (Movies.ID = directed_movie.MovieID) AND (directed_movie.DirectorID = Directors.ID);";
/* SELECT BASIC RESULT APPEARANCE OF MOVIE X */
static const char *strSelectBasicMovieInfo = "SELECT Movies.Title, Genres.Genre, Movies.ReleaseYear, Movies.Runtime, Qualities.Source, " \
                                             "Directors.Name, Movies.Rating, Movies.CommunityRating, Movies.Added, Movies.ArchiveStr FROM Movies " \
                                             "INNER JOIN Genres, Qualities, directed_movie, Directors " \
                                             "ON (Movies.Genre = Genres.ID) AND (Movies.Quality = Qualities.ID) AND (Movies.ID = directed_movie.MovieID) AND " \
                                             "(directed_movie.DirectorID = Directors.ID) WHERE Movies.Title LIKE :Title;";
/* SELECT BASIC SERIES RESULT APPEARANCE (FOR ALL SERIES IN THE DB) */                                             
static const char *strSelectBasicSeries = "SELECT Series.Title, Genres.Genre, Series.Season, Series.ReleaseYear, Qualities.Source, Series.Rating, " \
                                                "Series.CommunityRating, Series.Added, Series.ArchiveStr FROM Series " \
                                              "INNER JOIN Genres, Qualities ON (Series.Genre = Genres.ID) AND (Series.Quality = Qualities.ID);";                                              
/* SELECT BASIC RESULT APPEARANCE OF SERIES X */
static const char *strSelectBasicSeriesInfo = "SELECT Series.Title, Genres.Genre, Series.Season, Series.ReleaseYear, Qualities.Source, " \
                                              "Series.Rating, Series.CommunityRating, Series.Added, Series.ArchiveStr FROM Series " \
                                              "INNER JOIN Genres, Qualities ON (Series.Genre = Genres.ID) AND (Series.Quality = Qualities.ID) WHERE Series.Title LIKE :Title;";
/* SELECT Projects (SERIES AND Movies) OF ACTOR */
static const char *strSelectProjectsFromActor = "SELECT Movies.Title FROM acted_in_movie INNER JOIN Movies ON acted_in_movie.MovieID = Movies.ID " \
                                                "WHERE acted_in_movie.ActorsID = (SELECT ID FROM Actors WHERE Name LIKE :Name) UNION ALL " \
                                                "SELECT Series.Title FROM acted_in_series INNER JOIN Series ON acted_in_series.SeriesID = Series.ID " \
                                                "WHERE acted_in_series.ActorsID = (SELECT ID FROM Actors WHERE Name LIKE :Name);";
/* SELECT Movies FROM ACTOR */
static const char *strSelectMoviesFromActor = "SELECT Movies.Title FROM Movies " \
                                              "INNER JOIN acted_in_movie, Actors ON (Movies.ID = acted_in_movie.MovieID) AND (acted_in_movie.ActorsID = Actors.ID) " \
                                              "WHERE Actors.Name LIKE :Name;";
/* SELECT Movies FROM DIRECTOR */
static const char *strSelectMoviesFromDirector = "SELECT Movies.Title " \
                                                 "FROM Movies INNER JOIN directed_movie, Directors " \
                                                 "ON (Movies.ID = directed_movie.MovieID) AND (directed_movie.DirectorID = Directors.ID) WHERE Directors.Name LIKE :Name;";
/* SELECT SERIES FROM ACTOR */
static const char *strSelectSeriesFromActor = "SELECT Series.Title FROM Series " \
                                              "INNER JOIN acted_in_series, Actors ON (Series.ID = acted_in_series.SeriesID) AND (acted_in_series.ActorsID = Actors.ID) " \
                                              "WHERE Actors.Name LIKE :Name;";
/* SELECT ACTORS OF MOVIE */
static const char *strSelectActorsOfMovie = "SELECT Actors.Name FROM Movies " \
                                            "INNER JOIN acted_in_movie, Actors ON (Movies.ID = acted_in_movie.MovieID) AND (acted_in_movie.ActorsID = Actors.ID) " \
                                            "WHERE Movies.Title LIKE :Title;";
/* SELECT ACTORS OF SERIES */
static const char *strSelectActorsOfSeries = "SELECT Actors.Name FROM Series " \
                                             "INNER JOIN acted_in_series, Actors ON (Series.ID = acted_in_series.SeriesID) AND (acted_in_series.ActorsID = Actors.ID) " \
                                             "WHERE Series.Title LIKE :Title;";
/* SELECT ALL ALREADY SEEN MOVIES */
static const char *strSelectSeenMovies = "SELECT Movies.Title FROM Movies WHERE Movies.AlreadySeen = 1;";
/* SELECT ALL FAVOURITE MOVIES */
static const char *strSelectFavouriteMovies = "SELECT Movies.Title FROM Movies WHERE Movies.IsFavourite = 1;";   
/* SELECT ALL ALREADY SEEN SERIES */
static const char *strSelectSeenSeries = "SELECT Series.Title, Series.Season FROM Series WHERE Series.AlreadySeen = 1;";
/* SELECT ALL FAVOURITE SERIES */
static const char *strSelectFavouriteSeries = "SELECT Series.Title, Series.Season FROM Series WHERE Series.IsFavourite = 1;";
/* SELECT ALL CURRENTLY UNSEEN MOVIES */
static const char *strSelectUnseenMovies = "SELECT Movies.Title FROM Movies WHERE Movies.AlreadySeen = 0;";         
/* SELECT ALL CURRENTLY UNSEEN SERIES */
static const char *strSelectUnseenSeries = "SELECT Series.Title, Series.Season FROM Series WHERE Series.AlreadySeen = 0;";               
/* DELETION STUFF */
/* DELETE MOVIE (directors and actors entries remain untouched) --> the trigger takes care of the deletion of dependencies */
static const char *strDeleteMovie = "DELETE FROM Movies WHERE ID = :movieID;";   
/* DELETE SERIES (actors entries remain untouched) --> the trigger takes care of the deletion of dependencies */
static const char *strDeleteSeries = "DELETE FROM Series WHERE ID = :seriesID;";       
/* DELETE ALL (RESET TO RELEASE-DATABASE) */
static const char *strResetDatabase[] = {"DELETE FROM acted_in_movie;", 
                                         "DELETE FROM acted_in_series;",
                                         "DELETE FROM Actors;", 
                                         "DELETE FROM directed_movie;", 
                                         "DELETE FROM Directors;", 
                                         "DELETE FROM Genres WHERE ID > 26;", 
                                         "DELETE FROM Movies;", 
                                         "DELETE FROM Qualities WHERE ID > 11;", 
                                         "DELETE FROM Series;"};        
/* UPDATE STUFF */
static const char *updateQueriesArr[] = {"UPDATE Movies SET Title = :Title WHERE ID = :ID;",
                                         "UPDATE Series SET Title = :Title WHERE ID = :ID;",
                                         "UPDATE Movies SET Genre = (SELECT ID FROM Genres WHERE Genre = :Genre) WHERE ID = :ID;",
                                         "UPDATE Series SET Genre = (SELECT ID FROM Genres WHERE Genre = :Genre) WHERE ID = :ID;",
                                         "UPDATE Movies SET ReleaseYear = :Year WHERE ID = :ID;",
                                         "UPDATE Series SET Season = :Season WHERE ID = :ID;",
                                         "UPDATE Movies SET Runtime = :Runtime WHERE ID = :ID;",
                                         "UPDATE Series SET ReleaseYear = :Year WHERE ID = :ID;",
                                         "UPDATE Movies SET Quality = (SELECT ID FROM Qualities WHERE Source = :Src) WHERE ID = :ID;",
                                         "UPDATE Series SET Quality = (SELECT ID FROM Qualities WHERE Source = :Src) WHERE ID = :ID;",
                                         "UPDATE Movies SET Rating = :Rating WHERE ID = :ID;",
                                         "UPDATE Series SET Rating = :Rating WHERE ID = :ID;",
                                         "UPDATE Movies SET CommunityRating = :ComRating WHERE ID = :ID;"
                                         "UPDATE Series SET CommunityRating = :ComRating WHERE ID = :ID;",
                                         "UPDATE Movies SET AlreadySeen = :Seen WHERE ID = :ID;",
                                         "UPDATE Series SET AlreadySeen = :Seen WHERE ID = :ID;",
                                         "UPDATE Movies SET IsFavourite = :Fav WHERE ID = :ID;",
                                         "UPDATE Series SET IsFavourite = :Fav WHERE ID = :ID;",
                                         "UPDATE Movies SET ArchiveStr = :Archive WHERE ID = :ID;",
                                         "UPDATE Series SET ArchiveStr = :Archive WHERE ID = :ID;"};

/*****************************************
 ***    other function declarations    ***
 *****************************************/
void printError(char *, bool);
bool openDatabase();
bool closeDatabase();
static int prepareStatementIfNeeded(sqlite3_stmt *, const char *);   // IF the statement is not used yet, prepare it...
void closePreparedStatements(sqlite3_stmt *);
static void checkResultCode(int);
static void bind_param_index(sqlite3_stmt *, int *, char *);
// "void" oder "bool" bei INSERTs ???
void insertMovie(ctx_movieInfo *);
void insertSeries(ctx_seriesInfo *);
void deleteMovie(/* char * */ unsigned int);
void deleteSeries(/* char * */ unsigned int);
static void updateQuery(char *, char *[]);   // general purpose update function
// the specific functions (they call the general function "updateQuery")
// "updateMovieTitle", "updateSeriesTitle", "updateMovieGenre" and so on...
// "void" oder "bool" ?
void updateMovieTitle(unsigned int, char *);
void updateSeriesTitle(unsigned int, char *);
void updateMovieGenre(unsigned int, TGenres);
void updateSeriesGenre(unsigned int, TGenres);
void updateMovieReleaseYear(unsigned int, unsigned short);
void updateSeriesSeason(unsigned int, unsigned short);
void updateMovieRuntime(unsigned int, const char *);
void updateSeriesReleaseYear(unsigned int, unsigned short);
void updateMovieQuality(unsigned int, TQualities);
void updateSeriesQuality(unsigned int, TQualities);
void updateMovieRating(unsigned int, unsigned short);
void updateSeriesRating(unsigned int, unsigned short);
void updateMovieCommunityRating(unsigned int, unsigned short);
void updateSeriesCommunityRating(unsigned int, unsigned short);
void updateMovieAlreadySeen(unsigned int, bool);
void updateSeriesAlreadySeen(unsigned int, bool);
void updateMovieIsFavourite(unsigned int, bool);
void updateSeriesIsFavourite(unsigned int, bool);
void updateMovieArchive(unsigned int, const char *);
void updateSeriesArchive(unsigned int, const char *);

//...

#endif
