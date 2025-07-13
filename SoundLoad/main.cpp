#include "pch.hpp"
#include "cfg.hpp"
#include "ScApi.hpp"

using Json = nlohmann::json;

int main(int argc, char* argv[])
{
    // Parsing arguments

    if (argc < 2)
    {
        std::cerr << "ERROR: insufficient arguments\n";
        return 1;
    }

    if (!cfg::GetConfig(argc, argv))
    {
        return 2;
    }

    // Adding SoundLoad to PATH variables (if requested)

    std::wstring CfgPath = cfg::ExeDir + L"cfg.json";
    const bool CfgExists = std::filesystem::exists(CfgPath);

    if (cfg::flags & cfg::AddToPathVariables || !CfgExists)
    {
        cfg::AddToPath(); // AddToPathVariables bypasses MessageBox prompt
    }

    // Reading/writing to cfg.json

    {
        CfgFormat data;

        if (CfgExists)
        {
            Json JsonCfg;
            std::ifstream CfgFile(CfgPath);

            CfgFile >> JsonCfg;
            CfgFile.close();

            data = JsonCfg.get<CfgFormat>();
            cfg::ReadConfig(data);
        }

        if (cfg::flags & cfg::SaveToConfig)
        {
            cfg::SaveConfig(data, CfgPath);
        }

        CfgPath.clear();
    }

    if (cfg::flags & cfg::NoLinkProvided)
    {
        std::cout << "\n[!] INPUT HANDLED\n";
        return 0;
    }

	// Finishing cfg initialization

    if (cfg::cid.empty())
    {
		std::cerr << "ERROR: no client ID provided\n";
        return 3;
    }

    if (!cfg::TrackName.empty())
    {
        if (cfg::TrackTitle.empty()) cfg::TrackTitle = cfg::TrackName;
        if (cfg::album.empty()) cfg::album = cfg::TrackName;
        if (cfg::ArtName.empty()) cfg::ArtName = cfg::TrackName;
    }

    if (!cfg::TrackDir.empty())
    {
        std::replace(cfg::TrackDir.begin(), cfg::TrackDir.end(), '\\', '/');
        if (cfg::TrackDir.back() != '/') cfg::TrackDir.push_back('/');
    }

    if (!cfg::ArtDir.empty())
    {
        std::replace(cfg::ArtDir.begin(), cfg::ArtDir.end(), '\\', '/');
        if (cfg::ArtDir.back() != '/') cfg::ArtDir.push_back('/');
    }

    // Downloading track/art

    ScPost track(argv[1]);
    if (track.fail() || !track.download()) return 4;

    std::cout << "\n[!] DOWNLOAD COMPLETE\n";

    return 0;
}