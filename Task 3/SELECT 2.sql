SELECT g.Genre, COUNT(DISTINCT ag.Author_ID) AS Количество_исполнителей
FROM Genre g
LEFT JOIN Author_Genre ag ON ag.Genre_ID = g.Genre_ID
GROUP BY g.Genre_ID, g.Genre;

SELECT COUNT(*) AS Количество_треков
FROM Track t
JOIN Album a ON a.Album_ID = t.Album_ID
WHERE strftime('%Y', a.Albume_Release) IN ('2019', '2020');

SELECT a.Album,
       AVG(
         CASE
           WHEN instr(t.Track_length, ':') > 0 THEN
             CAST(substr(t.Track_length, 1, instr(t.Track_length, ':') - 1) AS INTEGER) * 60
             + CAST(substr(t.Track_length, instr(t.Track_length, ':') + 1) AS INTEGER)
           ELSE CAST(t.Track_length AS INTEGER)
         END
       ) AS Средняя_продолжительность_сек
FROM Album a
JOIN Track t ON t.Album_ID = a.Album_ID
GROUP BY a.Album_ID, a.Album;

SELECT a.Author
FROM Author a
WHERE NOT EXISTS (
  SELECT 1
  FROM Album_Author aa
  JOIN Album al ON al.Album_ID = aa.Album_ID
  WHERE aa.Author_ID = a.Author_ID
    AND strftime('%Y', al.Albume_Release) = '2020'
);

SELECT DISTINCT p.Playlist
FROM Playlist p
JOIN Playlist_Track pt ON pt.Playlist_ID = p.Playlist_ID
JOIN Track t ON t.Track_ID = pt.Track_ID
JOIN Album_Author aa ON aa.Album_ID = t.Album_ID
JOIN Author a ON a.Author_ID = aa.Author_ID
WHERE a.Author = 'Лампада';