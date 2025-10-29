#include "pch.hpp"
#include "config.hpp"

namespace cfg
{
	//
	//// ARGUMENT PARSING
	//

	static __forceinline void show_usage()
	{
		// to be added
	}

	static void set_arg_var(const char* src, std::string& dst)
	{
		if (src)
		{
			dst = src;
		}
		else
		{
			cfg::f.no_arg_data_provided = true;
		}
	}

	static void set_arg_var(const char* src, int& dst)
	{
		if (src)
		{
			try
			{
				dst = std::stoi(src);
			}
			catch (...)
			{
				cfg::f.invalid_data_provided = true;
			}
		}
		else
		{
			cfg::f.no_arg_data_provided = true;
		}
	}

	bool parse_arguments(int argc, char* argv[])
	{
		// Checking arg count

		if (argc < 2)
		{
			err::log("insufficient arguments, use '-?' for program usage");
			return false;
		}

		// Checking if first argument (link required for any downloads)
		
		if (_strnicmp(argv[1], "https://soundcloud.com/", 23) != 0)
		{
			cfg::f.no_link_provided = true;

			if (!strcmp(argv[1], "-?"))
			{
				show_usage();
				return false;
			}
		}
		
		// Parsing arguments (starts at index 2 if first arg is a link)

		for (int i = 1 + ((cfg::f.no_link_provided) == false); i < argc; ++i)
		{
			// Parsing argument data

			if (argv[i][0] != '-')
			{
				err::log("unrecognized argument \"{}\"", argv[i]);
				return false;
			}

			const char* next_arg = nullptr;
			if (i + 1 < argc) next_arg = argv[i + 1];

			switch (hash_rt(&argv[i][1]))
			{
				// Flag arguments

			case hash("save"):    { cfg::f.save_config            = true; break; } // Save applicable variables to config
			case hash("art"):     { cfg::f.download_art_seperate  = true; break; } // Downloads cover art independently
			case hash("n-art"):   { cfg::f.disable_art_download   = true; break; } // Prevents independent cover art download
			case hash("audio"):   { cfg::f.download_audio         = true; break; } // Enables MP3 downloading
			case hash("n-audio"): { cfg::f.disable_audio_download = true; break; } // Disables MP3 downloading
			case hash("pvars"):   { cfg::f.add_to_path            = true; break; } // Add program to PATH variables

				// Config data arguments

			case hash("cid"):      { set_arg_var(next_arg, cfg::client_id);                    ++i; break; } // Client ID
			case hash("img-name"): { set_arg_var(next_arg, cfg::g_track_data.image_file_name); ++i; break; } // Downloaded cover art file name
			case hash("mp3-name"): { set_arg_var(next_arg, cfg::g_track_data.audio_file_name); ++i; break; } // Downloaded MP3 file name
			case hash("img-dst"):  { set_arg_var(next_arg, cfg::image_out_dir);                ++i; break; } // Cover art download directory
			case hash("mp3-dst"):  { set_arg_var(next_arg, cfg::audio_out_dir);                ++i; break; } // MP3 download directory
			case hash("img-src"):  { set_arg_var(next_arg, cfg::image_src);                    ++i; break; } // MP3 cover art source
								
				// MP3 ID3v2 data arguments

			case hash("title"):    { set_arg_var(next_arg, cfg::g_track_data.title);           ++i; break; }
			case hash("comment"):  { set_arg_var(next_arg, cfg::g_track_data.comments);        ++i; break; }
			case hash("artists"):  { set_arg_var(next_arg, cfg::g_track_data.contrib_artists); ++i; break; } // Contributing artists
			case hash("a-artist"): { set_arg_var(next_arg, cfg::g_track_data.album_artists);   ++i; break; } // Album artist
			case hash("album"):    { set_arg_var(next_arg, cfg::g_track_data.album);           ++i; break; }
			case hash("genre"):    { set_arg_var(next_arg, cfg::g_track_data.genre);           ++i; break; }
			case hash("num"):      { set_arg_var(next_arg, cfg::g_track_data.number);          ++i; break; }
			case hash("year"):     { set_arg_var(next_arg, cfg::g_track_data.year);            ++i; break; }

				// Invalid arguments

			default:
			{
				err::log("invalid argument \"{}\"", argv[i]);
				return false;
			}
			}

			// Verifying valid data was provided

			if (cfg::f.no_arg_data_provided)
			{
				err::log("no variable provided for argument \"{}\"", argv[i - 1]);
				return false;
			}

			if (cfg::f.invalid_data_provided)
			{
				err::log("non-numeric or out of range variable (\"{}\") provided for argument \"{}\"", next_arg, argv[i - 1]);
				return false;
			}
		}

		// Validating arguments

		// Getting cover art source type

		if (!cfg::image_src.empty())
		{
			if (std::filesystem::exists(cfg::image_src))
			{
				cfg::f.cover_src_is_path = true;
			}
			else if (cfg::image_src.starts_with("https://soundcloud.com/"))
			{
				cfg::f.cover_src_is_sc_link = true;
			}
		}

		// Getting process directory

		WCHAR program_path[MAX_PATH];

		if (!GetModuleFileNameW(nullptr, program_path, MAX_PATH))
		{
			err::log_ex("failed to get soundload path");
			return false;
		}

		cfg::program_dir = std::filesystem::path(program_path).parent_path().wstring();

		return true;
	}

