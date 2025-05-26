#pragma once
#include "config.hpp"

using Json = nlohmann::json;

typedef class Track
{
	enum PostType
	{
		track,
		album,
		playlist
	};

	bool DownloadTrack();
	bool DownloadAlbum(); // Also works for playlists
	bool DownloadCover();
	bool GetStreamingUrl(const Json& json);
	bool GetTrackListIDs(const Json& json);
	void HandleMetadata(std::string& TrackPath);

	int type = track;
	Cfg* cfg = nullptr;

public:

	Track(std::string link, Cfg* pCfg);

	bool download()
	{
		if (type == track) return DownloadTrack();
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

	std::string UrlData; // If type == track, this is a progressive streaming URL. Otherwise, it's a track list ID resolution link.
	bool fail = false;

} Playlist, Album, PostData;