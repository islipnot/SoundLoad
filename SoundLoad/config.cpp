#include "pch.hpp"
#include "config.hpp"

bool Cfg::OpenCfg(std::fstream& FileBuf)
{
	constexpr const char* name = "cfg.txt";

	if (!std::filesystem::exists(name))
	{
		std::ofstream cfg(name);
		cfg.close();

		DBG_MSG("Created cfg.txt");
	}

	FileBuf.open(name);
	return !FileBuf.fail();
}

void Cfg::ReadCfg(std::fstream& cfg)
{
	std::string value;

	while (std::getline(cfg, value))
	{
		const size_t seperator  = value.find_first_of(' ');
		const std::string field = value.substr(0, seperator);
		value.erase(0, seperator + 1);

		if (field == "cid")
		{
			if (CID.empty()) CID = value;

			flags |= HasCID;
		}
		else if (field == "out")
		{
			if (output.empty()) output = value;

			flags |= HasOut;
		}
		else if (field == "img")
		{
			flags |= HasImg;
		}
	}
}

void Cfg::SaveCfg(std::fstream& cfg)
{
	if (!(flags & HasCID))
	{
		cfg << "cid " + CID;

		DBG_MSG("Saved CID to cfg.txt");
	}

	if (!(flags & HasOut))
	{
		cfg << "out " + output;

		DBG_MSG("Saved output dir to cfg.txt");
	}
}

Cfg::Cfg(int argc, char* argv[])
{
	// Getting argument values

	bool save = false;

	const std::unordered_map<std::string, std::function<void(const char*)>> map = {
		{ "--client",  [this] (const char* v) { CID      = v; }},
		{ "--fname",   [this] (const char* v) { fName    = v; }},
		{ "--title",   [this] (const char* v) { title    = v; }},
		{ "--album",   [this] (const char* v) { album    = v; }},
		{ "--cartist", [this] (const char* v) { cArtists = v; }},
		{ "--artist",  [this] (const char* v) { artist   = v; }},
		{ "--output",  [this] (const char* v) { output   = v; }},
		{ "--cover",   [this] (const char* v) { cover    = v; }},
		{ "--save",    [&save](const char* v) { save     = 1; }}
	};

	for (int i = 2; i < argc; i += 2)
	{
		std::string key = argv[i];

		for (char& ch : key)
		{
			ch = std::tolower(ch);
		}

		const auto it = map.find(key);

		if (it == map.end())
		{
			std::cerr << "INVALID ARGUMENT: " << key << '\n';

			error = true;
			return;
		}

		if (it->first != "--save" && i + 1 >= argc)
		{
			ERR_MSG("No value provided for final arg");

			error = true;
			return;
		}

		it->second(argv[i + 1]);
	}

	// Handling config

	std::fstream cfg;

	if (!OpenCfg(cfg))
	{
		ERR_MSG("Failed to open cfg.txt");

		cfg.close();
		return;
	}

	ReadCfg(cfg);

	if (save) SaveCfg(cfg);

	cfg.close();
}