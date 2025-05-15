#include "pch.hpp"
#include "config.hpp"

using json = nlohmann::json;

void Cfg::SetPathVar()
{
	if (!(flags & AddEnvVar) && MessageBox(nullptr, L"Add SoundLoad to environment variables?", L"SoundLoad", MB_YESNO | MB_ICONQUESTION) != IDYES)
	{
		return;
	}

	HKEY key;

	if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) != ERROR_SUCCESS)
	{
		return;
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

			std::wstring NewPath = std::filesystem::current_path().wstring() + L"\\soundload.exe;";

			std::transform(value.begin(), value.end(), value.begin(), ::tolower);
			std::transform(NewPath.begin(), NewPath.end(), NewPath.begin(), ::tolower);

			if (value.find(NewPath) != std::wstring::npos)
			{
				std::cout << "SoundLoad already exists in environment variables!\n";

				RegCloseKey(key);
				return;
			}
			else value += NewPath;

			if (RegSetValueEx(key, ValName, 0, REG_EXPAND_SZ, reinterpret_cast<BYTE*>(value.data()), value.size() * sizeof(WCHAR)) == ERROR_SUCCESS)
			{
				std::cout << "Added SoundLoad to environment variables!\n";

				RegCloseKey(key);
				return;
			}
		}
	}

	std::cout << "ERROR: FAILED TO ADD SOUNDLOAD TO ENVIRONMENT VARIABLES (ignoring)\n";
	RegCloseKey(key);
}

void Cfg::ReadCfg(std::ifstream cfg)
{
	json data;

	try { cfg >> data; }
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

		if (!CfgData.ran)
		{
			CfgData.ran = true;
			flags |= WasRan;
		}
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

void Cfg::ReadArgs(int argc, char* argv[])
{
	const std::unordered_map<std::string, int> map = {
		{ "-cid",     a_cid      },
		{ "-fname",   a_fname    },
		{ "-title",   a_title    },
		{ "-album",   a_album    },
		{ "-artists", a_artists  },
		{ "-artist",  a_artist   },
		{ "-genre",   a_genre    },
		{ "-out",     a_out      },
		{ "-cover",   a_cover    },
		{ "-save",    a_save     },
		{ "-year",    a_year     },
		{ "-num",     a_num      },
		{ "-envvar",  WasRan }
	};

	for (int i = 1; i < argc; i += 2)
	{
		if (i == 1) // Checking if the first argument is a track link or not
		{
			if (argv[1][0] != '-')
			{
				++i;

				if (i == argc) break;
			}
			else status |= NoLink;
		}

		std::string key = argv[i];
		std::transform(key.begin(), key.end(), key.begin(), ::tolower); // Converting to lowercase for hash map

		const auto it = map.find(key);

		if (it == map.end())
		{
			std::cerr << "ERROR: INVALID ARGUMENT - " << key << '\n';
			status |= Error;
			return;
		}

		if (it->first != "-save" && it->first != "-envvar" && i + 1 >= argc) // I chose not to ignore this error because it might have undesired effects
		{
			std::cerr << "ERROR: NO VALUE PROVIDED FOR ARGUMENT - " << argv[i] << '\n';
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
		case a_save:    { status |= Save;         break; }
		case a_envvar:  { flags  |= AddEnvVar;    break; }
		case a_year:    { year    = std::stoi(v); break; }
		case a_num:     { num     = std::stoi(v); break; }
		}
	}
}

Cfg::Cfg(int argc, char* argv[])
{
	ReadArgs(argc, argv);

	if (status & Error) return;

	// Handling config
	
	constexpr const char* name = "cfg.json";

	if (!std::filesystem::exists(name))
	{
		std::ofstream cfg(name);
		cfg.close();

		DBG_MSG("Created cfg.json");
	}

	if (status & Save) SaveCfg(name);
	
	if (!(status & NoLink))
	{
		ReadCfg(std::ifstream(name));

		if (CID.empty())
		{
			std::cerr << "ERROR: CLIENT ID NOT PROVIDED\n";
			status |= Error;
			return;
		}

		if (title.empty()) title = fName;
		if (!output.empty() && output.back() != '\\') output.push_back('\\');
	}

	// Adding SoundLoad to environment variables if requested

	if (!(flags & WasRan) || flags & AddEnvVar) SetPathVar();
}