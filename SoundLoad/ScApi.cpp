#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using json = nlohmann::json;

std::string GetRawJson(const std::string track, const std::string& CID)
{
	const std::string url = "https://api-v2.soundcloud.com/resolve?url=" + track + "&client_id=" + CID;
	cpr::Response response = cpr::Get(cpr::Url{ url });

	if (response.status_code != 200)
	{
		ERR_MSG("Track resolution failed");
		ERR_INFO(url + '\n' + response.error.message);

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

	if (!cfg.album.empty()) value = cfg.album;

	tag->setAlbum(value.c_str());

	value = cfg.cArtists.empty() ? std::string(data["user"]["username"]) : cfg.cArtists;

	tag->setArtist(value.c_str());

	if (data.contains("description"))
	{
		tag->setComment(std::string(data["description"]));
	}

	// Getting track cover

	if (cfg.cover.empty())
	{
		value = std::regex_replace(std::string(data["artwork_url"]), std::regex("-large."), "-original.");
		cpr::Response response = cpr::Get(cpr::Url{ value });

		auto cover = new TagLib::ID3v2::AttachedPictureFrame;
		cover->setPicture(TagLib::ByteVector(response.text.data(), response.text.size()));

		tag->addFrame(cover);
	}

	file.save();
}

bool DownloadTrack(const json& data, Cfg& cfg)
{
	// Getting the streaming url

	const int TrackID = data["id"];
	const std::string TrackAuth = data["track_authorization"];

	std::string url;

	for (int i = 0, sz = data["media"]["transcodings"].size(); i < sz; ++i)
	{
		if (data["media"]["transcodings"][i]["format"]["protocol"] == "progressive")
		{
			url = data["media"]["transcodings"][i]["url"];
		}
	}

	url += ("?client_id=" + cfg.CID + "&track_authorization=" + TrackAuth);

	// Requesting a download link

	cpr::Response response = cpr::Get(cpr::Url{ url });
	if (response.status_code != 200)
	{
		ERR_MSG("Failed to download track");
		return false;
	}

	url = json::parse(response.text)["url"];

	DBG_MSG("Downloading from: " + url);

	response = cpr::Get(cpr::Url{ url });
	if (response.status_code != 200)
	{
		ERR_MSG("Failed to download track");
		return false;
	}

	// Writing to output file

	std::string path;

	if (cfg.fName.empty()) path = data["title"];
	else path = cfg.fName;

	if (!cfg.output.empty()) path.insert(0, cfg.output);

	path += ".mp3";

	std::ofstream track(path, std::ios::binary | std::ios::trunc);
	track.write(response.text.data(), response.text.size());
	track.close();

	// Handling metadata and MP3 tag

	HandleMetadata(data, cfg, path);

	return true;
}