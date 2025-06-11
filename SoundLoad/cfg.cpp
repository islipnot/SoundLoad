#include "pch.hpp"
#include "cfg.hpp"

using Json = nlohmann::json;

void Config::AddPathVar()
{
	HKEY key = nullptr;

	if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) != ERROR_SUCCESS)
	{
		std::cout << "WARNING: FAILED TO OPEN HKEY_CURRENT_USER\\Environment\n";
		return;
	}

	constexpr PCWSTR ValName = L"Path";
	DWORD ValSz = 0;

	if (RegQueryValueExW(key, ValName, nullptr, nullptr, nullptr, &ValSz) == ERROR_SUCCESS)
	{
		std::wstring value(ValSz / sizeof(WCHAR), 0);

		if (RegQueryValueExW(key, ValName, nullptr, nullptr, reinterpret_cast<BYTE*>(value.data()), &ValSz) == ERROR_SUCCESS)
		{
			value.pop_back(); // removing null terminator
			if (value.back() != L';') value.push_back(L';');

			std::transform(value.begin(),  value.end(),  value.begin(),  ::tolower);
			std::transform(ExeDir.begin(), ExeDir.end(), ExeDir.begin(), ::tolower);

			if (value.find(ExeDir) == std::wstring::npos)
			{
				value += ExeDir;

				if (flags & AddPVar || MessageBox(nullptr, L"Add SoundLoad to user PATH variables?", L"SoundLoad", MB_YESNO | MB_ICONQUESTION) == IDYES)
				{
					if (RegSetValueExW(key, ValName, 0, REG_EXPAND_SZ, reinterpret_cast<BYTE*>(value.data()), value.size() * sizeof(WCHAR)) == ERROR_SUCCESS)
					{
						std::cout << "Added SoundLoad to environment variables!\n";
					}
					else std::cout << "WARNING: failed to set PATH value\n";
				}
			}
		}
		else std::cout << "WARNING: failed to query PATH value\n";
	}
	else std::cout << "WARNING: failed to query PATH size\n";
	
	RegCloseKey(key);
}

void Config::save(CfgFormat& cfg, const std::wstring& path)
{
	if (cfg.cid.empty() && !cid.empty()) cfg.cid = cid;
	if (cfg.ArtDst.empty() && !CoverDst.empty()) cfg.ArtDst = CoverDst;
	if (cfg.TrackDst.empty() && !TrackDst.empty()) cfg.TrackDst = TrackDst;

	const bool GetTrack = flags & GetAudio;
	const bool NoTrack  = flags & NoAudio;
	const bool GetArt   = flags & GetCover;
	const bool NoArt    = flags & NoCover;

	if (GetArt) cfg.GetArt = 1;
	else if (NoArt) cfg.GetArt = 0;

	if (GetTrack) cfg.GetTrack = 1;
	else if (NoTrack) cfg.GetTrack = 0;

	std::ofstream CfgFile(path, std::ios::out | std::ios::trunc);
	CfgFile << Json(cfg).dump(4);
	CfgFile.close();
}

void Config::read(Json& JsonCfg, CfgFormat& cfg, bool CidOnly)
{
	cfg.cid      = JsonCfg.value("cid",      std::string{});
	cfg.ArtDst   = JsonCfg.value("ArtDst",   std::string{});
	cfg.TrackDst = JsonCfg.value("TrackDst", std::string{});
	cfg.GetTrack = JsonCfg.value("GetTrack", 1);
	cfg.GetArt   = JsonCfg.value("GetArt",   0);

	if (cid.empty()) cid = cfg.cid;

	if (CidOnly) return;

	if (CoverDst.empty()) CoverDst = cfg.ArtDst;
	if (TrackDst.empty()) TrackDst = cfg.TrackDst;

	if (!(flags & (GetAudio | NoAudio)) && cfg.GetTrack != -1)
	{
		if (cfg.GetTrack == false) flags |= NoAudio;
		else flags |= GetAudio;
	}

	if (!(flags & (GetCover | NoCover)) && cfg.GetArt != -1)
	{
		if (cfg.GetArt == false) flags |= NoCover;
		else flags |= GetCover;
	}
}

