#pragma once

std::string GetJson(const std::string track, const std::string& CID);

bool DownloadTrack(const nlohmann::json& data, Cfg& cfg);

bool DownloadPlaylist(const nlohmann::json& data, Cfg& cfg);