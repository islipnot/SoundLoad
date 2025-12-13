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
		USHORT is_m4a_media  : 1; // Lossless media transcoding (.m3u8 -> .m4a)
		USHORT is_hls_mpeg   : 1; // HLS media transcoding (.m3u -> .mp3)
	};

	//
	//// PRIVATE MEMBER VARIABLES
	//

	std::string      streaming_url;
	std::vector<int> track_ids;

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
	//// PRIVATE METHODS
	//

	bool download_track();

	bool download_album();

	bool download_cover() const;

	bool parse_manifest(const std::string& raw_data, std::string& buffer) const;

	bool get_streaming_url(const Json& data);

	bool get_track_ids(const Json& data);

	bool get_cover_link(std::string& buffer) const;

	void add_m4a_tag(const std::wstring& path) const;

	void add_mp3_tag(const std::wstring& path) const;

public:

	//
	//// PUBLIC MEMBER VARIABLES
	//

	upload_flags_t f = {};

	//
	//// PUBLIC MEMBER FUNCTIONS
	//

	sc_upload(std::wstring url);

	//
	//// HELPERS
	//

	bool download()
	{
		if (cfg::f.download_art_seperate)
		{
			if (!this->download_cover())
			{
				return false;
			}

			if (cfg::f.disable_audio_download)
			{
				std::cout << "WARNING: only downloaded cover art\n";
			}
		}

		if (cfg::f.disable_audio_download)
		{
			return true;
		}

		return this->f.is_track ? this->download_track() : this->download_album();
	}
};