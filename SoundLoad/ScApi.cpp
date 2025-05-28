#include "pch.hpp"
#include "ScApi.hpp"
#include "config.hpp"

using Json = nlohmann::json;

bool Playlist::GetTrackListIDs(const Json& json)
{
	constexpr auto tracks = "tracks";

	if (!json.contains(tracks))
	{
		std::cerr << "ERROR: tracks property not found\n";
		return false;
	}

	UrlData = "https://api-v2.soundcloud.com/tracks?ids=";

	for (const auto& track : json[tracks])
	{
		UrlData += std::to_string(track.value("id", 0)) + ',';
	}

	UrlData.pop_back(); // removing final comma
	UrlData += "&client_id=" + cfg->CID;

	return true;
}

bool Track::GetStreamingUrl(const Json& json)
{
	constexpr auto media = "media";
	constexpr auto transcodings = "transcodings";

	if (!json.contains(media))
	{
		std::cerr << "ERROR: media property not found\n";
		return false;
	}

	if (!json[media].contains(transcodings))
	{
		std::cerr << "ERROR: transcodings property not found\n";
		return false;
	}

	for (const auto& transcoding : json[media][transcodings])
	{
		const auto format = transcoding["format"];
		const bool IsHls = format["protocol"] == "hls";

		if ((format["protocol"] == "progressive") || (IsHls && format["mime_type"] == "audio/mpeg"))
		{
			UrlData = transcoding["url"].get<std::string>() + "?client_id=" + cfg->CID;

			if (IsHls)
			{
				flags |= Hls;
				DBG_MSG("> HLS URL: " + UrlData);
			}
			else { DBG_MSG("> PROGRESSIVE URL: " + UrlData); }

			return true;
		}
	}

	std::cerr << "ERROR: NO PROGRESSIVE/HLS-MPEG URL FOUND\n";
	return false;
}

Track::Track(std::string link, Cfg* pCfg)
{
	// Requesting track data

	if (link.find('?') != std::string::npos) // for when you use the copy link button rather than copying browser url
	{
		link.erase(link.find_first_of('?'));
	}

	const std::string url = "https://api-v2.soundcloud.com/resolve?url=" + link + "&client_id=" + pCfg->CID;
	const cpr::Response r = cpr::Get(cpr::Url{ url });

	std::cout << "> RESOLUTION URL: " << url << '\n';

	if (RequestFail(r))
	{
		FetchErr(r);
		flags |= Error;
		return;
	}

	// Setting member variable values

	const Json json = Json::parse(r.text);

	CoverUrl    = std::regex_replace(json.value("artwork_url", std::string{}), std::regex("-large."), "-original.");
	CreatedAt   = json.value("created_at",  std::string{});
	description = json.value("description", std::string{});
	genre       = json.value("genre",       std::string{});
	tags        = json.value("tag_list",    std::string{});
	title       = json.value("title",       std::string{});
	cfg         = pCfg;

	constexpr auto kind = "kind";
	constexpr auto publisher_metadata = "publisher_metadata";

	if (json.contains(kind))
	{
		const char PostType = json[kind].get<std::string>()[0];

		if (PostType == 't')
		{
			id = json.value("id", 0);
			type = tTrack;
		}
		else if (PostType == 'a') type = tAlbum;
		else type = tPlaylist;
	}

	if (!json[publisher_metadata].is_null()) artist = json[publisher_metadata].value("artist", std::string{});
	else artist = json["user"]["username"].get<std::string>(); // using get() instead of value() cuz this will never be null
	
	if (type == tTrack)
	{
		if (!GetStreamingUrl(json))
		{
			flags |= Error;
			return;
		}
	}
	else if (!GetTrackListIDs(json))
	{
		flags |= Error;
		return;
	}
}

