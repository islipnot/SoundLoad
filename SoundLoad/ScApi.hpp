#pragma once
#include "cfg.hpp"

#define FetchErr(response) std::cerr << "REQUEST FAILED: " << response.url << " (" << response.error.message << " )\n" // Expects cpr::Response

#define RequestFail(response) response.status_code != 200 // Expects cpr::Response

#define ToUtf8(str) TagLib::String(str.c_str(), TagLib::String::UTF8)

typedef class Track
{
	Config* cfg = nullptr;
	bool hls = false;
	int type = -1;

	std::vector<int> ids; // for albums/playlists with over 50 songs (the limit per request)

	bool DownloadTrack();
	bool DownloadAlbum(); // Works for playlists
	bool DownloadCover();

	bool GetTrackIDs(const Json& json);
	bool GetStreamingUrl(const Json& json);

	void AddTag(const std::string& path);
	void HandleTagArt(TagLib::ID3v2::Tag* tag);

public:

	enum TrackFlags
	{
		Error  = 1,
		tTrack = 1 << 1, // Object represents a song
		tAlbum = 1 << 2, // Object represents album/playlist
	};

	// Metadata

	std::string CoverUrl;  // artwork_url
	std::string CreatedAt; // created_at
	std::string description;
	std::string genre;
	int id = 0;
	std::string artist; // publisher_metadata.artist
	std::string tags;   // tag_list
	std::string title;
	std::string ArtistPfpUrl;

	// Info

	std::string UrlData; // If type == track, progressive/hls streaming URL. Else, track list ID resolution link.
	int flags = 0;

	// Methods

	Track(std::string url, Config* cfg, bool CoverOnly = false);

	bool download()
	{
		if (cfg->flags & Config::GetCover && !DownloadCover()) return false;

		if (!(cfg->flags & Config::NoAudio))
		{
			if (type == tTrack) return DownloadTrack();
			else return DownloadAlbum();
		}
	}

	inline bool fail() { return flags & Error; }

} Album, ScPost;