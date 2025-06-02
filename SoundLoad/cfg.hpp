#pragma once

/* VALID ARGUMENTS (case-insensitive)
*
* [*] GENERAL
*
* -cid  > SoundCloud client ID (saveable)
* -PVar > Adds program to user PATH variables
*
* [*] CONFIG MANAGEMENT
*
* -save  > Saves applicable args to cfg.json
* -fresh > Does not read from cfg.json (besides CID)
* -clear > Restores default state for cfg.json
*
* [*] ART MANAGEMENT
*
* -cFile > Cover art file name
* -cDst  > Cover art output directory (saveable)
* -cSrc  > Cover art source (path, sc link, img link)
* -art   > Downloads cover art seperately from MP3 (saveable)
* --art  > Opposite of "-art" (saveable)
*
* [*] MP3 MANAGEMENT
*
* -aFile  > MP3 file name
* -aDst   > MP3 output directory (saveable)
* --audio > Audio will not be downloaded (saveable)
* -audio  > Opposite of "--audio" (saveable)
*
* [*] MP3 ID3v2 TAG PROPERTIES
*
* -title
* -comment
* -cArtist > "contributing artists"
* -aArtist > "album artist"
* -album
* -genre
* -tNum
* -year
*/

using Json = nlohmann::json;

struct CfgFormat
{
	std::string cid;
	std::string ArtDst;
	std::string TrackDst;
	int GetArt   = -1;
	int GetTrack = -1;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CfgFormat, cid, ArtDst, TrackDst, GetArt, GetTrack)

struct Config
{
	// Enums

	enum CfgFlags
	{
		Error    = 1,       // Error occured within a member function
		SkipCfg  = 1 << 1,  // Does not read from cfg.json (besides CID)
		SaveCfg  = 1 << 2,  // Save applicable arguments to cfg.json (-save)
		ClearCfg = 1 << 3,  // Set all data in cfg.json to the default values (-clear)
		GetCover = 1 << 4,  // Downloads cover art seperate from MP3 metadata (-art)
		NoCover  = 1 << 5,  // Reverses the effect of GetCover when saved to cfg (--art)
		NoAudio  = 1 << 6,  // Does not download audio from the track link (--audio)
		GetAudio = 1 << 7,  // Reverses the effect of NoAudio when saved to cfg (-audio)
		AddPVar  = 1 << 8,  // Adds program to user PATH variables
		tPath    = 1 << 9,  // CoverSrc is an image path
		tScLink  = 1 << 10, // CoverSrc is a SoundCloud link
		tImgLink = 1 << 11, // CoverSrc is an image link

		NoLink = NoAudio | NoCover
	};

	// File info

	std::string TrackName;
	std::string CoverName; // Defaults to TrackName
	std::string title;     // Defaults to TrackName
	std::string comments;
	std::string cArtists; // Contributing artists
	std::string aArtist;  // Album artist
	std::string album;    // Defaults to TrackName
	std::string genre;
	int tNum = -1;
	int year = -1;

	// Data

	std::string cid;
	std::string TrackDst; // MP3 output path
	std::string CoverDst; // Cover art output path
	std::string CoverSrc; // Cover art source
	std::wstring ExeDir;  // Directory of SoundLoad.exe

	int flags = 0;

	// Methods

	Config(int argc, char* argv[]);

	void AddPathVar();

	bool save(CfgFormat& cfg, const std::wstring& path);

	void read(Json& JsonCfg, CfgFormat& cfg, bool CidOnly);

	inline bool fail() { return flags & Error; }
};

/* STRUCT INFO
*
* CoverSrc can be a link to a SoundCloud track (in which case it'll
* use the cover art of the track for the MP3), a regular image link,
* or a path to an image you already have downloaded.
*/