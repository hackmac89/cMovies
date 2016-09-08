/*
File: cmovies.c
Synopsis: main application code (entry point)
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 08/23/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies

MIT License
Copyright (c) 2016 Jeff Wagner
Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

// include main header
#include "cmovies.h"

// for testing purposes
ctx_movieInfo *testCaseMovie;
ctx_seriesInfo *testCaseSeries;

/*
    Main Function / Program Entry Point
*/
int main(int argc, char **argv)
{
	printf("\t\t__--//cMovies [v.%s] ALPHA\\\\--__\n\n", VERSION);

	appendToLog("cMovies[v.%s] was started", VERSION);
    appendToLog("Opening the \"cMovies\" database");
    
    if( !openDatabase() )
    {
        printError("[!] ERROR : Unable to open the \"movies.db\" database file (%s) !\n", true);
    }
    else
    {
        appendToLog("Database successfully opened");

        // initialize our test-case contexts
        testCaseMovie = initMovieContext();
        testCaseSeries = initSeriesContext();

        /*
             -----------------------------------
            |               Auswahl             |
             -----------------------------------
            |   1 = INSERT MOVIE #1             |
            |   2 = INSERT MOVIE #2             |
            |   3 = INSERT SERIES #1            |
            |   4 = INSERT SERIES #2            |
            |   5 = DELETE MOVIE                |
            |   6 = DELETE SERIES               |
            |   7 = (UPDATE DB)                 |
            |   q = QUIT                        |
            |                                   |
             -----------------------------------
        */
        printf(" ----------------------------------- \n|\t   Choose wisely\t    |\n|-----------------------------------|\n" \
            "|\t1 = INSERT MOVIE #1\t    |\n|\t2 = INSERT MOVIE #2\t    |\n|\t3 = INSERT SERIES #1\t    |\n" \
            "|\t4 = INSERT SERIES #2\t    |\n|\t5 = DELETE MOVIE\t    |\n|\t6 = DELETE SERIES\t    |\n|\tq = QUIT\t\t    |\n|\t\t\t\t    |\n ----------------------------------- \n");
    
        int choice = -1;
        do
        {              
            if(choice != '\n')
            {
                switch(choice)
                {
                    case '1' : {
                                /* 
                                    TEST-CASE #1:
                                    ------------
                                    Insert movie #1
                                */
                                appendToLog("Adding basic movie informations to the current context");
                                printf("DEBUG INSERT MOVIE #1 (Star Wars)\n");
                        
                                /* Add basic info (e.g. title, plot, release year etc.) about the movie to the context */
                                if( !(addBasicInfoToMovieContext(&testCaseMovie, "Star Wars: Das Erwachen der Macht", setGenre(TSCIENCEFICTION), "02:15:00",
                                    "Mehr als drei Jahrzehnte nach „Star Wars 6 – Die Rückkehr der Jedi-Ritter“ wurde das Imperium durch die „Erste Ordnung“ abgelöst, " \
                                    "eine ebenfalls diktatorische Organisation mit anderem Namen, die Krieg gegen den Widerstand führt...", setQuality(TBRRIP), "HDD", 2015, 0, 8, false, false)) )
                                        printError("[!] ERROR while inserting the basic informations into the movie context. Aborting...", true);
                                /* Add director(s) and actor(s) to the movie context */
                                addDirectorToMovieContext(&testCaseMovie, "J. J. Abrams");
                                addActorToMovieContext(&testCaseMovie, "Daisy Ridley");

                                insertMovie(testCaseMovie);
    
                                // call the "reinitialization" function to free the current context and to create a new one
                                testCaseMovie = reinitializeMovieContext(&testCaseMovie);
                                printf("D0N3 !\n");
                                break;
                    }
                    case '2' : {
                                /* 
                                    TEST-CASE #2:
                                    ------------
                                    Insert movie #2
                                */
                                appendToLog("Adding basic movie informations to the current context");
                                printf("DEBUG INSERT MOVIE #2 (Zoomania)\n");

                                if( !(addBasicInfoToMovieContext(&testCaseMovie, "Zoomania", setGenre(TANIMATION), "01:49:00",
                                    "In einer von anthropomorphen Säugetieren bewohnten Welt erfüllt Judy Hopps aus dem ländlichen Dorf Bunnyborrow in Nageria ihren Traum, " \
                                    "als erster Hase Polizist zu werden...", setQuality(TBRRIP), "HDD", 2015, 8, 8, true, true)) )
                                        printError("[!] ERROR while inserting the basic informations into the movie context. Aborting...", true);
                                /* Add director(s) and actor(s) to the movie context */
                                addDirectorToMovieContext(&testCaseMovie, "Byron Howard");
                                addDirectorToMovieContext(&testCaseMovie, "Rich Moore");
                                addActorToMovieContext(&testCaseMovie, "Ginnifer Goodwin");
                                addActorToMovieContext(&testCaseMovie, "Jason Bateman");
                                addActorToMovieContext(&testCaseMovie, "Idris Elba");
                                addActorToMovieContext(&testCaseMovie, "Jenny Slate");
                                addActorToMovieContext(&testCaseMovie, "Nate Torrence");
                                addActorToMovieContext(&testCaseMovie, "Bonnie Hunt");
                                addActorToMovieContext(&testCaseMovie, "Don Lake");
                                addActorToMovieContext(&testCaseMovie, "Tommy Chong");
                                addActorToMovieContext(&testCaseMovie, "J. K. Simmons");
                                addActorToMovieContext(&testCaseMovie, "Alan Tudyk");

                                insertMovie(testCaseMovie);
    
                                // call the "reinitialization" function to free the current context and to create a new one
                                testCaseMovie = reinitializeMovieContext(&testCaseMovie);
                                printf("D0N3 !\n");
                                break;
                    }
                    case '3' : {
                                /* 
                                    TEST-CASE #3:
                                    ------------
                                    Insert a series #1
                                */
                                appendToLog("Adding basic series informations to the current context");
                                printf("DEBUG INSERT SERIES #1 (The Big Bang Theory)\n");

                                /* Add basic info (e.g. title, plot, release year etc.) about the series to the context */
                                if( !(addBasicInfoToSeriesContext(&testCaseSeries, "The Big Bang Theory", "Sitcom",
                                    "Die Serie handelt von den hochintelligenten jungen Physikern Leonard Hofstadter und Sheldon Cooper, deren WG gegenüber der Wohnung der hübschen Kellnerin Penny liegt. " \
                                    "Dabei wird die geekhafte Art der beiden Wissenschaftler durch die Naivität, aber auch die Sozialkompetenz bzw. den gesunden Menschenverstand der Nachbarin, einer klischeehaften Blondine, kontrastiert",
                                    "DVD-RIP", "DVD0xM0x", 1, 2007, 9, 9, true, true)) )
                                        printError("[!] ERROR while inserting the basic informations into the series context. Aborting...", true);
                                /* Add actor(s) to the series context */
                                addActorToSeriesContext(&testCaseSeries, "Johnny Galecki");
                                addActorToSeriesContext(&testCaseSeries, "Jim Parsons");
                                addActorToSeriesContext(&testCaseSeries, "Kaley Cuoco");
                                addActorToSeriesContext(&testCaseSeries, "Simon Helberg");
                                addActorToSeriesContext(&testCaseSeries, "Kunal Nayyar");
                                addActorToSeriesContext(&testCaseSeries, "Sara Gilbert");
                                addActorToSeriesContext(&testCaseSeries, "Melissa Rauch");
                                addActorToSeriesContext(&testCaseSeries, "Mayim Bialik");
                                addActorToSeriesContext(&testCaseSeries, "Kevin Sussman");
                                addActorToSeriesContext(&testCaseSeries, "Laura Spencer");

                                insertSeries(testCaseSeries);

                                testCaseSeries = reinitializeSeriesContext(&testCaseSeries);
                                printf("D0N3 !\n");
                                break;
                    }
                    case '4' : {
                                /* 
                                    TEST-CASE #4:
                                    ------------
                                    Insert a series #2
                                */
                                appendToLog("Adding basic series informations to the current context");
                                printf("DEBUG INSERT SERIES #2 (The King of Queens)\n");

                                /* Add basic info (e.g. title, plot, release year etc.) about the series to the context */
                                if( !(addBasicInfoToSeriesContext(&testCaseSeries, "The King of Queens", "Sitcom",
                                    "Die Serie handelt von Douglas Heffernan, einem übergewichtigen Kurierfahrer aus dem New Yorker Stadtteil Queens und seiner Frau Carrie, einer Rechtsanwaltsgehilfin in Manhattan...",
                                    "DVD-RIP", "DVD0xM0x", 1, 1998, 9, 9, true, true)) )
                                        printError("[!] FEHLER beim eintragen der Basisinformationen. Breche ab...", true);
                                /* Add actor(s) to the series context */
                                addActorToSeriesContext(&testCaseSeries, "Kevin James");
                                addActorToSeriesContext(&testCaseSeries, "Leah Remini");
                                addActorToSeriesContext(&testCaseSeries, "Jerry Stiller");
                                addActorToSeriesContext(&testCaseSeries, "Victor Williams");
                                addActorToSeriesContext(&testCaseSeries, "Patton Oswalt");
                                addActorToSeriesContext(&testCaseSeries, "Gary Valentine");

                                insertSeries(testCaseSeries);

                                testCaseSeries = reinitializeSeriesContext(&testCaseSeries);
                                printf("D0N3 !\n");
                                break;
                    }
                    case '5' : {
                                /*
                                    TEST-CASE #5:
                                    ------------
                                    Delete the first movie (just as an example)
                                */
                                appendToLog("Deleting movie with ID 1");
                                printf("DEBUG DELETE MOVIE\n");

                                deleteMovie(1);
                                printf("D0N3 !\n");
                                break;
                    }
                    case '6' : {
                                /*
                                    TEST-CASE #6:
                                    ------------
                                    Delete the second series (just as an example)
                                */
                                appendToLog("Deleting series with ID 2");
                                printf("DEBUG DELETE SERIES\n");

                                deleteSeries(2);
                                printf("D0N3 !\n");
                                break;
                    }
                    default : break;
                }   
            }
            fflush(stdin);
        }
        while(toupper(choice = getchar()) != 'Q');

        appendToLog("Freeing the movie context");
        freeMovieContext(&testCaseMovie);
        appendToLog("Freeing the series context");
        freeSeriesContext(&testCaseSeries);

        appendToLog("Closing the database connection(s)");
        
        closePreparedStatements(stmtSqlInsertMovie);
        closePreparedStatements(stmtSqlInsertSeries);
        closePreparedStatements(stmtSqlUpdate);
        closePreparedStatements(stmtSqlDeletion);
        if( !closeDatabase() )
            printError("[!] ERROR : Unable to properly close the \"movies.db\" database !\n", true);
        else
            appendToLog("Database successfully closed");
    }

    appendToLog("Closing cMovies2016");
	return EXIT_SUCCESS;
}

