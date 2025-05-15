#include "pch.hpp"
#include "config.hpp"

using json = nlohmann::json;

bool Cfg::RegPath()
{
	HKEY key;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) != ERROR_SUCCESS)
	{
		return false;
	}

	constexpr PCWSTR ValName = L"Path";
	DWORD ValSz = 0;

	if (RegQueryValueEx(key, ValName, nullptr, nullptr, nullptr, &ValSz) == ERROR_SUCCESS)
	{
		std::wstring value(ValSz / sizeof(WCHAR), 0);
		DWORD NewSz = value.size() * sizeof(WCHAR);

		if (RegQueryValueEx(key, ValName, nullptr, nullptr, reinterpret_cast<BYTE*>(value.data()), &NewSz) == ERROR_SUCCESS)
		{
			value.pop_back(); // removing null terminator

			if (value.back() != L';') value.push_back(L';');

			const std::wstring NewPath = std::filesystem::current_path().wstring() + L"\\SoundLoad.exe;";

			if (value.find(NewPath) != std::wstring::npos)
			{
				std::cout << "SoundLoad already exists in environment variables!\n";
				RegCloseKey(key);
				return true;
			}
			else value += NewPath;

			if (RegSetValueEx(key, ValName, 0, REG_EXPAND_SZ, reinterpret_cast<BYTE*>(value.data()), value.size() * sizeof(WCHAR)) == ERROR_SUCCESS)
			{
				RegCloseKey(key);
				return true;
			}
		}
	}

	RegCloseKey(key);
	return false;
}

void Cfg::ReadCfg(std::ifstream cfg)
{
	json data;

	try 
	{ 
		cfg >> data; 
	}
	catch (const std::exception& e) 
	{
		cfg.close();
		return;
	}

	cfg.close();

	const JsonCfg CfgData = data.get<JsonCfg>();

	if (!CfgData.cid.empty() && CID.empty())
	{
		CID = CfgData.cid;
		flags |= HasCID;
	}

	if (!CfgData.out.empty() && output.empty())
	{
		output = CfgData.out;
		flags |= HasOut;
	}
	
	if (!CfgData.img.empty() && cover.empty())
	{
		cover = CfgData.img;
		flags |= HasImg;
	}

	if (CfgData.ran) flags |= WasRan;
}

void Cfg::SaveCfg(const char* path)
{
	json data;
	JsonCfg CfgData;

	std::ifstream InCfg(path);

	try 
	{
		InCfg >> data;
		CfgData = data.get<JsonCfg>();
	}
	catch (const std::exception& e) {}

	InCfg.close();

	if (!CID.empty()    && CfgData.cid.empty()) CfgData.cid = CID;
	if (!output.empty() && CfgData.out.empty()) CfgData.out = output;
	if (!cover.empty()  && CfgData.img.empty()) CfgData.img = cover;

	std::ofstream cfg(path);

	if (cfg.is_open()) cfg << json(CfgData).dump(4);
	else std::cout << "ERROR: FAILED TO OPEN cfg.json FOR WRITING (ignoring)\n";

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
			if (argv[1][0] != '-')
			{
				++i;

				if (i == argc) break;
			}
			else status |= NoLink;
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

			status |= Error;
			return;
		}

		if (it->first != "-save" && i + 1 >= argc)
		{
			std::cerr << "NO VALUE PROVIDED FOR ARGUMENT: " << argv[i] << '\n';

			status |= Error;
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
	
	constexpr const char* name = "cfg.json";

	if (!std::filesystem::exists(name))
	{
		std::ofstream cfg(name);
		cfg.close();

		DBG_MSG("Created cfg.json");
	}

	if (save) SaveCfg(name);
	
	if (!(status & NoLink))
	{
		ReadCfg(std::ifstream(name));

		if (title.empty()) title = fName;
		if (!output.empty() && output.back() != '\\') output.push_back('\\');
	}

	if (CID.empty())
	{
		std::cerr << "ERROR: CLIENT ID NOT PROVIDED\n";
		status |= Error;
		return;
	}

	// Adding SoundLoad to environment variables if requested

	if (!(flags & WasRan) && MessageBox(nullptr, L"Add SoundLoad to environment variables?", L"SoundLoad", MB_YESNO | MB_ICONQUESTION) == IDYES)
	{
		if (RegPath())
		{
			std::cout << "Added SoundLoad to environment variables!\n";
			std::ofstream cfg(name);
			cfg << "ran\n";
			cfg.close();
		}
		else std::cout << "ERROR: FAILED TO ADD SOUNDLOAD TO ENVIRONMENT VARIABLES (ignoring)\n";
	}
}