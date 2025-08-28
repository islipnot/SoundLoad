#pragma once

#include "cfg.hpp"

#define FetchErr(response) std::cerr << "REQUEST FAILED: " << response.url << " (" << response.error.message << " )\n" // Expects cpr::Response

#define RequestFail(response) response.status_code != 200 // Expects cpr::Response

#define ToUtf8(str) TagLib::String(str.c_str(), TagLib::String::UTF8)

using Json = nlohmann::json;

typedef class Track
{
	bool hls = false;
	int type = -1;

	std::vector<int> ids; // for albums/playlists with over 50 songs (the limit per request)

	bool DownloadTrack() const;
	bool DownloadAlbum(); // Works for playlists
	bool DownloadCover() const;

	bool GetTrackIDs(const Json& json);
	bool GetStreamingUrl(const Json& json);

	void AddTag(const std::string& path) const;
	void HandleTagArt(TagLib::ID3v2::Tag* tag) const;

public:

	enum TrackFlags
	{
		Error = 1,
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

	Track(std::string url, bool CoverOnly = false);

	inline bool download()
	{
		if (cfg::flags & cfg::GetArtIndependent && !DownloadCover()) return false;

		if (!(cfg::flags & cfg::DontGetAudio))
		{
			if (type == tTrack) return DownloadTrack();
			return DownloadAlbum();
		}
	}

	inline bool fail() const { return flags & Error; }

} Album, ScPost;