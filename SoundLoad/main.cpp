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

	DBG_MSG("[*] HANDLING ARGUMENTS...\n");
	
	Cfg cfg(argc, argv);

	if (cfg.status & Cfg::Error) return -1;
	if (cfg.status & Cfg::NoLink) return 0;

	DBG_MSG("\n[*] SCRAPING TRACK DATA...\n");

	const json data = json::parse(GetRawJson(argv[1], cfg.CID));

	if (!DownloadTrack(data, cfg)) return -1;
	
	if (data.empty()) return -1;
	
	std::cout << "\n[!] SUCCESSFULLY DOWNLOADED TRACK!\n";

	return 0;
}