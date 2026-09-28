PRAGMA foreign_keys = ON;

CREATE TABLE Genre (
    Genre_ID INTEGER PRIMARY KEY,
    Genre TEXT
);

CREATE TABLE Author (
    Author_ID INTEGER PRIMARY KEY,
    Author TEXT
);

CREATE TABLE Album (
    Album_ID INTEGER PRIMARY KEY,
    Album TEXT,
    Albume_Release DATE
);

CREATE TABLE Playlist (
    Playlist_ID INTEGER PRIMARY KEY,
    Playlist TEXT,
    Playlist_Release DATE
);

CREATE TABLE Track (
    Track_ID INTEGER PRIMARY KEY,
    Track TEXT,
    Track_length TEXT,
    Album_ID INTEGER,
    FOREIGN KEY (Album_ID) REFERENCES Album(Album_ID)
);

CREATE TABLE Author_Genre (
    Author_ID INTEGER,
    Genre_ID INTEGER,
    PRIMARY KEY (Author_ID, Genre_ID),
    FOREIGN KEY (Author_ID) REFERENCES Author(Author_ID),
    FOREIGN KEY (Genre_ID)  REFERENCES Genre(Genre_ID)
);

CREATE TABLE Album_Author (
    Album_ID INTEGER,
    Author_ID INTEGER,
    PRIMARY KEY (Album_ID, Author_ID),
    FOREIGN KEY (Album_ID)  REFERENCES Album(Album_ID),
    FOREIGN KEY (Author_ID) REFERENCES Author(Author_ID)
);

CREATE TABLE Playlist_Track (
    Playlist_ID INTEGER,
    Track_ID INTEGER,
    PRIMARY KEY (Playlist_ID, Track_ID),
    FOREIGN KEY (Playlist_ID) REFERENCES Playlist(Playlist_ID),
    FOREIGN KEY (Track_ID)    REFERENCES Track(Track_ID)
);