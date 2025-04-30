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
	const int TrackID = data["id"];
	const std::string TrackAuth = data["track_authorization"];
	const std::string StreamUrl = data["media"]["transcodings"][1]["url"];

	std::string url = StreamUrl + "?client_id=" + cfg.CID + "&track_authorization=" + TrackAuth;

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

	std::string fName = cfg.fName + ".mp3";
	if (!cfg.output.empty()) fName.insert(0, cfg.output);

	std::ofstream track(fName, std::ios::binary | std::ios::trunc);
	track.write(response.text.data(), response.text.size());

	return true;
}