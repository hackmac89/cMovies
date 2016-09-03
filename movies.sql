/**
	cMovies SQL Script
	-----------------------------------------------------
	Prog : cMovies
	Version : 3.5
	Description : SQL statements to create the database file
	Date : 22.08.16
	Author : Jeff Wagner
	Info : https://www.sqlite.org/datatype3.html
*/ 

-- Enable Auto-Vacuum to shrink database filesize after deletion of database entries
PRAGMA auto_vacuum=FULL;

CREATE TABLE Movies
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	Title VARCHAR(100) NOT NULL COLLATE NOCASE,
	Genre INTEGER NOT NULL,
	ReleaseYear INTEGER CHECK(ReleaseYear BETWEEN 1800 AND 2200),
	Runtime TEXT NOT NULL DEFAULT "00:00:00",   -- TEXT	the runtime in "HH:MM:SS" format.
	Plot TEXT NOT NULL DEFAULT "", 
	Quality INTEGER NOT NULL,
	Rating INTEGER DEFAULT 0 CHECK(Rating BETWEEN 0 AND 10),
	CommunityRating INTEGER DEFAULT 0,   -- eventually add a check here, too, but the various online databases use different rating patterns
	Added datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
	AlreadySeen TINYINT CHECK(AlreadySeen BETWEEN 0 AND 1) DEFAULT 0,   -- do it that way, because there is no boolean builtin type in SQLite 
	IsFavourite TINYINT CHECK(IsFavourite BETWEEN 0 AND 1) DEFAULT 0,
	ArchiveStr VARCHAR(260) DEFAULT "",   -- max. pathlen on windows machines = 260, in case a path is given
	FOREIGN KEY(Genre) REFERENCES Genres(ID),
	FOREIGN KEY(Quality) REFERENCES Qualities(ID)
);

CREATE TABLE Series
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	Title VARCHAR(100) NOT NULL COLLATE NOCASE,
	Genre INTEGER NOT NULL,
	Season TINYINT NOT NULL DEFAULT 1,
	-- eventually add another field for "number of episodes" for a better granularity
	ReleaseYear INTEGER CHECK(ReleaseYear BETWEEN 1800 AND 2200),
	Plot TEXT NOT NULL DEFAULT "",
	Quality INTEGER NOT NULL,
	Rating INTEGER DEFAULT 0 CHECK(Rating BETWEEN 0 AND 10),
	CommunityRating INTEGER DEFAULT 0,   -- eventually add a check here, too, but the various online databases use different rating patterns
	Added datetime NOT NULL DEFAULT CURRENT_TIMESTAMP,
	AlreadySeen TINYINT CHECK(AlreadySeen BETWEEN 0 AND 1) DEFAULT 0,   -- do it that way, because there is no boolean builtin type in SQLite 
	IsFavourite TINYINT CHECK(IsFavourite BETWEEN 0 AND 1) DEFAULT 0,
	ArchiveStr VARCHAR(260) DEFAULT "",   -- max. pathlen on windows machines = 260, in case a path is given
	FOREIGN KEY(Genre) REFERENCES Genres(ID),
	FOREIGN KEY(Quality) REFERENCES Qualities(ID)
);

CREATE TABLE Genres
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	Genre VARCHAR(50) UNIQUE NOT NULL
);

-- INSERTs (predefined Genre types)
INSERT INTO Genres VALUES(1, "Action");
INSERT INTO Genres VALUES(2, "Adventure");
INSERT INTO Genres VALUES(3, "Animation");
INSERT INTO Genres VALUES(4, "Comedy");
INSERT INTO Genres VALUES(5, "Crime");
INSERT INTO Genres VALUES(6, "Drama");
INSERT INTO Genres VALUES(7, "Documentary");
INSERT INTO Genres VALUES(8, "Eastern");
INSERT INTO Genres VALUES(9, "Epic");
INSERT INTO Genres VALUES(10, "Fantasy");
INSERT INTO Genres VALUES(11, "Film noir");
INSERT INTO Genres VALUES(12, "History");
INSERT INTO Genres VALUES(13, "Horror");
INSERT INTO Genres VALUES(14, "Martial-Arts");
INSERT INTO Genres VALUES(15, "Melodrama");
INSERT INTO Genres VALUES(16, "Mockumentary");
INSERT INTO Genres VALUES(17, "Mystery");
INSERT INTO Genres VALUES(18, "Romance");
INSERT INTO Genres VALUES(19, "Science-Fiction");
INSERT INTO Genres VALUES(20, "Sitcom");
INSERT INTO Genres VALUES(21, "Thriller");
INSERT INTO Genres VALUES(22, "Tragicomedy");
INSERT INTO Genres VALUES(23, "Western");
INSERT INTO Genres VALUES(24, "Other");

