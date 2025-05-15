#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "ERROR: insufficient arguments\n";
		return -1;
	}

	std::cout << "[*] HANDLING ARGUMENTS...\n\n";
	
	Cfg cfg(argc, argv);

	if (cfg.status & Cfg::Error) return -1;
	if (cfg.status & Cfg::NoLink) return 0;

	std::cout << "\n[*] SCRAPING PAGE DATA...\n\n";

	const json data = json::parse(GetJson(argv[1], cfg.CID));

	if (data["kind"] == "playlist")
	{
		if (!DownloadPlaylist(data, cfg)) return -1;
	}
	else if (!DownloadTrack(data, cfg)) return -1;
	
	if (data.empty()) return -1;
	
	std::cout << "\n[!] DOWNLOAD SUCCESSFUL!\n";

	return 0;
}