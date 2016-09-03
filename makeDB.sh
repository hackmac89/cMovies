#!/bin/bash
#
# SCRIPT: makeDB.sh
# AUTHOR: Jeff Wagner
# DATE: 25.01.2016
# REV: 1.1.P (valid values are A, B, D, T and P)
# (for Alpha, Beta, Dev, Test und Production)
#
# PLATFORM: Not platform dependent
#
# PURPOSE: call sqlite3 with the sql-statements inside 
#          the "new_db_design.sql" file to create our 
#          "movies"-database with the needed tables and
#          predefined values.
#
# REV LIST:
# DATE: ETA
# BY: ???
# MODIFICATION: 08/22/2016 - Added purpose description
#
##########################################################
# DEFINE FILES AND VARIABLES HERE
##########################################################
DBPATH="./bin/movies.db"
BACKUPPATH="./bin/DBBACKUP"

##########################################################
# DEFINE FUNCTIONS HERE
##########################################################
function checkIfDBAlreadyExists()
{
	[ -f $DBPATH ] ; return $?
}

function createBACKUP()
{
	if [ -d $BACKUPPATH ]; then
		$(mv $DBPATH $BACKUPPATH/movies.db)
		return $?
	else
		$(mkdir $BACKUPPATH && mv $DBPATH $BACKUPPATH/movies.db)
		return $?
	fi	
}

##########################################################
# BEGINNING OF MAIN
##########################################################
# Bereits existierende sqlite DB "löschen"
if checkIfDBAlreadyExists; then
	printf "%s\n" "[+] Creating backup of existing database in \"${BACKUPPATH}\"..."
	printf "%s\n" "[+] Deleting existing database..."   # genauer gesagt verschiebe ich sie nur :)
	if createBACKUP; then
		printf "\t%s\n%s\n" "[+] Everything went fine." "[+] D0n3 !"
	else
		printf "\t%s\n%s\n" "[+] Opps. Something went wrong while creating the BACKUP-file! Aborting..."	
		exit 1
	fi 
fi

printf "%s\n" "[+] Creating the database..."
 
if $(sqlite3 "./bin/movies.db" < movies.sql); then
	printf "%s\n" "[+] D0n3 ! The Database was successfully created."
	exit 0
fi