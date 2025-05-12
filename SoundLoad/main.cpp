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

	std::cout << "[*] SCRAPING TRACK DATA...\n";

	const json data = json::parse(GetRawJson(argv[1], cfg.CID));

	if (!data.contains("kind") || data["kind"] != "track")
	{
		std::cerr << "ERROR: INVALID TRACK LINK\n";
		return -1;
	}

	if (!DownloadTrack(data, cfg)) return -1;
	
	if (data.empty()) return -1;
	
	std::cout << "[!] SUCCESSFULLY DOWNLOADED TRACK!\n";

	return 0;
}