	//
	//// CONFIG HANDLING
	//

	void read_config(const cfg_format& data)
	{
		auto read_str = [](const std::string& src, std::string& dst) 
			{ 
				if (!src.empty() && dst.empty()) dst = src; 
			};

		read_str(data.cid,           cfg::client_id);
		read_str(data.art_out_dir,   cfg::image_out_dir);
		read_str(data.track_out_dir, cfg::audio_out_dir);

		if (!cfg::audio_flags_set() && !data.get_track_audio)
		{
			cfg::f.disable_audio_download = true;
		}

		if (!cfg::art_flags_set() && data.get_track_art)
		{
			cfg::f.download_art_seperate = true;
		}
	}

	void save_config(cfg_format& data)
	{
		auto save_str = [](std::string& dst, const std::string& src)
			{
				if (!src.empty()) dst = src;
			};

		save_str(data.cid,           cfg::client_id);
		save_str(data.art_out_dir,   cfg::image_out_dir);
		save_str(data.track_out_dir, cfg::audio_out_dir);

		if (cfg::audio_flags_set())
		{
			data.get_track_audio = cfg::f.download_audio;
		}

		if (cfg::art_flags_set())
		{
			data.get_track_art = cfg::f.download_art_seperate;
		}
	}

	//
	//// PATH variable
	//

	void add_to_path()
	{
		// Getting 'HKCU/Environment/Path' value

		HKEY hkey = nullptr;
		if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Environment", 0, KEY_QUERY_VALUE | KEY_SET_VALUE, &hkey) != ERROR_SUCCESS)
		{
			err::log_ex("failed to open HKCU/Environment (ignoring)");
			return;
		}

		DWORD cbSize = 0;
		if (RegGetValueW(hkey, nullptr, L"Path", RRF_NOEXPAND | RRF_RT_REG_EXPAND_SZ, nullptr, nullptr, &cbSize) != ERROR_SUCCESS)
		{
			err::log_ex("failed to query size of HKCU/Environment/Path (ignoring)");
			RegCloseKey(hkey);
			return;
		}

		std::wstring value(cbSize / sizeof(WCHAR), 0);
		if (RegGetValueW(hkey, nullptr, L"Path", RRF_NOEXPAND | RRF_RT_REG_EXPAND_SZ, nullptr, value.data(), &cbSize) != ERROR_SUCCESS)
		{
			err::log_ex("failed to query value of HKCU/Environment/Path (ignoring)");
			RegCloseKey(hkey);
			return;
		}

		// Formatting value

		value.erase(value.find_first_of(L'\0')); // there are multiple null terminators sometimes
		if (value.back() != L';') value.push_back(L';');

		std::transform(value.begin(), value.end(), value.begin(), ::tolower);
		std::transform(cfg::program_dir.begin(), cfg::program_dir.end(), cfg::program_dir.begin(), ::tolower);

		// Setting value

		if (value.find(cfg::program_dir) == std::wstring::npos
		&& (cfg::f.add_to_path || MessageBoxW(nullptr, L"Add SoundLoad to PATH variables?", L"SoundLoad", MB_YESNO | MB_ICONQUESTION) == IDYES))
		{
			value += cfg::program_dir + L';';

			if (RegSetValueExW(hkey, L"Path", 0, REG_EXPAND_SZ, reinterpret_cast<BYTE*>(value.data()), static_cast<DWORD>(value.size() * sizeof(WCHAR))) == ERROR_SUCCESS)
			{
				std::cout << "[!] Added SoundLoad to PATH variables\n";
			}
			else err::log_ex("failed to set HKCU/Environment/Path value (ignoring)");
		}

		RegCloseKey(hkey);
	}
}