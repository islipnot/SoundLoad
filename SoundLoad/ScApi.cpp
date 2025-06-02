#include "pch.hpp"
#include "cfg.hpp"
#include "ScApi.hpp"

using Json = nlohmann::json;

static bool DownloadM3U(const std::string& m3u, std::string& buffer)
{
	// Parse M3U -> download MP3 segments -> concat

	std::istringstream stream(m3u);
	std::string line;

	while (std::getline(stream, line))
	{
		if (line.starts_with("#EXTINF"))
		{
			std::getline(stream, line); // Skipping to next line, which is the URL

			const cpr::Response r = cpr::Get(cpr::Url{ line });
			if (RequestFail(r))
			{
				FetchErr(r);
				return false;
			}

			buffer += r.text;
		}
	}

	return true;
}

void Track::HandleTagArt(TagLib::ID3v2::Tag* tag)
{
	std::string& CoverSrc = cfg->CoverSrc;

	if (CoverSrc.empty())
	{
		if (!CoverUrl.empty()) CoverSrc = CoverUrl;
		else if (!ArtistPfpUrl.empty()) CoverSrc = ArtistPfpUrl;
	}
	if (CoverSrc.empty())
	{
		std::cout << "ERROR: FAILED TO FIND COVER SOURCE (ignoring)\n";
		return;
	}

	std::cout << "Artwork source: " << CoverSrc << "\n\n";

	const int CfgFlags = cfg->flags;
	auto cover = new TagLib::ID3v2::AttachedPictureFrame;

	if (CfgFlags & Config::tPath)
	{
		std::ifstream ArtFile(CoverSrc, std::ios::binary);
		if (ArtFile.fail())
		{
			std::cout << "ERROR: Failed to open cover art file (ignoring)\n";
			return;
		}

		const size_t sz = std::filesystem::file_size(CoverSrc);
		std::vector<char> buffer(sz);

		ArtFile.read(buffer.data(), sz);
		ArtFile.close();

		cover->setPicture(TagLib::ByteVector(buffer.data(), sz));
	}
	else
	{
		if (CfgFlags & Config::tScLink)
		{
			const Track ArtTrack(CoverSrc, cfg, true);
			if (ArtTrack.CoverUrl.empty())
			{
				std::cout << "ERROR: FAILED TO FETCH ART FROM SOURCE (ignoring)\n";
				return;
			}

			CoverSrc = ArtTrack.CoverUrl;
		}

		if (!(CfgFlags & Config::tImgLink)) CoverSrc = std::regex_replace(CoverSrc, std::regex("-large."), "-original.");

		const cpr::Response r = cpr::Get(cpr::Url{ CoverSrc });

		if (RequestFail(r)) FetchErr(r);
		else cover->setPicture(TagLib::ByteVector(r.text.data(), static_cast<UINT>(r.text.size())));
	}

	tag->addFrame(cover);
}

void Track::AddTag(const std::string& path)
{
	TagLib::MPEG::File file(path.c_str());
	TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);

	// Title

	std::string value = cfg->title.empty() ? title : cfg->title;
	tag->setTitle(ToUtf8(value));

	// Album

	if (!cfg->album.empty()) value = cfg->album;
	else if (!cfg->title.empty()) value = cfg->title;

	tag->setAlbum(ToUtf8(value));

	// Artist

	tag->setArtist(ToUtf8((cfg->cArtists.empty() ? artist : cfg->cArtists)));

	// Comments

	value = "Release date: " + CreatedAt + "\n\nSaved with https://github.com/islipnot/SoundLoad";
	if (!description.empty()) value.insert(0, "Description: \"" + description + "\"\n\n");

	tag->setComment(ToUtf8(value));

	// Genre

	if (!cfg->genre.empty()) tag->setGenre(ToUtf8(cfg->genre));
	else if (!genre.empty()) tag->setGenre(ToUtf8(genre));

	// Track number

	if (cfg->tNum != -1) tag->setTrack(cfg->tNum);

	// Year

	if (cfg->year != -1) tag->setYear(cfg->year);
	else tag->setYear(std::stoi(CreatedAt.substr(0, 4)));

	// MP3 cover

	HandleTagArt(tag);

	file.save();
}

bool Album::GetTrackIDs(const Json& json)
{
	constexpr auto tracks = "tracks";

	if (!json.contains(tracks))
	{
		std::cerr << "ERROR: 'tracks' property not found\n";
		return false;
	}

	UrlData = "https://api-v2.soundcloud.com/tracks?ids=";

	for (const auto& track : json[tracks])
	{
		UrlData += std::to_string(track.value("id", 0)) + ',';
	}

	UrlData.pop_back(); // removing final comma
	UrlData += "&client_id=" + cfg->cid;

	return true;
}

bool Track::GetStreamingUrl(const Json& json)
{
	constexpr auto media = "media";
	constexpr auto transcodings = "transcodings";

	if (!json.contains(media))
	{
		std::cerr << "ERROR: media property not found\n";
		return false;
	}

	if (!json[media].contains(transcodings))
	{
		std::cerr << "ERROR: transcodings property not found\n";
		return false;
	}

	bool FoundLink = false;

	for (const auto& transcoding : json[media][transcodings])
	{
		const auto format = transcoding["format"];
		hls = format["protocol"] == "hls";

		if ((format["protocol"] == "progressive") || (hls && format["mime_type"] == "audio/mpeg"))
		{
			UrlData = transcoding["url"].get<std::string>() + "?client_id=" + cfg->cid;

			FoundLink = true;

			if (!hls) break;
		}
	}

	if (!FoundLink)
	{
		std::cerr << "ERROR: NO MEDIA TRANSCODING URL FOUND\n";
		return false;
	}

	std::cout << "Streaming URL: " << UrlData << "\n\n";
	return true;
}

