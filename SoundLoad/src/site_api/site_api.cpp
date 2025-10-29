#include "pch.hpp"
#include "../config/config.hpp"
#include "site_api.hpp"

//
//// HELPER FUNCTIONS
//

static void mb_to_wide(const std::string& src, std::wstring& dst)
{
	if (!src.empty())
	{
		dst.resize(src.size());
		MultiByteToWideChar(CP_UTF8, 0, src.c_str(), -1, dst.data(), static_cast<int>(dst.size()));
	}
}

static bool download_m3u(const std::string& raw_data, std::string& mp3_buffer)
{
	// M3U's are just a list of links which lead to segments of an MP3 file.
	// HLS streaming links use these, though almost all tracks I've seen use progressive (one download).

	std::istringstream iss(raw_data);
	std::string line;

	while (std::getline(iss, line))
	{
		if (line.starts_with("#EXTINF"))
		{
			std::getline(iss, line); // line following "#EXTINF" contains next MP3 segment

			const cpr::Response r = cpr::Get(cpr::Url{ line });
			if (request_failed(r))
			{
				err::log_net(r);
				return false;
			}

			mp3_buffer += r.text;
		}
	}

	if (mp3_buffer.empty())
	{
		err::log("failed to parse M3U file");
		return false;
	}

	return true;
}

static bool resolve_post(std::string& url, Json& buffer)
{
	// Erasing tracking data from link

	if (url.find('?') != std::string::npos)
	{
		url.erase(url.find_first_of('?'));
	}

	// Resolving post

	const cpr::Response r = cpr::Get(cpr::Url{ "https://api-v2.soundcloud.com/resolve?url=" + url + "&client_id=" + cfg::client_id });
	if (request_failed(r))
	{
		err::log_net(r);
		return false;
	}

	buffer = Json::parse(r.text);
	return true;
}

//
//// MEMBER FUNCTIONS
//

void sc_upload::add_tag(const std::wstring& path)
{
	// Creating ID3v2 tag

	TagLib::MPEG::File file(path.c_str());
	TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);

	// Setting metadata

	tag->setAlbum(this->album);

	tag->setTitle(this->title);

	tag->setArtist(this->artist);

	tag->setGenre(this->genre);

	tag->setComment(this->description);
	
	tag->setYear(static_cast<UINT>(this->year));

	// Setting cover art

	auto cover = new TagLib::ID3v2::AttachedPictureFrame;

	if (cfg::f.cover_src_is_path)
	{
		std::ifstream cover_file(cfg::image_src, std::ios::binary | std::ios::ate);
		if (cover_file.fail())
		{
			err::log("failed to open cover art file \"{}\"", cfg::image_src);
			file.save();
			delete cover;
			return;
		}

		const size_t sz = cover_file.tellg();
		char* raw_image = new char[sz];

		cover_file.seekg(0, std::ios::beg);
		cover_file.read(raw_image, sz);
		cover_file.close();

		cover->setPicture({ raw_image, static_cast<UINT>(sz) });
		delete[] raw_image;
	}
	else
	{
		// For any of the remaining cases, the goal is to set this->art_src to the cover art link.
		// This will already be the case if an image link is provided or if no source was specified 
		// by the user. If a SoundCloud post was specified as the art source, we will have to resolve 
		// the artwork_url from it.

		if (cfg::f.cover_src_is_sc_link) // User specified a different SoundCloud post to use the cover art from
		{
			Json post_data;
			if (!resolve_post(cfg::image_src, post_data))
			{
				file.save();
				delete cover;
				return;
			}

			this->art_src = post_data.value("artwork_url");
			if (this->art_src.empty())
			{
				file.save();
				delete cover;
				return;
			}

			this->art_src = std::regex_replace(this->art_src, std::regex("-large."), "-original.");
		}
		
		// Downloading and applying cover art

		const cpr::Response r = cpr::Get(cpr::Url{ this->art_src });
		if (request_failed(r))
		{
			err::log_net(r);
			file.save();
			delete cover;
			return;
		}

		cover->setPicture({ r.text.data(), static_cast<UINT>(r.text.size()) });
	}
	
	tag->addFrame(cover);
	file.save();
}

