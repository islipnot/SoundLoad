#include "pch.hpp"
#include "config.hpp"

void Cfg::ReadCfg(std::ifstream cfg)
{
	std::string value;

	const std::unordered_map<std::string, int> map = {
		{ "cid", HasCID },
		{ "out", HasOut },
		{ "img", HasImg },
		{ "ran", WasRan}
	};

	while (std::getline(cfg, value))
	{
		const size_t seperator = value.find_first_of(' ');
		const std::string key = value.substr(0, seperator);
		value.erase(0, seperator + 1);

		const auto it = map.find(key);

		if (it == map.end()) continue;

		switch (it->second)
		{
		case HasCID:

			if (CID.empty()) CID = value;
			flags |= HasCID;
			break;

		case HasOut:

			if (output.empty()) output = value;
			flags |= HasOut;
			break;

		case HasImg:

			if (cover.empty()) cover = value;
			flags |= HasImg;
			break;

		case WasRan:

			flags |= WasRan;
			break;
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

	const std::unordered_map<std::string, int> map = {
		{ "-cid",     a_cid     },
		{ "-fname",   a_fname   },
		{ "-title",   a_title   },
		{ "-album",   a_album   },
		{ "-artists", a_artists },
		{ "-artist",  a_artist  },
		{ "-genre",   a_genre   },
		{ "-out",     a_out     },
		{ "-cover",   a_cover   },
		{ "-save",    a_save    },
		{ "-year",    a_year    },
		{ "-num",     a_num     }
	};

	for (int i = 1; i < argc; i += 2)
	{
		if (i == 1)
		{
			if (argv[1][0] != '-') ++i;
			else flags |= NoLink;
		}

		std::string key = argv[i];

		for (char& ch : key) // converting to lowercase for hash map
		{
			if (std::isalpha(ch)) ch = std::tolower(ch);
		}

		const auto it = map.find(key);

		if (it == map.end())
		{
			std::cerr << "INVALID ARGUMENT: " << key << '\n';

			flags |= Error;
			return;
		}

		if (it->first != "-save" && i + 1 >= argc)
		{
			std::cerr << "NO VALUE PROVIDED FOR ARGUMENT: " << argv[i] << '\n';

			flags |= Error;
			return;
		}

		const char* v = argv[i + 1];

		switch (it->second)
		{
		case a_cid:     { CID     = v; break; }
		case a_fname:   { fName   = v; break; }
		case a_title:   { title   = v; break; }
		case a_album:   { album   = v; break; }
		case a_artists: { artists = v; break; }
		case a_artist:  { artist  = v; break; }
		case a_genre:   { genre   = v; break; }
		case a_out:     { output  = v; break; }
		case a_cover:   { cover   = v; break; }
		case a_save:    { save    = 1; break; }
		case a_year:    { year = std::stoi(v); break; }
		case a_num:     { num  = std::stoi(v); break; }
		}
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

	// Adding SoundLoad to environment variables if requested

	/*if (!(flags & WasRan) && MessageBox(nullptr, L"Add SoundLoad to environment variables?", L"SoundLoad", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		HANDLE handle = GetCurrentProcess();
		OpenProcessToken(handle, TOKEN_IMPERSONATE | TOKEN_QUERY | TOKEN_DUPLICATE, &handle);
	}*/
}