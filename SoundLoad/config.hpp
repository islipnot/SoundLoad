#pragma once

/* VALID CFG FIELDS
*
* - cid <client ID>
* - out <MP3 output dir>
* - img <default cover dir>
*
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

	bool OpenCfg(std::fstream&); // returns false if file failed to open
	void ReadCfg(std::fstream&);
	void SaveCfg(std::fstream&);

public:

	Cfg(int argc, char* argv[]);

	std::string CID;      // SoundCloud client ID
	std::string fName;    // File name
	std::string title;    // Title
	std::string cArtists; // Contributing artists
	std::string artist;   // Album artist
	std::string album;    // Album

	std::string output; // MP3 output directory
	std::string cover;  // Either the covers path or its link

	bool CoverLink = true; // If true, cover is a link
	bool error = false;
};