Config::Config(int argc, char* argv[])
{
	// Hash map and lambdas for args

	const std::unordered_map<std::string, std::function<void(const std::string&)>> map =
	{
		{ "save",    [&](const std::string& v) { flags |= SaveCfg;  } },
		{ "art",     [&](const std::string& v) { flags |= GetCover; } },
		{ "-art",    [&](const std::string& v) { flags |= NoCover;  } },
		{ "-audio",  [&](const std::string& v) { flags |= NoAudio;  } },
		{ "audio",   [&](const std::string& v) { flags |= GetAudio; } },
		{ "pvar",    [&](const std::string& v) { flags |= AddPVar;  } },
		{ "cid",     [&](const std::string& v) { if (!v.empty()) cid       = v; } },
		{ "cfile",   [&](const std::string& v) { if (!v.empty()) CoverName = v; } },
		{ "cdst",    [&](const std::string& v) { if (!v.empty()) CoverDst  = v; } },
		{ "csrc",    [&](const std::string& v) { if (!v.empty()) CoverSrc  = v; } },
		{ "afile",   [&](const std::string& v) { if (!v.empty()) TrackName = v; } },
		{ "adst",    [&](const std::string& v) { if (!v.empty()) TrackDst  = v; } },
		{ "title",   [&](const std::string& v) { if (!v.empty()) title     = v; } },
		{ "comment", [&](const std::string& v) { if (!v.empty()) comments  = v; } },
		{ "cartist", [&](const std::string& v) { if (!v.empty()) cArtists  = v; } },
		{ "aartist", [&](const std::string& v) { if (!v.empty()) aArtist   = v; } },
		{ "album",   [&](const std::string& v) { if (!v.empty()) album     = v; } },
		{ "genre",   [&](const std::string& v) { if (!v.empty()) genre     = v; } },
		{ "tnum",    [&](const std::string& v) { tNum = std::stoi(v); } },
		{ "year",    [&](const std::string& v) { year = std::stoi(v); } }
	};

	// Parsing args

	if (argc > 1 && std::tolower(argv[1][0]) != 'h')
	{
		flags |= (NoAudio | NoCover);
	}

	for (int i = 1; i < argc; ++i)
	{
		if (argv[i][0] != '-')
		{
			if (i == 1) // Checking whether or not first arg is a link
			{
				++i;
				if (argc == 2) break; // Only the case if the SoundCloud link is the only arg
			}
			else continue;
		}

		std::string arg = &argv[i][1]; // Ignoring first dash in args
		std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

		const auto it = map.find(arg);

		if (it == map.end())
		{
			std::cerr << "ERROR: invalid argument(s)\n";
			flags |= Error;
			return;
		}

		std::string v;
		if (i + 1 < argc) v = argv[i + 1];
		it->second(v);
	}

	// Checking CoverSrc

	if (!CoverSrc.empty())
	{
		// This is not fool proof, some invalid links may pass this check

		if (std::filesystem::exists(CoverSrc)) flags |= tPath;
		else
		{
			std::transform(CoverSrc.begin(), CoverSrc.end(), CoverSrc.begin(), ::tolower);
			if (CoverSrc.starts_with("https://soundcloud.com/")) flags |= tScLink;
			else flags |= tImgLink;
		}
	}

	// Getting directory of SoundLoad.exe

	ExeDir.resize(MAX_PATH);

	if (!GetModuleFileName(nullptr, ExeDir.data(), MAX_PATH))
	{
		std::cerr << "ERROR: failed to get ExeDir\n";
		flags |= Error;
		return;
	}

	ExeDir = std::filesystem::path(ExeDir).parent_path().wstring() + L'\\';
}