bool sc_upload::get_track_ids(const Json& data)
{
	const auto tracks = data.value("tracks", Json{});
	if (tracks.empty())
	{
		err::log("failed to parse track ID's");
		return false;
	}

	for (size_t i = tracks.size() - 1; i; --i)
	{
		track_ids.push_back(tracks[i].value("id", 0));
	}

	return true;
}

bool sc_upload::get_streaming_url(const Json& json)
{
	auto list = json.value("media", Json{});
	if (list.empty())
	{
		err::log("'media' property not found in metadata");
		return false;
	}

	list = list.value("transcodings", Json{});
	if (list.empty())
	{
		err::log("'transcodings' property not found in metadata");
		return false;
	}

	bool found = false;
	bool hls = false;

	for (const auto& transcoding : list)
	{
		const auto format = transcoding["format"];
		hls = format["protocol"] == "hls";

		if ((format["protocol"] == "progressive") || (hls && format["mime_type"] == "audio/mpeg"))
		{
			this->streaming_url = transcoding["url"].get<std::string>() + "?client_id=" + cfg::client_id;
			found = true;

			if (!hls) break;
		}
	}

	if (!found)
	{
		err::log("failed to locate valid media transcoding");
		return false;
	}

	if (hls)
	{
		this->f.is_hls_media = true;
	}

	return true;
}

bool sc_upload::download_track()
{
	// Downloading track (MP3 or HLS)

	cpr::Response r = cpr::Get(cpr::Url{ this->streaming_url });
	if (request_failed(r))
	{
		err::log_net(r);
		return false;
	}

	r = cpr::Get(cpr::Url{ Json::parse(r.text)["url"].get<std::string>()});
	if (request_failed(r))
	{
		err::log_net(r);
		return false;
	}

	this->streaming_url.clear();
	
	// Finalizing MP3 download

	const char* raw_mp3 = nullptr;
	size_t mp3_size = 0;

	// This must be outside of the if-else scope below or else raw_mp3 will hold a dangling pointer for HLS downloads.
	// If one std::string buffer was used for HLS and progressive it would require copying the entire mp3 across memory.
	std::string raw_hls_result;

	if (this->f.is_hls_media)
	{
		if (!download_m3u(r.text, raw_hls_result))
			return false;

		raw_mp3  = raw_hls_result.data();
		mp3_size = raw_hls_result.size();
	}
	else
	{
		raw_mp3  = r.text.data();
		mp3_size = r.text.size();
	}

	// Getting MP3 output path

	std::wstring path;
	mb_to_wide(cfg::audio_out_dir, path);
	path += std::regex_replace(this->title, std::wregex(L"[<>:\"/\\|?*]"), L"_") + L".mp3";

	// Writing MP3 to disk

	std::ofstream mp3_file(path, std::ios::binary | std::ios::trunc);
	if (mp3_file.fail())
	{
		err::log(L"failed to create mp3 file at \"{}\"", path);
		return false;
	}

	mp3_file.write(raw_mp3, mp3_size);
	mp3_file.close();

	// Adding ID3v2 tag

	this->add_tag(path);

	return true;
}

bool sc_upload::download_album()
{
	return true;
}

bool sc_upload::download_cover() const
{
	const cpr::Response r = cpr::Get(cpr::Url{ this->artwork_url });
	if (request_failed(r))
	{
		err::log_net(r);
		return false;
	}

	std::wstring path;
	mb_to_wide(cfg::image_out_dir, path);
	path += std::regex_replace(this->title, std::wregex(L"[<>:\"/\\|?*]"), L"_") + L".jpg";

	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	if (file.fail())
	{
		err::log(L"failed to open/create \"{}\"", path);
		return false;
	}

	file.write(r.text.data(), r.text.size());
	file.close();

	return true;
}

