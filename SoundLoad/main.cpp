#include "pch.hpp"
#include "config.hpp"
#include "ScApi.hpp"

using json = nlohmann::json;

/* ARGUMENTS
* 
* ADDED:
* 
* --client  <client_id>
* --fName   <mp3 file name>
* --title   <mp3 title property>
* --cArtist <contributing artists property> (SURROUND IN QUOTES IF MULTIPLE ARTISTS)
* --artist  <album artist property>
* --album   <album property>
* --output  <final mp3 output dir> (defaults to command directory at runtime)
* --cover   <cover path>
* --save    (saves args to cfg)
* 
* Above fields (besides client ID) are scraped from track page or left empty if not provided by the user
*/

int main(int argc, char* argv[])
{
	SetConsoleOutputCP(CP_UTF8);

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