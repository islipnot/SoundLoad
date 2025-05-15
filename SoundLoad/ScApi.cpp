#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using json = nlohmann::json;

std::string GetRawJson(std::string track, const std::string& CID)
{
	if (track.find('?') != std::string::npos) // for when you use the copy link button rather than copying browser url
	{
		track.erase(track.find_first_of('?'));
	}

	const std::string url = "https://api-v2.soundcloud.com/resolve?url=" + track + "&client_id=" + CID;
	cpr::Response response = cpr::Get(cpr::Url{ url });
	
	std::cout << "> RESOLUTION URL: " << url << '\n';

	if (response.status_code != 200)
	{
		std::cerr << "ERROR: TRACK RESOLUTION FAILED || " << response.error.message << '\n';
		return {};
	}

	return response.text;
}

void HandleMetadata(const json& data, Cfg& cfg, std::string& path)
{
	// Creating an ID3v2 tag for the MP3

	TagLib::MPEG::File file(path.c_str());
	TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);

	// Setting track properties

	std::string value = cfg.title.empty() ? std::string(data["title"]) : cfg.title;

	tag->setTitle(value.c_str()); // if a title wasn't provided it'll use the file name (config.cpp/Cfg::Cfg)

	if (!cfg.album.empty())
	{
		value = cfg.album;
	}

	tag->setAlbum(value.c_str()); // album defaults to track title for spotify, if you import it there

	value = cfg.artists.empty() ? std::string(data["user"]["username"]) : cfg.artists;

	tag->setArtist(value.c_str());

	value = "(github.com/islipnot/SoundLoad)";

	if (data.contains("description"))
	{
		value.insert(0, std::string(data["description"]) + ' ');
	}

	tag->setComment(value.c_str());

	if (data.contains("genre") || !cfg.genre.empty())
	{
		if (cfg.genre.empty()) value = std::string(data["genre"]);
		else value = cfg.genre;

		tag->setGenre(value.c_str());
	}

	if (cfg.num != -1)
	{
		tag->setTrack(cfg.num);
	}

	if (cfg.year != -1)
	{
		tag->setYear(cfg.year);
	}

	// Getting track cover

	if (cfg.cover.empty())
	{
		value = std::regex_replace(std::string(data["artwork_url"]), std::regex("-large."), "-original.");
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

	file.save();
}

bool DownloadTrack(const json& data, Cfg& cfg)
{
	// Getting the streaming url

	std::string url;

	for (const auto& transcoding : data["media"]["transcodings"])
	{
		if (transcoding["format"]["protocol"] == "progressive")
		{
			url = transcoding["url"];
		}
	}

	// Requesting a download link

	cpr::Response response = cpr::Get(cpr::Url{ url + "?client_id=" + cfg.CID });
	if (response.status_code != 200)
	{
		std::cerr << "FAILED TO GET TRACK (ScApi.cpp:DownloadTrack A)\n";
		return false;
	}

	url = json::parse(response.text)["url"];

	DBG_MSG("> STREAMING URL: " + url);

	response = cpr::Get(cpr::Url{ url });
	if (response.status_code != 200)
	{
		std::cerr << "FAILED TO GET TRACK (ScApi.cpp:DownloadTrack B)\n";
		return false;
	}

	// Writing to output file

	std::string path;

	if (cfg.fName.empty()) path = data["title"];
	else path = cfg.fName;

	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg.output.empty()) path.insert(0, cfg.output);

	path += ".mp3";
	
	std::ofstream track(path, std::ios::binary | std::ios::trunc);
	track.write(response.text.data(), response.text.size());
	track.close();

	// Handling metadata and MP3 tag

	HandleMetadata(data, cfg, path);

	return true;
}