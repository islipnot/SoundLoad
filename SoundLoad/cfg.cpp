#include "pch.hpp"
#include "cfg.hpp"

#define HashArg(arg) HashArgEx(arg, sizeof(arg) - 1)

static constexpr SIZE_T HashArgEx(const char* arg, const int sz)
{
	SIZE_T hash = 14695981039346656037ULL;

	for (int i = 0 ; i < sz; ++i)
	{
		hash ^= static_cast<uint32_t>(arg[i]);
		hash *= 1099511628211ULL;
	}

	return hash;
}

using Json = nlohmann::json;

namespace cfg
{
	void AddToPath()
	{
		HKEY key = nullptr;

		if (RegOpenKeyEx(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &key) != ERROR_SUCCESS)
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
					if (flags & AddToPathVariables || MessageBox(nullptr, L"Add SoundLoad to user PATH variables?", L"SoundLoad", MB_YESNO | MB_ICONQUESTION) == IDYES)
					{
						value += ExeDir;

						if (RegSetValueExW(key, ValName, 0, REG_EXPAND_SZ, reinterpret_cast<BYTE*>(value.data()), value.size() * sizeof(WCHAR)) == ERROR_SUCCESS)
						{
							std::cout << "Added SoundLoad to environment variables!\n";
						}
						else std::cout << "WARNING: failed to add SoundLoad to PATH variables\n";
					}
				}
			}
			else std::cout << "WARNING: failed to query PATH value\n";
		}
		else std::cout << "WARNING: failed to query PATH size\n";

		RegCloseKey(key);
	}

	void SaveConfig(CfgFormat& data, const std::wstring& path)
	{
		if (!cid.empty() && data.cid.empty()) data.cid = cid;
		if (!ArtDir.empty() && data.ArtDir.empty()) data.ArtDir = ArtDir;
		if (!TrackDir.empty() && data.TrackDir.empty()) data.TrackDir = TrackDir;

		if (flags & GetAudio)          data.GetTrack = true;
		else if (flags & DontGetAudio) data.GetTrack = false;

		if (flags & GetArtIndependent) data.GetArt = true;
		else if (flags & DontGetArt)   data.GetArt = false;

		std::ofstream CfgFile(path, std::ios::trunc);
		CfgFile << Json(data).dump(4);
		CfgFile.close();
	}

	void ReadConfig(const CfgFormat& data)
	{
		if (cid.empty() && !data.cid.empty()) cid = data.cid;
		if (ArtDir.empty() && !data.ArtDir.empty()) ArtDir = data.ArtDir;
		if (TrackDir.empty() && !data.TrackDir.empty()) TrackDir = data.TrackDir;
		
		if (!(flags & (GetAudio | DontGetAudio)) && data.GetTrack != -1)
		{
			flags |= data.GetTrack ? GetAudio : DontGetAudio;
		}
		if (!(flags & (GetArtIndependent | DontGetArt)) && data.GetArt != -1)
		{
			flags |= data.GetArt ? GetArtIndependent : DontGetArt;
		}
	}

	bool GetConfig(int argc, char* argv[])
	{
		if (strlen(argv[1]) < 8 || _strnicmp(argv[1], "https://", 8) != 0)
		{
			flags |= NoLinkProvided;
		}

		// Parsing arguments (starts at index 2 if first argument is a link)

		for (int i = 1 + ~(flags & NoLinkProvided); i < argc; ++i)
		{
			if (argv[i][0] != '-') continue;

			std::string arg = &argv[i][1]; // [i][1] removes '-'
			std::transform(arg.begin(), arg.end(), arg.begin(), ::tolower);

			const char* param = nullptr;
			if (i + 1 < argc) param = argv[i + 1];
		
			switch (HashArgEx(arg.c_str(), static_cast<int>(arg.size())))
			{
			case HashArg("save"):    { flags |= SaveToConfig;                     break; }
			case HashArg("art"):     { flags |= GetArtIndependent;                break; }
			case HashArg("-art"):    { flags |= DontGetArt;                       break; }
			case HashArg("-audio"):  { flags |= DontGetAudio;                     break; }
			case HashArg("audio"):   { flags |= GetAudio;                         break; }
			case HashArg("pvar"):    { flags |= AddToPathVariables;               break; }
			case HashArg("cid"):     { if (param) cid         = param;            break; }
			case HashArg("cfile"):   { if (param) ArtName     = param;            break; }
			case HashArg("cdst"):    { if (param) ArtDir      = param;            break; }
			case HashArg("csrc"):    { if (param) CoverSrc    = param;            break; }
			case HashArg("afile"):   { if (param) TrackName   = param;            break; }
			case HashArg("adst"):    { if (param) TrackDir    = param;            break; }
			case HashArg("title"):   { if (param) TrackTitle  = param;            break; }
			case HashArg("comment"): { if (param) comment     = param;            break; }
			case HashArg("cartist"): { if (param) cArtist     = param;            break; }
			case HashArg("aartist"): { if (param) aArtist     = param;            break; }
			case HashArg("album"):   { if (param) album       = param;            break; }
			case HashArg("genre"):   { if (param) genre       = param;            break; }
			case HashArg("tnum"):    { if (param) TrackNumber = std::stoi(param); break; }
			case HashArg("year"):    { if (param) year        = std::stoi(param); break; }

			default:
			{
				std::cerr << "ERROR: invalid argument \"" << arg << "\"\n";
				return false;
			}
			}
		}

		// Getting cover art source type

		if (!CoverSrc.empty())
		{
			if (std::filesystem::exists(CoverSrc))
			{
				flags |= ArtSrcPath;
			}
			else
			{
				std::transform(CoverSrc.begin(), CoverSrc.end(), CoverSrc.begin(), ::tolower);
				flags |= CoverSrc.starts_with("https://soundcloud.com/") ? ArtSrcScLink : ArtSrcImgLink;
			}
		}

		// Getting SoundLoad.exe directory

		ExeDir.resize(MAX_PATH);

		if (!GetModuleFileName(nullptr, ExeDir.data(), MAX_PATH))
		{
			std::cerr << "ERROR: failed to get cfg.json path\n";
			return false;
		}

		ExeDir = std::filesystem::path(ExeDir).parent_path().wstring() + L'\\';

		return true;
	}
}