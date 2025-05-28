#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using Json = nlohmann::json;

int main(int argc, char* argv[])
{
	if (argc < 2)
	{
		std::cerr << "ERROR: insufficient arguments\n";
		return -1;
	}

	Cfg cfg(argc, argv);

	if (cfg.status & Cfg::Error) return -1;
	if (cfg.status & Cfg::NoLink) return 0;

	PostData ScPost(argv[1], &cfg);
	if (!ScPost.download()) return -1;
	
	std::cout << "\n[!] DOWNLOAD SUCCESSFUL!\n";

	return 0;
}