bool Track::DownloadTrack()
{
	cpr::Response r = cpr::Get(cpr::Url{ UrlData });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	const std::string url = Json::parse(r.text)["url"];

	r = cpr::Get(cpr::Url{ url });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	std::cout << "Streaming URL: " << url << "\n\n";

	// Sanitizing MP3 output path

	std::string path = cfg->TrackName.empty() ? title : cfg->TrackName;
	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg->TrackDst.empty()) path.insert(0, cfg->TrackDst);
	path += ".mp3";

	// Downloading MP3(s) and writing output to disk

	char* pAudio;
	size_t sz;

	std::string HlsMp3;

	if (hls)
	{
		if (!DownloadM3U(r.text, HlsMp3)) return false;

		pAudio = HlsMp3.data();
		sz = HlsMp3.size();
	}
	else
	{
		pAudio = r.text.data();
		sz = r.text.size();
	}

	std::ofstream track(path, std::ios::binary | std::ios::trunc);
	track.write(pAudio, sz);
	track.close();

	AddTag(path);
	return true;
}

bool Album::DownloadAlbum()
{
	// Requesting an array of resolved tracks from album

	const std::string url = "https://api-v2.soundcloud.com/tracks?ids=" + UrlData + "&client_id=" + cfg->cid;

	const cpr::Response r = cpr::Get(cpr::Url{ url });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	// Downloading each track

	std::cout << "ID resolution URL: " << url << "\n\n";

	const Json tracks = Json::parse(r.text);

	for (int i = tracks.size(); i; --i)
	{
		const char* permalink = tracks[i].value("permalink_url", nullptr);
		if (!permalink)
		{
			std::cout << "ERROR: 'permalink_url' IS NULL (ignoring track)\n";
			continue;
		}

		Track track(permalink, cfg);
		track.cfg->tNum = i;

		if (!track.DownloadTrack()) std::cout << "ERROR: FAILED TO DOWNLOAD TRACK (ignoring)\n";
	}

	return true;
}

bool ScPost::DownloadCover()
{
	cpr::Response r = cpr::Get(cpr::Url{ CoverUrl });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	std::cout << "Artwork URL: " << CoverUrl << "\n\n";

	std::string path = cfg->CoverName.empty() ? title : cfg->CoverName;
	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg->CoverDst.empty()) path.insert(0, cfg->CoverDst);
	path += ".jpg";

	std::ofstream cover(path, std::ios::binary | std::ios::trunc);
	cover.write(r.text.data(), r.text.size());
	cover.close();

	return true;
}

Track::Track(std::string url, Config* pCfg, bool CoverOnly)
{
	cfg = pCfg;

	// Requesting track data

	if (url.find('?') != std::string::npos) // for when you use the copy link button rather than copying browser url
	{
		url.erase(url.find_first_of('?'));
	}

	const std::string ResUrl = "https://api-v2.soundcloud.com/resolve?url=" + url + "&client_id=" + cfg->cid;
	const cpr::Response r = cpr::Get(cpr::Url{ ResUrl });

	if (RequestFail(r))
	{
		FetchErr(r);
		cfg->flags |= Config::Error;
		return;
	}

	std::cout << "Resolution URL: " << ResUrl << "\n\n";

	// Setting member var values

	const Json json = Json::parse(r.text);

	constexpr auto ArtUrl = "artwork_url";
	constexpr auto large  = "-large.";
	constexpr auto og     = "-original.";

	if (json[ArtUrl].is_null())
	{
		const auto& PfpUrl = json["user"]["avatar_url"];
		if (!PfpUrl.is_null()) CoverUrl = std::regex_replace(PfpUrl.get<std::string>(), std::regex(large), og);
	}
	else CoverUrl = std::regex_replace(json[ArtUrl].get<std::string>(), std::regex(large), og);

	if (CoverOnly) return;

	title = json.value("title", std::string{});

	if (cfg->flags & Config::NoAudio) return;

	CreatedAt   = json.value("created_at",  std::string{});
	description = json.value("description", std::string{});
	genre       = json.value("genre",       std::string{});
	tags        = json.value("tag_list",    std::string{});

	constexpr auto kind = "kind";
	constexpr auto publisher_metadata = "publisher_metadata";

	if (json.contains(kind))
	{
		const char PostType = json[kind].get<std::string>()[0];

		if (PostType == 't')
		{
			id = json.value("id", 0);
			type = tTrack;
		}
		else type = tAlbum;
	}

	if (!json[publisher_metadata].is_null()) artist = json[publisher_metadata].value("artist", std::string{});
	else artist = json["user"]["username"].get<std::string>(); // using get() instead of value() cuz this will never be null

	if (type == tTrack)
	{
		if (!GetStreamingUrl(json))
		{
			flags |= Error;
			return;
		}
	}
	else if (!GetTrackIDs(json))
	{
		flags |= Error;
		return;
	}
}