/*
    Initialize our movies context structure
    by allocating memory etc.
    ----------------------------------------
    @param none
*/
ctx_movieInfo* initMovieContext(void)
{
    appendToLog("Creating the movies context");
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
            printError("[!] ERROR : Unable to allocate memory space for directors name in \"initMovieContext\" !\n", true);
    for(int i = 0; i <= 9; i++)
        if( (movieInfo->actors[i] = malloc( 51 * sizeof(char *) )) == NULL)   // max 51 characters per actors name
            printError("[!] ERROR : Unable to allocate memory space for actors name in \"initMovieContext\" !\n", true);
    movieInfo->directors[2] = NULL;
    movieInfo->actors[10] = NULL;
    
    return movieInfo;
}

/*
    Initialize our series context structure
    by allocating memory etc.
    ----------------------------------------
    @param none
*/
ctx_seriesInfo* initSeriesContext(void)
{
    appendToLog("Creating the series context");
    ctx_seriesInfo *seriesInfo = malloc(sizeof(ctx_seriesInfo));
    seriesInfo->title = malloc(sizeof(char *) * 101);
    seriesInfo->genre = malloc(sizeof(char *) * 51);   // frei Schnauze
    seriesInfo->plot = malloc(sizeof(char *) * 32768);   // string with sufficient space
    seriesInfo->sourceQuality = malloc(sizeof(char *) * 50);
    seriesInfo->archived = malloc(sizeof(char *) * 261);   // 260 --> max DOS Pathlength + 1 Nullbyte
    seriesInfo->cntActors = 0;

    for(int i = 0; i <= 9; i++)
        if( (seriesInfo->actors[i] = malloc( 51 * sizeof(char *) )) == NULL)   // max 51 characters per actors name
            printError("[!] ERROR : Unable to allocate memory space for actors name in \"initSeriesContext\" !\n", true);
    seriesInfo->actors[10] = NULL;

    return seriesInfo;
}

