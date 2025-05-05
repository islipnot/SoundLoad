#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using json = nlohmann::json;

std::string GetRawJson(const std::string track, const std::string& CID)
{
	const std::string url = "https://api-v2.soundcloud.com/resolve?url=" + track + "&client_id=" + CID;
	cpr::Response response = cpr::Get(cpr::Url{ url });

	if (response.status_code > 399)
	{
		ERR_MSG("Track resolution failed");
		ERR_INFO(url + '\n' + response.error.message);

		return {};
	}

	return response.text;
}

bool DownloadTrack(const json data, Cfg& cfg)
{
	// Getting the streaming url

	const int TrackID = data["id"];
	const std::string TrackAuth = data["track_authorization"];

	std::string url;

	for (int i = 0; i < 3; ++i)
	{
		if (data["media"]["transcodings"][i]["format"]["protocol"] == "progressive")
		{
			url = data["media"]["transcodings"][i]["url"];
		}
	}

	url += ("?client_id=" + cfg.CID + "&track_authorization=" + TrackAuth);

	// Requesting a download link

	cpr::Response response = cpr::Get(cpr::Url{ url });
	if (response.status_code > 399)
	{
		ERR_MSG("Failed to download track");
		return false;
	}

	url = json::parse(response.text)["url"];

	DBG_MSG("Downloading from: " + url);

	response = cpr::Get(cpr::Url{ url });
	if (response.status_code > 399)
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

	return true;
}