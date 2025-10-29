#pragma once

// TYPES

struct cfg_format
{
	std::string cid;
	std::string art_out_dir;
	std::string track_out_dir;
	bool get_track_audio = true;
	bool get_track_art = false;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(cfg_format, cid, art_out_dir, track_out_dir, get_track_audio, get_track_art)

struct track_data
{
	// Output data

	std::string audio_file_name; // Name for downloaded MP3(s). Derived from track title by default.
	std::string image_file_name; // Name for downloaded cover art. Derived from track title by default.

	// MP3 ID3v2 data

	std::string title;
	std::string comments;
	std::string contrib_artists; // contributing artists
	std::string album_artists;   // album artist
	std::string album;
	std::string genre;
	int         number = -1; // track number (for albums)
	int         year   = -1;
};

// CONFIG DATA

namespace cfg
{
	//
	//// TYPES
	//

	struct cfg_flags_t
	{
		UINT add_to_path            : 1; // Adds program to PATH variables
		UINT save_config            : 1; // Saves applicable arguments to config
		UINT download_art_seperate  : 1; // [saveable] downloads track's cover art to its own file
		UINT disable_art_download   : 1; // [saveable] [default] disables the DOWNLOAD_ART_SEPERATE flag if it was saved to config
		UINT download_audio         : 1; // [saveable] [default] reenables audio downloads if disabled by DISABLE_AUDIO_DOWNLOAD in config
		UINT disable_audio_download : 1; // [saveable] disables audio downloads
		UINT no_link_provided       : 1; // Indicates that no link was provided - used when modifying config without downloading anything
		UINT cover_src_is_path      : 1; // Provided source for MP3 cover art is an image path on the computer
		UINT cover_src_is_sc_link   : 1; // Provided source for MP3 cover art is a SoundCloud link (uses cover art from song/profile)
		UINT no_arg_data_provided   : 1; // No variable was provided following an argument where its expected (e.g. '-title' used without providing title)
		UINT invalid_data_provided  : 1; // A non-numeric variable was provided following an argument where its expected (e.g. '-year' used without a numeric value provided)
		UINT config_just_created    : 1; // cfg.json was just created
	};

	//
	//// GLOBAL CONFIG VARIABLES
	//

	inline cfg_flags_t f = {}; // Flags

	inline std::string  client_id;     // SoundCloud client ID
	inline std::string  audio_out_dir; // MP3 download directory
	inline std::string  image_out_dir; // Cover art download directory
	inline std::string  image_src;     // MP3 cover art source (file path, soundcloud track link, or image link)
	inline std::wstring program_dir;   // SoundCloud.exe directory (used to open config and add to PATH variables)

	inline track_data g_track_data = {}; // Track data specified in arguments, mostly for singular MP3 downloads

	//
	//// FORWARD DECLARATIONS
	//

	bool parse_arguments(int argc, char* argv[]);

	void read_config(const cfg_format& data);

	void save_config(cfg_format& data);

	void add_to_path();

	//
	//// HELPERS
	//

	__forceinline bool audio_flags_set() noexcept
	{
		return cfg::f.download_audio || cfg::f.disable_audio_download;
	}

	__forceinline bool art_flags_set() noexcept
	{
		return cfg::f.download_art_seperate || cfg::f.disable_art_download;
	}

	__forceinline bool cover_art_only() noexcept
	{
		return cfg::f.download_art_seperate && cfg::f.disable_audio_download;
	}
}