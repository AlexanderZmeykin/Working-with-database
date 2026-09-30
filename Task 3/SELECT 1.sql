SELECT Track, Track_length
FROM Track
ORDER BY
  CASE
    WHEN instr(Track_length, ':') > 0 THEN
      CAST(substr(Track_length, 1, instr(Track_length, ':') - 1) AS INTEGER) * 60
      + CAST(substr(Track_length, instr(Track_length, ':') + 1) AS INTEGER)
    ELSE CAST(Track_length AS INTEGER)
  END DESC
LIMIT 1;

SELECT Track
FROM Track
WHERE
  CASE
    WHEN instr(Track_length, ':') > 0 THEN
      CAST(substr(Track_length, 1, instr(Track_length, ':') - 1) AS INTEGER) * 60
      + CAST(substr(Track_length, instr(Track_length, ':') + 1) AS INTEGER)
    ELSE CAST(Track_length AS INTEGER)
  END >= 210;

SELECT Playlist
FROM Playlist
WHERE strftime('%Y', Playlist_Release) IN ('2018', '2019', '2020');

Исполнители, чьё имя состоит из одного слова
SELECT Author
FROM Author
WHERE instr(trim(Author), ' ') = 0;

SELECT Track
FROM Track
WHERE Track LIKE '%мой%'
   OR Track LIKE '%Мой%'
   OR Track LIKE '%my%'
	OR Track LIKE '%My%';