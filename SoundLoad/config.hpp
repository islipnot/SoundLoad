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
* --client  <client_id>
* --fName   <mp3 file name>
* --title   <mp3 title property>
* --cArtist <contributing artists property> (SURROUND IN QUOTES IF MULTIPLE ARTISTS)
* --artist  <album artist property>
* --album   <album property>
* --output  <final mp3 output dir> (defaults to command directory at runtime)
* --cover   <cover path>
* --save    (saves args to cfg)
*
* Above fields (besides client ID) are scraped from track page or left empty if not provided by the user
*/

class Cfg
{
	enum CfgFlags
	{
		HasCID = 0x01,
		HasOut = 0x02,
		HasImg = 0x04, // for future use
	};

	int flags = 0;

	void ReadCfg(std::ifstream cfg);
	void SaveCfg(std::ofstream cfg);

public:

	Cfg(int argc, char* argv[]);

	std::string CID;      // SoundCloud client ID
	std::string fName;     // File name
	std::string title;    // Title
	std::string cArtists; // Contributing artists
	std::string artist;   // Album artist
	std::string album;    // Album

	std::string output; // MP3 output directory
	std::string cover;  // Either the covers path or its link

	bool CoverLink = true; // If true, cover is a link
	bool error = false;
};