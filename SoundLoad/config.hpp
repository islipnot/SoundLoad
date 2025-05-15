#pragma once

struct JsonCfg
{
	std::string cid; // SoundCloud client ID
	std::string out; // MP3 output directory
	std::string img; // Path for MP3 cover (to get from or store)
	bool ran = true; // Whether the program has been ran or not
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(JsonCfg, cid, out, img, ran)

class Cfg
{
	enum PrivFlags
	{
		HasCID     = 0x01,
		HasOut     = 0x02,
		HasImg     = 0x04,
		WasRan     = 0x08,
		a_cid      = 0x10,
		a_fname    = 0x20,
		a_title    = 0x40,
		a_album    = 0x80,
		a_artists  = 0x100,
		a_artist   = 0x200,
		a_genre    = 0x400,
		a_out      = 0x800,
		a_cover    = 0x1000,
		a_save     = 0x2000,
		a_year     = 0x4000,
		a_num      = 0x8000,
		a_envvar   = 0x10000,
		AddEnvVar  = 0x10000
	};

	void SetPathVar();
	void ReadCfg(std::ifstream cfg);
	void SaveCfg(const char* path);
	void ReadArgs(int argc, char* argv[]);

	int flags = 0;

public:

	enum StatusFlags
	{
		Error  = 0x01,
		NoLink = 0x02, // No link provided, this should only be the case for saving your config 
		NoSong = 0x04, // Only the cover is to be downloaded, not the song
		Save   = 0x08
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
	std::string cover;  // Path for MP3 cover (to get from or store)

	int status = 0;
};