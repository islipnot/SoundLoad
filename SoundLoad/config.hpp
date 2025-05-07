#pragma once

/* VALID CFG FIELDS
*
* - cid <client ID>
* - out <MP3 output dir>
* - img <default cover dir>
*
*/

/* ARGUMENTS
*
* ADDED:
*
* -CID     <client_id>
* -fName   <mp3 file name>
* -title   <mp3 title property>
* -artists <contributing artists property> (SURROUND IN QUOTES IF MULTIPLE ARTISTS)
* -artist  <album artist property>
* -album   <album property>
* -genre   <genre property>
* -year    <year property>
* -num     <# property>
* -out     <final mp3 output dir> (defaults to command directory at runtime)
* -cover   <cover path>
* -save    (saves args to cfg)
*
* Above fields (besides client ID) are scraped from track page or left empty if not provided by the user
*/

class Cfg
{
	enum CfgFlags
	{
		HasCID = 0x01,
		HasOut = 0x02,
		HasImg = 0x04
	};

	int flags = 0;

	void ReadCfg(std::ifstream cfg);
	void SaveCfg(std::ofstream cfg);

public:

	enum StatusFlags
	{
		Error  = 0x01,
		NoLink = 0x02, // No link provided, this should only be the case for saving your config 
		NoSong = 0x04  // Only the cover is to be downloaded, not the song
	};

	Cfg(int argc, char* argv[]);

	// Description

	std::string title;    // Title
	std::string comments; // Comments

	// Media

	std::string artists; // Contributing artists
	std::string artist;  // Album artist
	std::string album;   // Album
	int year = -1;       // Year
	int num  = -1;       // # (Track Number)
	std::string genre;   // Genre

	// Extra

	std::string fName;  // File name
	std::string CID;    // SoundCloud client ID
	std::string output; // MP3 output directory
	std::string cover;  // Path for MP3 cover

	int status = 0;
};