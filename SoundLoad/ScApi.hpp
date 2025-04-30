#pragma once

std::string GetRawJson(const std::string track, const std::string& CID);

bool DownloadTrack(const nlohmann::json data, Cfg& cfg);