#include "pch.hpp"
#include "../config/config.hpp"
#include "../site_api/site_api.hpp"

static bool handle_config()
{
	const std::wstring path = cfg::program_dir + L"\\cfg.json";
	const bool cfg_exists = std::filesystem::exists(path);

	std::fstream cfg_file;
	cfg_format cfg_data_raw = {};

	// Reading from config if it exists

	if (cfg_exists)
	{
		cfg_file.open(path, std::ios::in);
		if (cfg_file.fail())
		{
			err::log("failed to open cfg.json");
			return false;
		}

		try
		{
			Json cfg_data_json;
			cfg_file >> cfg_data_json;
			cfg_data_raw = cfg_data_json.get<cfg_format>();
		}
		catch (...)
		{
			cfg_file.close();
			err::log("failed to read config");
			return false;
		}

		cfg_file.close();
		cfg::read_config(cfg_data_raw);
	}
	else
	{
		cfg::f.config_just_created = true;
	}
	
	// Saving config if requested

	if (cfg::f.save_config)
	{
		cfg::save_config(cfg_data_raw);
		
		cfg_file.open(path, std::ios::out | std::ios::trunc);
		if (cfg_file.fail())
		{
			err::log("failed to open/create cfg.json");
			return false;
		}

		try
		{
			cfg_file << Json(cfg_data_raw).dump(4);
		}
		catch (...)
		{
			cfg_file.close();
			err::log("failed to save config");
			return false;
		}

		cfg_file.close();
	}

	return true;
}

int main(int argc, char* argv[])
{
	// Parsing arguments

	if (!cfg::parse_arguments(argc, argv))
		return 1;

	// Reading/saving config

	if (!handle_config())
		return 2;

	// Adding program to PATH variables

	if (cfg::f.add_to_path || cfg::f.config_just_created)
		cfg::add_to_path(); // failure is ignored as its non-vital to primary functionality

	cfg::program_dir.clear();

	// Preparing for download(s)

	if (cfg::f.no_link_provided)
	{
		std::cout << "\n[!] INPUT HANDLED\n";
		return 0;
	}
	
	if (cfg::client_id.empty())
	{
		err::log("no client ID provided");
		return 3;
	}

	{
		auto fix_path = [](std::string& path)
			{
				if (!path.empty())
				{
					std::replace(path.begin(), path.end(), '\\', '/');
					if (path.back() != '/') path.push_back('/');
				}
			};

		fix_path(cfg::audio_out_dir);
		fix_path(cfg::image_out_dir);
	}

	sc_upload post(argv[1]);
	if (post.f.error_occured || !post.download()) 
		return 4;

	std::cout << "\n[!] DOWNLOAD(s) COMPLETE\n";
	return 0;
}