void Track::HandleMetadata(std::string& path)
{
	TagLib::MPEG::File file(path.c_str());
	TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);

	// Title

	std::string value = cfg->title.empty() ? title : cfg->title;
	tag->setTitle(ToUtf8(value));

	// Album

	if (!cfg->album.empty()) value = cfg->album;
	else if (!cfg->title.empty()) value = cfg->title;

	tag->setAlbum(ToUtf8(value));

	// Artist

	tag->setArtist(ToUtf8((cfg->artists.empty() ? artist : cfg->artists)));

	// Comments

	value = "Release date: " + CreatedAt + "\n\nsaved with https://github.com/islipnot/SoundLoad";
	if (!description.empty()) value.insert(0, description + "\n\n");
	
	tag->setComment(ToUtf8(value));

	// Genre/track/year

	if (!cfg->genre.empty()) tag->setGenre(ToUtf8(cfg->genre));
	else if (!genre.empty()) tag->setGenre(ToUtf8(genre));

	if (cfg->TrackNum != -1) tag->setTrack(cfg->TrackNum);
	if (cfg->year != -1) tag->setYear(cfg->year);
	
	// MP3 cover

	if (cfg->cover.empty())
	{
		const bool NoUrl = CoverUrl.empty();

		if (!NoUrl || !ArtistPfp.empty())
		{
			value = std::regex_replace(NoUrl ? ArtistPfp : CoverUrl, std::regex("-large."), "-original.");
			cpr::Response r = cpr::Get(cpr::Url{ value });

			if (!RequestFail(r))
			{
				std::cout << "> ARTWORK URL: " << value << '\n';

				auto cover = new TagLib::ID3v2::AttachedPictureFrame;
				cover->setPicture(TagLib::ByteVector(r.text.data(), static_cast<UINT>(r.text.size())));
				tag->addFrame(cover);
			}
			else FetchErr(r);
		}
		else std::cout << "NO COVER FOUND (ScApi.cpp:HandleMetadata, IGNORING)\n";
	}

	file.save();
}

static bool DownloadM3U(const std::string& m3u, std::string& buffer)
{
	std::istringstream stream(m3u);
	std::string line;

	while (std::getline(stream, line))
	{
		if (line.starts_with("#EXTINF"))
		{
			std::getline(stream, line); // Skipping to next line, which is the URL

			const cpr::Response r = cpr::Get(cpr::Url{ line });
			if (RequestFail(r))
			{
				FetchErr(r);
				return false;
			}

			buffer += r.text;
		}
	}

	return true;
}

bool Track::DownloadTrack()
{
	// Requesting a download link

	cpr::Response r = cpr::Get(cpr::Url{ UrlData });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	const std::string url = Json::parse(r.text)["url"];

	DBG_MSG("> STREAMING URL: " + url);

	r = cpr::Get(cpr::Url{ url });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	// Getting and sanitizing MP3 output path

	std::string path = cfg->fName.empty() ? title : cfg->fName;
	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg->output.empty()) path.insert(0, cfg->output);
	path += ".mp3";

	// Downloading MP3(s) and writing output to disk

	char* pFile = nullptr;
	size_t size = 0;

	std::string HlsMp3;

	if (flags & Hls) // HLS downloads combine various segment files that make up one MP3
	{
		if (!DownloadM3U(r.text, HlsMp3)) return false;

		pFile = HlsMp3.data();
		size  = HlsMp3.size();
	}
	else
	{
		pFile = r.text.data();
		size  = r.text.size();
	}
	
	std::ofstream track(path, std::ios::binary | std::ios::trunc);
	track.write(pFile, size);
	track.close();
	
	// Handling metadata and MP3 tag

	HandleMetadata(path);

	return true;
}

bool Playlist::DownloadAlbum()
{
	// Requesting an array of resolved tracks from album
	// Im using this ID method because playlist resolutions (GetJson()) dont seem to include all streaming links for some reason

	const std::string url = "https://api-v2.soundcloud.com/tracks?ids=" + UrlData + "&client_id=" + cfg->CID;

	std::cout << "> ID RESOLUTION URL: " << url << '\n';

	const cpr::Response r = cpr::Get(cpr::Url{ url });

	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	// Downloading each track

	if (type == tAlbum && cfg->album.empty()) cfg->album = title;

	const Json tracks = Json::parse(r.text);

	for (const auto& track : tracks)
	{
		const char* permalink = track.value("permalink_url", nullptr);

		if (permalink == nullptr)
		{
			std::cout << "ERROR: TRACK permalink_url IS NULL (ignoring)\n";
			continue;
		}

		Track track(permalink, cfg);
		
		if (!track.DownloadTrack())
		{
			if (track.flags & Error) std::cerr << "ERROR: TRACK DOWNLOAD FAILED\n";
			else std::cout << "ERROR: TRACK DOWNLOAD FAILED (ignoring)\n";
		}
	}

	return true;
}

bool Track::DownloadCover()
{
	return true;
}