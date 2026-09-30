SELECT al.Album
FROM Album al
JOIN Album_Author aa ON aa.Album_ID = al.Album_ID
JOIN Author_Genre ag ON ag.Author_ID = aa.Author_ID
GROUP BY al.Album_ID, al.Album
HAVING COUNT(DISTINCT ag.Genre_ID) > 1;

SELECT t.Track
FROM Track t
WHERE NOT EXISTS (
  SELECT 1
  FROM Playlist_Track pt
  WHERE pt.Track_ID = t.Track_ID
);

WITH TrackDur AS (
  SELECT Track_ID,
         Album_ID,
         CASE
           WHEN instr(Track_length, ':') > 0 THEN
             CAST(substr(Track_length, 1, instr(Track_length, ':') - 1) AS INTEGER) * 60
             + CAST(substr(Track_length, instr(Track_length, ':') + 1) AS INTEGER)
           ELSE CAST(Track_length AS INTEGER)
         END AS duration_seconds
  FROM Track
)
SELECT DISTINCT a.Author
FROM Author a
JOIN Album_Author aa ON aa.Author_ID = a.Author_ID
JOIN TrackDur t ON t.Album_ID = aa.Album_ID
WHERE t.duration_seconds = (SELECT MIN(duration_seconds) FROM TrackDur);

SELECT al.Album
FROM Album al
LEFT JOIN Track t ON t.Album_ID = al.Album_ID
GROUP BY al.Album_ID, al.Album
HAVING COUNT(t.Track_ID) = (
  SELECT MIN(cnt)
  FROM (
    SELECT COUNT(t2.Track_ID) AS cnt
    FROM Album al2
    LEFT JOIN Track t2 ON t2.Album_ID = al2.Album_ID
    GROUP BY al2.Album_ID
  )
);