sc_upload::sc_upload(std::string url)
{
	// Resolving post

	Json post_data;
	if (!resolve_post(url, post_data))
	{
		this->f.error_occured = true;
		return;
	}

	// Getting artwork URL & title
	
	{
		mb_to_wide(cfg::g_track_data.title.empty() ? post_data.value("title") : cfg::g_track_data.title, this->title);
		this->title.resize(lstrlenW(this->title.c_str()));

		this->artwork_url = post_data.value("artwork_url", post_data.value("avatar_url"));
		if (!this->artwork_url.empty())
		{
			this->artwork_url = std::regex_replace(this->artwork_url, std::regex("-large."), "-original.");
		}

		this->art_src = cfg::image_src.empty() ? this->artwork_url : cfg::image_src;

		if (cfg::cover_art_only())
		{
			return;
		}
	}

	// Getting album (order: cfg::g_track_data.album -> first existing album via SoundCloud API -> title)

	{
		if (cfg::g_track_data.album.empty())
		{
			const cpr::Response r = cpr::Get(cpr::Url{ "https://api-v2.soundcloud.com/tracks/" + std::to_string(post_data["id"].get<int>()) + "/albums?client_id=" + cfg::client_id });

			if (r.status_code == 200)
			{
				const Json album_data = Json::parse(r.text)["collection"];

				if (!album_data.empty())
				{
					mb_to_wide(album_data[0].value("title"), this->album);
				}
			}

			if (this->album.empty())
			{
				this->album = this->title;
			}
		}
		else
		{
			mb_to_wide(cfg::g_track_data.album, this->album);
		}
	}

	// Creating comments

	{
		// Specified comments

		mb_to_wide(cfg::g_track_data.comments, this->description);

		// Extra comments

		std::wstring temp;

		auto add_comment = [this, &post_data, &temp](PCWSTR label, PCSTR value)
			{
				mb_to_wide(post_data.value(value), temp);

				if (!temp.empty())
				{
					if (!this->description.empty())
					{
						this->description += L"\n\n";
					}

					this->description += label + temp;

					temp.clear();
				}
			};

		add_comment(L"Upload date: ",          "created_at");
		add_comment(L"Original description: ", "description");
		add_comment(L"Original tags: ",        "tag_list");
	}
	
	// Getting genre and year

	{
		mb_to_wide(cfg::g_track_data.genre.empty() ? post_data.value("genre") : cfg::g_track_data.genre, this->genre);

		this->year = std::stoi(post_data["created_at"].get<std::string>());
	}

	// Getting artist

	{
		if (!cfg::g_track_data.contrib_artists.empty())
		{
			mb_to_wide(cfg::g_track_data.contrib_artists, this->artist);
		}
		else
		{
			std::string mb_artist;

			if ((mb_artist = post_data.value("publisher_metadata", Json{}).value("artist")).empty() 
			&&  (mb_artist = post_data.value("user",               Json{}).value("username")).empty())
			{
				err::log("failed to get publisher_metadata.artist or user.username");
				this->f.error_occured = true;
				return;
			}

			mb_to_wide(mb_artist, this->artist);
		}
	}
	
	// Getting upload type, track ID(s), and streaming url (if applicable)

	{
		const std::string kind = post_data.value("kind");
		if (kind.empty())
		{
			err::log("failed to get 'kind' field from metadata");
			this->f.error_occured = true;
			return;
		}

		if (kind[0] == 't')
		{
			this->id = post_data.value("id", 0);
			this->f.is_track = true;

			if (!get_streaming_url(post_data))
			{
				this->f.error_occured = true;
			}
		}
		else
		{
			this->f.is_album = true;

			if (!get_track_ids(post_data))
			{
				this->f.error_occured = true;
			}
		}
	}
}