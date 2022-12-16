ALTER TABLE Playlist ADD DisplayOrder INTEGER;
UPDATE Playlist SET DisplayOrder = Id;
ALTER TABLE Scares ADD DisplayOrder INTEGER;
UPDATE Scares SET DisplayOrder = Id;
ALTER TABLE Extras ADD DisplayOrder INTEGER;
UPDATE Extras SET DisplayOrder = Id;