CREATE TABLE Qualities
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	Source VARCHAR(50) UNIQUE NOT NULL
);

-- INSERTs (predefined quality types)
INSERT INTO Qualities VALUES(1, "2K-UHD");
INSERT INTO Qualities VALUES(2, "4K-UHD");
INSERT INTO Qualities VALUES(3, "BR");
INSERT INTO Qualities VALUES(4, "BR-RIP");
INSERT INTO Qualities VALUES(5, "CAM");
INSERT INTO Qualities VALUES(6, "DVD");
INSERT INTO Qualities VALUES(7, "DVD-RIP");
INSERT INTO Qualities VALUES(8, "DVDScreener");
INSERT INTO Qualities VALUES(9, "Telesync");
INSERT INTO Qualities VALUES(10, "TS-LD");
INSERT INTO Qualities VALUES(11, "TS-MD");
INSERT INTO Qualities VALUES(12, "TV-RIP");
INSERT INTO Qualities VALUES(13, "VHS");
INSERT INTO Qualities VALUES(14, "WEB-RIP");
INSERT INTO Qualities VALUES(15, "Other");

CREATE TABLE Directors
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	Name VARCHAR(75) UNIQUE NOT NULL
);

CREATE TABLE directed_movie
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	MovieID INTEGER NOT NULL,
	DirectorID INTEGER NOT NULL,
	FOREIGN KEY(MovieID) REFERENCES Movies(ID),
	FOREIGN KEY(DirectorID) REFERENCES Directors(ID) 
);

CREATE TABLE Actors
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	Name VARCHAR(75) UNIQUE NOT NULL
);

CREATE TABLE acted_in_movie
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	MovieID INTEGER NOT NULL,
	ActorsID INTEGER NOT NULL,
	FOREIGN KEY(MovieID) REFERENCES Movies(ID),
	FOREIGN KEY(ActorsID) REFERENCES Actors(ID) 
);

CREATE TABLE acted_in_series
(
	ID INTEGER PRIMARY KEY AUTOINCREMENT,
	SeriesID INTEGER NOT NULL,
	-- eventually add a separate field for "seasonID" later, to get a finer granularity which actors were in which season ...
	--SeasonID TINYINT NOT NULL,
	ActorsID INTEGER NOT NULL,
	FOREIGN KEY(SeriesID) REFERENCES Series(ID),
	--FOREIGN KEY(SeasonID) REFERENCES Series(Season), 
	FOREIGN KEY(ActorsID) REFERENCES Actors(ID) 
);

-- INDEXES
-- ??? alternative WITHOUT using indexes : add "unique(<<columns>>)" to <<tablename>> ???
CREATE UNIQUE INDEX   -- unique index (hash) per row of table "acted_in_movie" (given that, it´s possible to insert an actor mutliple times, but not for the same movie)
        UX_acted_in
ON      acted_in_movie(MovieID, ActorsID);   

CREATE UNIQUE INDEX   -- dito
        UX_directed_movie
ON      directed_movie(MovieID, DirectorID);

CREATE UNIQUE INDEX 
		UX_Movies
ON 		Movies(Title, ReleaseYear);   -- to allow inserting movies with the same title, but different release years (e.g for originals and remakes) 

CREATE UNIQUE INDEX
		UX_Series
ON 		Series(Title, Season);   -- to allow inserting a series multiple times, as long as the season is different

CREATE UNIQUE INDEX  -- given that, it´s possible to insert an actor mutliple times, but not for the same series
		UX_acted_in_series
ON 		acted_in_series(SeriesID, ActorsID);

-- TRIGGERS (trigger events on delete-operations to reduce the later C code for deleting an database entry) 
-- delete entries with foreign key dependencies before deleting the actual movie entry
CREATE TRIGGER tgr_movies_deletion
BEFORE DELETE
ON Movies FOR EACH ROW
BEGIN
	DELETE FROM acted_in_movie
	WHERE acted_in_movie.MovieID = (
										SELECT Movies.ID
										FROM Movies
										WHERE Movies.Title = old.Title
									);
	DELETE FROM directed_movie
	WHERE directed_movie.MovieID = (
										SELECT Movies.ID
										FROM Movies
										WHERE Movies.Title = old.Title
									);
END;

-- delete entries with foreign key dependencies before deleting the actual series entry
CREATE TRIGGER tgr_series_deletion
BEFORE DELETE
ON Series FOR EACH ROW
BEGIN
	DELETE FROM acted_in_series
	WHERE acted_in_series.SeriesID = (
										SELECT Series.ID
										FROM Series
										WHERE Series.Title = old.Title
									);
END;
