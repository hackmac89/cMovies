/*
File: log.h
Synopsis: header with function prototype for log.c code file
Author: hackmac89
E-mail: hackmac89@filmdatenbank-manager.de
date: 08/23/2016
https://www.filmdatenbank-manager.de/
https://github.com/hackmac89/cMovies
*/
#ifndef LOG_H
#define LOG_H
// include main header
//#include "dbutils.h" ==> would allow access to "FILE", "time_t" etc. but the other stuff except for "static" functions would be accessible, too
#include <stdlib.h>
#include <stdio.h>
#include <string.h>   // necessary for "strlen"   
#include <time.h>
#include <stdarg.h>   // to use variable function arguments list

/*********************
 ***   CONSTANTS   ***
 *********************/
#define LOGNAME "cmovies.log"   // the filename of our logfile
/************************
 *** global variables ***
 ************************/
static FILE *logFile = NULL;
/*************************************
 *** other function declarations   ***
 *************************************/
int appendToLog(const char *formatString, ...);   // append logging information to the logfile instead of creating several logfiles
#endif

