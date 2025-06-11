#include "pch.hpp"
#include "cfg.hpp"
#include "ScApi.hpp"

/* TODO
*
* make -clear & -fresh work
*/

using Json = nlohmann::json;

int main(int argc, char* argv[])
{
    Config cfg(argc, argv);
    if (cfg.fail()) return 1;

    // Reading and saving cfg.json

    std::wstring CfgPath = cfg.ExeDir + L"cfg.json";
    const bool CfgExists = std::filesystem::exists(CfgPath);

    if (cfg.flags & Config::AddPVar || !CfgExists)
    {
        cfg.AddPathVar();
    }

    int& flags = cfg.flags;
    const bool SkipCfg = flags & Config::SkipCfg;
    CfgFormat CfgData;

    if (CfgExists)
    {
        Json JsonCfg;
        std::ifstream CfgFile(CfgPath, std::ios::in);

        CfgFile >> JsonCfg;
        CfgFile.close();

        cfg.read(JsonCfg, CfgData, SkipCfg);
    }

    if (!SkipCfg && flags & Config::SaveCfg) cfg.save(CfgData, CfgPath);

    CfgPath.clear();

    if ((flags & Config::NoLink) == Config::NoLink)
    {
        std::cout << "\n[!] INPUT HANDLED\n";
        return 0;
    }

    // Finishing config initialization

    if (!(flags & (Config::NoAudio | Config::NoCover)))
    {
        if (cfg.cid.empty())
        {
            std::cerr << "ERROR: no client ID\n";
            return 1;
        }

        const std::string& TrackName = cfg.TrackName;

        if (!TrackName.empty())
        {
            if (cfg.title.empty()) cfg.title = TrackName;
            if (cfg.album.empty()) cfg.album = TrackName;
            if (cfg.CoverName.empty()) cfg.CoverName = TrackName;
        }
    }

    std::string& TrackDst = cfg.TrackDst;
    std::string& CoverDst = cfg.CoverDst;

    std::replace(TrackDst.begin(), TrackDst.end(), '\\', '/');
    std::replace(CoverDst.begin(), CoverDst.end(), '\\', '/');

    if (!TrackDst.empty() && TrackDst.back() != '/') TrackDst.push_back('/');
    if (!CoverDst.empty() && CoverDst.back() != '/') CoverDst.push_back('/');

    // Downloading track/cover

    ScPost post(argv[1], &cfg);
    if (post.fail() || !post.download()) return 1;

    std::cout << "\n[!] DOWNLOAD COMPLETE\n";

    return 0;
}