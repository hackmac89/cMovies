/*
File: log.c
Synopsis: implementation of the logging functionality
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 08/23/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies
*/

// IMPORTS
#include "log.h"

/* 
	Append logging info messages to the logfile.
	The append-mode creates the file if it´s not existing yet, 
  	so there is no need to provide a separate "createFile" function.
    ----------------------------------------------------------------
    @param const char *formatString - The format string which defines how to format the output of the following arguments
    @param ... - the variable argument list 

    ..:: EXAMPLES ::..
    appendToLog("Message") ==> no format, no other arguments. Results in "[TIMESTAMP] : Message"
    appendToLog("%s\n", "Some other message") ==> format string with 1 argument. Results in "[TIMESTAMP] : Some other message"
    The behaviour is like "printf" & co., but for files.
*/ 
int appendToLog(const char *formatString, ...)
{
	// Declarations / Initializations
	time_t timer;
    char buffer[26];
    struct tm* tm_info;
    char *output = malloc((sizeof(char *)) * 256);
    va_list vl;
    int done = -1;

    // fill struct
    time(&timer);
    tm_info = localtime(&timer);
	strftime(buffer, 26, "[%d.%m.%Y %H:%M:%S]", tm_info);
	sprintf(output, "%s : ", buffer);   // save time to string 
	strncat(output, formatString, strlen(formatString));   // append the format string to the time string
	strncat(output, "\n", 1);   // append a newline character

	va_start(vl, formatString);
	if( (logFile = fopen(LOGNAME, "a")) != NULL )
	{
		done = vfprintf(logFile, output, vl);   // work the variable arguments list...
		fclose(logFile);   // free ressources
	}

	va_end(vl);
	free(output);
	return done;
}
