#pragma once
#include "config.hpp"

#define ToUtf8(str) TagLib::String(str.c_str(), TagLib::String::UTF8)

using Json = nlohmann::json;

typedef class Track
{
	enum PostType
	{
		tTrack,
		tAlbum,
		tPlaylist
	};

	bool DownloadTrack();
	bool DownloadAlbum(); // Also works for playlists
	bool DownloadCover();

	bool GetStreamingUrl(const Json& json);
	bool GetTrackListIDs(const Json& json);

	void HandleMetadata(std::string& TrackPath);

	int type = tTrack;
	Cfg* cfg = nullptr;

public:

	enum Flags
	{
		Error = 0x01,
		Hls   = 0x02
	};

	Track(std::string link, Cfg* pCfg);

	bool download()
	{
		if (type == tTrack) return DownloadTrack();
		else if (cfg->status & Cfg::NoLink) return DownloadCover();
		else return DownloadAlbum();
	}

	// SoundCloud post metadata

	std::string CoverUrl;    // URL to the track cover (artwork_url)
	std::string CreatedAt;   // Track upload timestamp (created_at)
	std::string description; // Track description
	std::string genre;       // Track genre
	int id = 0;              // Track ID - if 0, the object represents a playlist/album
	std::string artist;      // Track artist (publisher_metadata.artist)
	std::string tags;        // Track tags (tag_list)
	std::string title;       // Track title
	std::string ArtistPfp;   // Profile picture of the account that uploaded the track (used if no cover was added to track)

	// Extra

	std::string UrlData; // If type == track, this is a progressive/hls streaming URL. Otherwise, it's a track list ID resolution link.
	int flags = 0;

} Playlist, Album, PostData;

/* SoundCloud and M3U's
*
* For every SoundCloud track, there are various transcodings,
* most of them being HLS, with just one being progressive usually.
* Progressive transcodings provide a complete MP3 that can be
* downloaded with a single request, while HLS transcodings are
* split into segments typically ranging from ~1.2-10 seconds.
* Each of these segments is a complete MP3 file, and while
* media players can simply play them sequentially, we must
* download and concatenate them to create a single MP3 file.
*/