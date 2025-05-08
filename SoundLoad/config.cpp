#include "pch.hpp"
#include "config.hpp"

void Cfg::ReadCfg(std::ifstream cfg)
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
			if (cover.empty()) cover = value;

			flags |= HasImg;
		}
	}

	cfg.close();
}

void Cfg::SaveCfg(std::ofstream cfg)
{
	if (!(flags & HasCID) && !CID.empty())
	{
		cfg << "cid " << CID << '\n';

		DBG_MSG("Saved CID to cfg.txt");
	}

	if (!(flags & HasOut) && !output.empty())
	{
		cfg << "out " << output << '\n';

		DBG_MSG("Saved output dir to cfg.txt");
	}

	if (!(flags & HasImg) && !cover.empty())
	{
		cfg << "img " << cover << '\n';

		DBG_MSG("Saved cover dir to cfg.txt");
	}

	cfg.close();
}

Cfg::Cfg(int argc, char* argv[])
{
	// Reading arguments

	bool save = false;
	
	const std::unordered_map<std::string, std::function<void(const char*)>> map = {
		{ "-cid",     [this] (const char* v) { CID     = v; }},
		{ "-fname",   [this] (const char* v) { fName   = v; }},
		{ "-title",   [this] (const char* v) { title   = v; }},
		{ "-album",   [this] (const char* v) { album   = v; }},
		{ "-artists", [this] (const char* v) { artists = v; }},
		{ "-artist",  [this] (const char* v) { artist  = v; }},
		{ "-genre",   [this] (const char* v) { genre   = v; }},
		{ "-out",     [this] (const char* v) { output  = v; }},
		{ "-cover",   [this] (const char* v) { cover   = v; }},
		{ "-save",    [&save](const char* v) { save    = 1; }},
		{ "-year",    [this] (const char* v) { year    = std::stoi(v); }},
		{ "-num",     [this] (const char* v) { num     = std::stoi(v); }}
	};

	for (int i = 1; i < argc; i += 2)
	{
		if (i == 1)
		{
			if (argv[1][0] != '-') ++i;
			else status |= NoLink;
		}

		std::string key = argv[i];

		for (char& ch : key) // converting to lowercase for hash map
		{
			ch = std::tolower(ch);
		}

		const auto it = map.find(key);

		if (it == map.end())
		{
			std::cerr << "INVALID ARGUMENT: " << key << '\n';

			status |= Error;
			return;
		}

		if (it->first != "-save" && i + 1 >= argc)
		{
			std::cerr << "NO VALUE PROVIDED FOR ARGUMENT: " << argv[i] << '\n';

			status |= Error;
			return;
		}

		it->second(argv[i + 1]);
	}

	// Handling config

	constexpr const char* name = "cfg.txt";

	if (!std::filesystem::exists(name))
	{
		std::ofstream cfg(name);
		cfg.close();

		DBG_MSG("Created cfg.txt");
	}

	if (!(flags & NoLink))
	{
		ReadCfg(std::ifstream(name));

		if (title.empty()) title = fName;
		if (!output.empty() && output.back() != '\\') output.push_back('\\');
	}

	if (save) SaveCfg(std::ofstream(name));
}