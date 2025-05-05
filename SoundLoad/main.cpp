#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using json = nlohmann::json;

int main(int argc, char* argv[])
{
	// Handling arguments

	if (argc < 2)
	{
		std::cerr << "ERROR: insufficient arguments\n";
		return -1;
	}

	DBG_MSG("[*] HANDLING ARGUMENTS...\n");
	
	Cfg cfg(argc, argv);

	if (cfg.error) return -1;

	DBG_MSG("\n[*] SCRAPING TRACK DATA...\n");

	const json data = json::parse(GetRawJson(argv[1], cfg.CID));
	if (data.empty()) return -2;

	if (!DownloadTrack(data, cfg)) return -3;
	
	std::cout << "\n[!] SUCCESSFULLY DOWNLOADED TRACK!\n";

	return 0;
}