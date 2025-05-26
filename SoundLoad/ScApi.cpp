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
		if (transcoding["format"]["protocol"] == "progressive")
		{
			UrlData = transcoding.value("url", std::string{}) + "?client_id=" + cfg->CID;
			DBG_MSG("> PROGRESSIVE URL: " + UrlData);
			return true;
		}
	}

	std::cerr << "ERROR: progressive streaming url not found\n";
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
	const cpr::Response response = cpr::Get(cpr::Url{ url });

	std::cout << "> RESOLUTION URL: " << url << '\n';

	if (response.status_code != 200)
	{
		std::cerr << "ERROR: TRACK RESOLUTION FAILED - " << response.error.message << '\n';
		fail = true;
		return;
	}

	// Setting member variable values

	const Json json = Json::parse(response.text);

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
			type = track;
		}
		else if (PostType == 'a') type = album;
		else type = playlist;
	}

	if (json.contains(publisher_metadata)) artist = json[publisher_metadata].value("artist", std::string{});
	
	if (type == track)
	{
		if (!GetStreamingUrl(json))
		{
			fail = true;
			return;
		}
	}
	else if (!GetTrackListIDs(json))
	{
		fail = true;
		return;
	}
}

void Track::HandleMetadata(std::string& path)
{
	// Creating an ID3v2 tag for the MP3

	TagLib::MPEG::File file(path.c_str());
	TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);

	// Setting track properties

	std::string value = title.empty() ? cfg->title : title;

	tag->setTitle(TagLib::String(value.c_str(), TagLib::String::UTF8)); // if a title wasn't provided it'll use the file name (config.cpp/Cfg::Cfg)

	if (!cfg->album.empty()) value = cfg->album;
	else if (!cfg->title.empty()) value = cfg->title;
	
	tag->setAlbum(TagLib::String(value.c_str(), TagLib::String::UTF8)); // album defaults to track title for spotify, if you import it there

	value = cfg->artists.empty() ? artist : cfg->artists;

	tag->setArtist(TagLib::String(value.c_str(), TagLib::String::UTF8));

	value = "Release date: " + CreatedAt + "\n\n(saved with https://github.com/islipnot/SoundLoad)";

	if (!description.empty()) value.insert(0, description + "\n\n");
	
	tag->setComment(TagLib::String(value.c_str(), TagLib::String::UTF8));

	if (!genre.empty() || !cfg->genre.empty())
	{
		if (cfg->genre.empty()) value = genre;
		else value = cfg->genre;

		tag->setGenre(value.c_str());
	}

	if (cfg->TrackNum != -1)
	{
		tag->setTrack(cfg->TrackNum);
	}

	if (cfg->year != -1)
	{
		tag->setYear(cfg->year);
	}

	// Getting track cover

	value.clear();

	if (cfg->cover.empty())
	{
		const bool NoUrl = CoverUrl.empty();

		if (!NoUrl || !ArtistPfp.empty())
		{
			value = std::regex_replace(NoUrl ? ArtistPfp : CoverUrl, std::regex("-large."), "-original.");

			cpr::Response response = cpr::Get(cpr::Url{ value });

			std::cout << "> ARTWORK URL: " << value << '\n';

			if (response.status_code == 200)
			{
				auto cover = new TagLib::ID3v2::AttachedPictureFrame;
				cover->setPicture(TagLib::ByteVector(response.text.data(), static_cast<UINT>(response.text.size())));
				tag->addFrame(cover);
			}
			else std::cout << "FAILED TO GET TRACK COVER (ScApi.cpp:HandleMetadata) (IGNORING)\n";
		}
		else std::cout << "NO COVER FOUND (ScApi.cpp:HandleMetadata) (IGNORING)\n";
	}

	file.save();
}

bool Track::DownloadTrack()
{
	// Requesting a download link

	cpr::Response response = cpr::Get(cpr::Url{ UrlData });
	if (response.status_code != 200)
	{
		std::cerr << "FAILED TO GET TRACK (ScApi.cpp:DownloadTrack A)\n";
		return false;
	}

	const std::string url = Json::parse(response.text)["url"];

	DBG_MSG("> STREAMING URL: " + url);

	response = cpr::Get(cpr::Url{ url });
	if (response.status_code != 200)
	{
		std::cerr << "FAILED TO GET TRACK (ScApi.cpp:DownloadTrack B)\n";
		return false;
	}

	// Writing to output file

	std::string path;

	if (cfg->fName.empty()) path = title;
	else path = cfg->fName;

	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg->output.empty()) path.insert(0, cfg->output);

	path += ".mp3";
	
	std::ofstream track(path, std::ios::binary | std::ios::trunc);
	track.write(response.text.data(), response.text.size());
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

	const cpr::Response response = cpr::Get(cpr::Url{ url });

	if (response.status_code != 200)
	{
		std::cerr << "ERROR: PLAYLIST RESOLUTION FAILED - " << response.error.message << '\n';
		return false;
	}

	// Downloading each track

	if (type == album && cfg->album.empty()) cfg->album = title;

	const Json tracks = Json::parse(response.text);

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
			if (track.fail) std::cerr << "ERROR: TRACK DOWNLOAD FAILED\n";
			else std::cout << "ERROR: TRACK DOWNLOAD FAILED (ignoring)\n";
		}
	}

	return true;
}

bool Track::DownloadCover()
{
	return true;
}