/*
    Free the resources used by our movies context structure
    (call with "&")
    -------------------------------------------------------
    @param ctx_moviesInfo **moviesInfo - The structure to be freed
*/
void freeMovieContext(ctx_movieInfo **movieInfo)
{
    appendToLog("Deleting the ADTs <MovieContext>");
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

/*
    Free the resources used by our series context structure
    (call with "&")
    -------------------------------------------------------
    @param ctx_seriesInfo **seriesInfo - The structure to be freed
*/
void freeSeriesContext(ctx_seriesInfo **seriesInfo)
{
    appendToLog("Deleting the ADTs <SeriesContext>");
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

/*
    Set the movies´/series´ genre
    -------------------------------------------------------
    @param TGenres genre - the enum corresponding to the chosen genre
    @return const char* - the genre string to query the db
*/
static char *setGenre(TGenres genre)
{
    switch(genre){
        case TACTION:	return "Action";
        case TADVENTURE:	return "Adventure";
        case TANIMATION:	return "Animation";
        case TANIME:    return "Anime";
        case TCOMEDY:   return "Comedy";
        case TCRIME:    return "Crime";
        case TDRAMA:    return "Drama";
        case TDOCUMENTARY:  return "Documentary";
        case TEASTERN:  return "Eastern";
        case TEPIC: return "Epic";
        case TFANTASY:  return "Fantasy";
        case TFILMNOIR: return "Film noir";
        case THISTORY:  return "History";
        case THORROR:   return "Horror";
        case TMARTIALARTS:  return "Martial-Arts";
        case TMELODRAMA:    return "Melodrama";
        case TMOCKUMENTARY: return "Mockumentary";
        case TMYSTERY:  return "Mystery";
        case TPORN: return "Porn"; 
        case TROMANCE:  return "Romance";
        case TSCIENCEFICTION:   return "Science-Fiction";
        case TSITCOM:   return "Sitcom";
        case TTHRILLER: return "Thriller";
        case TTRAGICOMEDY:  return "Tragicomedy";
        case TWESTERN:  return "Western";
        default:	return "Other";
    }
}

/*
 Set the movies´/series´ quality
 -------------------------------------------------------
 @param TQualities quality - the enum corresponding to the chosen quality
 @return const char* - the quality string to query the db
 */
static char *setQuality(TQualities quality)
{
    switch(quality){
        case T2K:   return "2K-UHD";
        case T4K:   return "4K-UHD";
        case TBR:   return "BR";
        case TBRRIP:    return "BR-RIP";
        case TCAM:  return "CAM";
        case TDVD:  return "DVD";
        case TDVDRIP:   return "DVD-RIP";
        case TDVDSCREENER:  return "DVDScreener";
        case TTELESYNC: return "Telesync";
        case TTSLD: return "TS-LD";
        case TTSMD: return "TS-MD";
        case TTVRIP:    return "TV-RIP";
        case TVHS:  return "VHS";
        case TWEBRIP:   return "WEB-RIP";
        default:    return "Other";
    }
}

/* 
    Add the users' information related to a movie to our movie-context
    -------------------------------------------------------
    @param ctx_movieInfo **movieInfo - The movie context which we want to fill the information into
    @param char *title - The movie title
    @param char *genre - The movie genre
    @param char *runtime - The runtime of the movie
    @param char *plot - The plot of the movie
    @param char *sourceQuality - The source where the movie is from (VHS, DVD, RiP etc.)
    @param char *archived - The place where you archived the movie
    @param unsigned short year - The release year of the movie
    @param unsigned short myRating - Your rating for the movie (0-10)
    @param unsigned short communityRating - Other peoples (from rottentomatoes, metacritics etc.) rating for the movie
    @param bool seen - true, if you´ve already seen it, false otherwise
    @param bool favourite - true, if the movie is one of your favourites, false otherwise

    @return bool - true, if no error occured, false otherwise 
*/
static bool addBasicInfoToMovieContext(ctx_movieInfo **movieInfo, char *title, const char *genre, char *runtime, char *plot,
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

/* 
    Add up to 2 directors of one movie into the movie context
    -------------------------------------------------------
    @param ctx_movieInfo **movieInfo - The movie context which we want to fill the information into
    @param char *director - The directors name 
*/
static void addDirectorToMovieContext(ctx_movieInfo **movieInfo, char *director)
{
    appendToLog("Adding the director \"%s\" to the current movie context", director);
    if( *movieInfo != NULL ){
        if( (*movieInfo)->cntDirectors < 2 ){
            strncpy((*movieInfo)->directors[(*movieInfo)->cntDirectors], director, strlen(director) + 1);
            (*movieInfo)->cntDirectors++;
        };
    }
}

/* 
    Add up to 10 actors of one movie into the movie context
    -------------------------------------------------------
    @param ctx_movieInfo **movieInfo - The movie context which we want to fill the information into
    @param char *actor - The actors name
*/
static void addActorToMovieContext(ctx_movieInfo **movieInfo, char *actor)
{
    appendToLog("Adding the actor \"%s\" to the current movies context", actor);
    if( *movieInfo != NULL ){
        if( (*movieInfo)->cntActors < 10 ){
            strncpy((*movieInfo)->actors[(*movieInfo)->cntActors], actor, strlen(actor) + 1);
            (*movieInfo)->cntActors++;
        };
    }
}

/* 
    Add the users' information related to a series to our series-context
    -------------------------------------------------------
    @param ctx_seriesInfo **seriesInfo - The series context which we want to fill the information into
    @param char *title - The series title
    @param char *genre - The series genre
    @param char *plot - The plot of the series
    @param char *sourceQuality - The source where the series is from (TV, VHS, DVD, RiP etc.)
    @param char *archived - The place where you archived the series
    @param unsigned short season - The season of the series which you want to fill into the context
    @param unsigned short year - The release year of the series (season)
    @param unsigned short myRating - Your rating for the series (0-10)
    @param unsigned short communityRating - Other peoples (from rottentomatoes, metacritics etc.) rating for the series
    @param bool seen - true, if you´ve already seen it, false otherwise
    @param bool favourite - true, if the series is one of your favourites, false otherwise

    @return bool - true, if no error occured, false otherwise  
*/
static bool addBasicInfoToSeriesContext(ctx_seriesInfo **seriesInfo, char *title, const char *genre, char *plot, char *sourceQuality, char *archived,
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

/* 
    Add up to 10 actors of one series into the series context
    -------------------------------------------------------
    @param ctx_seriesInfo **seriesInfo - The series context which we want to fill the information into
    @param char *actor - The actors name 
*/
static void addActorToSeriesContext(ctx_seriesInfo **seriesInfo, char *actor)
{
    appendToLog("Adding the actor \"%s\" to the current series context", actor);
    if( (*seriesInfo)->cntActors < 10 ){
        strncpy((*seriesInfo)->actors[(*seriesInfo)->cntActors], actor, strlen(actor) + 1);
        (*seriesInfo)->cntActors++;
    }
}

/*
    A function to "reinitialize" a movie context.
    Basically all it does is to free the resources of the old context and 
    then creating a new movie context.
    -------------------------------------------------------
    @param ctx_movieInfo **movieInfo - The current movie context to free
    @return ctx_movieInfo* - After freeing the old context, return a newly created
*/
static ctx_movieInfo* reinitializeMovieContext(ctx_movieInfo **movieInfo)
{
    appendToLog("Reinitialize the movie context for further usage");
    freeMovieContext(movieInfo);
    return initMovieContext();
}

/*
    A function to "reinitialize" a series context.
    Basically all it does is to free the resources of the old context and 
    then creating a new series context.
    -------------------------------------------------------
    @param ctx_seriesInfo **seriesInfo - The current series context to free
    @return ctx_seriesInfo* - After freeing the old context, return a newly created
*/
static ctx_seriesInfo* reinitializeSeriesContext(ctx_seriesInfo **seriesInfo)
{
    appendToLog("Reinitialize the series context for further usage");
    freeSeriesContext(seriesInfo);
    return initSeriesContext();
}
