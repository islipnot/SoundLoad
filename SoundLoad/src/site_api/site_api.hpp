#pragma once

#define request_failed(response) response.status_code != 200 // decltype(response) == cpr::Response

struct sc_upload
{
private:

	//
	//// PRIVATE TYPES
	//

	struct upload_flags_t
	{
		USHORT error_occured : 1; // An error occured in the constructor
		USHORT is_track      : 1; // Object represents a singular track
		USHORT is_album      : 1; // Object represents an album or playlist
		USHORT is_hls_media  : 1; // Streaming URL links an HLS transcoding (.m3u file)
	};

	//
	//// PRIVATE MEMBER VARIABLES
	//

	std::string      streaming_url;
	std::vector<int> track_ids;

	//
	//// PRIVATE METHODS
	//

	bool download_track();

	bool download_album();

	bool download_cover() const;

	bool get_streaming_url(const Json& data);

	bool get_track_ids(const Json& data);

	void add_tag(const std::wstring& path);

public:

	//
	//// PUBLIC MEMBER VARIABLES
	//

	upload_flags_t f = {};

	std::wstring art_src;
	std::string  artwork_url;
	std::string  artist_pfp_url;

	std::wstring description;
	std::wstring genre;
	std::wstring album;
	std::wstring artist;
	std::wstring title;
	UINT         year = 0u;
	UINT         num  = 0u;

	int id = 0;

	//
	//// PUBLIC MEMBER FUNCTIONS
	//

	sc_upload(std::wstring url);

	//
	//// HELPERS
	//

	bool download()
	{
		if (cfg::f.download_art_seperate && !this->download_cover())
			return false;

		if (cfg::f.disable_audio_download)
			return true;

		return this->f.is_track ? this->download_track() : this->download_album();
	}
};