#pragma once

// JSON CONFIG FORMAT

struct CfgFormat
{
	std::string cid;
	std::string ArtDir;
	std::string TrackDir;
	char GetArt   = -1;
	char GetTrack = -1;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CfgFormat, cid, ArtDir, TrackDir, GetArt, GetTrack)

// CONFIG SYSTEM

namespace cfg
{
	// ENUMS

	enum CfgFlags : WORD
	{
		AddToPathVariables = 1,      // Adds SoundLoad to PATH variables
		SaveToConfig       = 1 << 1, // Saves applicable arguments to cfg.json
		GetArtIndependent  = 1 << 2, // Independently downloads cover art (saveable)
		DontGetArt         = 1 << 3, // Disables GetArtIndependent if it was saved to cfg.json (saveable, enabled by default)
		DontGetAudio       = 1 << 4, // MP3 will not be downloaded (saveable)
		GetAudio           = 1 << 5, // MP3 will be downloaded (saveable, enabled by default)
		NoLinkProvided     = 1 << 6, // Used when config modifications are the only operation
		ArtSrcPath         = 1 << 7, // CoverSrc is a path
		ArtSrcScLink       = 1 << 8, // CoverSrc is a SoundCloud link
		ArtSrcImgLink      = 1 << 9, // CoverSrc is a direct image link
	};

	// VARIABLES

	inline WORD flags = 0;

	inline std::string TrackName;  // MP3 file name
	inline std::string ArtName;    // JPG file name (if independent art download is enabled)
	inline std::string TrackTitle; // MP3 ID3v2 title property (defaults to TrackName)
	inline std::string comment;    // MP3 ID3v2 comments property
	inline std::string cArtist;    // MP3 ID3v2 contributing artists property
	inline std::string aArtist;    // MP3 ID3v2 album artist property
	inline std::string album;      // MP3 ID3v2 album property
	inline std::string genre;      // MP3 ID3v2 genre property
	inline int TrackNumber;        // MP3 ID3v2 # property
	inline int year;               // MP3 ID3v2 year property

	inline std::string cid;      // SoundCloud client ID
	inline std::string TrackDir; // MP3 output directory
	inline std::string ArtDir;   // JPG output directory
	inline std::string CoverSrc; // MP3 ID3v2 cover art source (can be image link, soundcloud track link, or path on computer)
	inline std::wstring ExeDir;  // SoundLoad.exe path

	// FORWARD DECLARATIONS

	void AddToPath();

	void SaveConfig(CfgFormat& data, const std::wstring& path);

	void ReadConfig(const CfgFormat& data);

	bool GetConfig(int argc, char* argv[]);
}