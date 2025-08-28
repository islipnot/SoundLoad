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

static bool GetJsonValue(const Json& json, const char* name, std::string& buffer)
{
	if (!json.contains(name) || json[name].is_null())
	{
		return false;
	}

	buffer = json.value(name, std::string{});
	return true;
}

void Track::HandleTagArt(TagLib::ID3v2::Tag* tag) const
{
	std::string& CoverSrc = cfg::CoverSrc;

	if (CoverSrc.empty())
	{
		if (!CoverUrl.empty()) CoverSrc = CoverUrl;
		else if (!ArtistPfpUrl.empty()) CoverSrc = ArtistPfpUrl;
	}
	if (CoverSrc.empty())
	{
		std::cout << "WARNING: no art source found\n";
		return;
	}

	std::cout << "\nArt source: " << CoverSrc << '\n';

	auto cover = new TagLib::ID3v2::AttachedPictureFrame;

	if (cfg::flags & cfg::ArtSrcPath)
	{
		std::ifstream ArtFile(CoverSrc, std::ios::binary);
		if (ArtFile.fail())
		{
			std::cout << "WARNING: Failed to open cover art file\n";
			return;
		}

		const size_t sz = std::filesystem::file_size(CoverSrc);
		std::vector<char> buffer(sz);

		ArtFile.read(buffer.data(), sz);
		ArtFile.close();

		cover->setPicture(TagLib::ByteVector(buffer.data(), static_cast<UINT>(sz)));
	}
	else
	{
		if (cfg::flags & cfg::ArtSrcScLink)
		{
			const Track ArtTrack(CoverSrc, true);
			if (ArtTrack.CoverUrl.empty())
			{
				std::cout << "WARNING: failed to get art from source\n";
				return;
			}

			CoverSrc = ArtTrack.CoverUrl;
		}

		if (!(cfg::flags & cfg::ArtSrcImgLink)) CoverSrc = std::regex_replace(CoverSrc, std::regex("-large."), "-original.");

		const cpr::Response r = cpr::Get(cpr::Url{ CoverSrc });

		if (RequestFail(r)) FetchErr(r);
		else cover->setPicture(TagLib::ByteVector(r.text.data(), static_cast<UINT>(r.text.size())));
	}

	tag->addFrame(cover);
}

void Track::AddTag(const std::string& path) const
{
	TagLib::MPEG::File file(path.c_str());
	TagLib::ID3v2::Tag* tag = file.ID3v2Tag(true);

	// Title

	std::string value = cfg::TrackTitle.empty() ? title : cfg::TrackTitle;
	tag->setTitle(ToUtf8(value));

	// Album

	if (!cfg::album.empty()) value = cfg::album;
	else if (!cfg::TrackTitle.empty()) value = cfg::TrackTitle;

	tag->setAlbum(ToUtf8(value));

	// Artist

	tag->setArtist(ToUtf8((cfg::cArtist.empty() ? artist : cfg::cArtist)));

	// Comments

	value = "Release date: " + CreatedAt + "\n\nSaved with https://github.com/islipnot/SoundLoad";
	if (!description.empty()) value.insert(0, "Description: \"" + description + "\"\n\n");

	tag->setComment(ToUtf8(value));

	// Genre

	if (!cfg::genre.empty()) tag->setGenre(ToUtf8(cfg::genre));
	else if (!genre.empty()) tag->setGenre(ToUtf8(genre));

	// Track number

	if (cfg::TrackNumber != -1) tag->setTrack(cfg::TrackNumber);

	// Year

	if (cfg::year != -1) tag->setYear(cfg::year);
	else tag->setYear(std::stoi(CreatedAt.substr(0, 4)));

	// MP3 cover

	HandleTagArt(tag);

	file.save();
}

bool Album::GetTrackIDs(const Json& json)
{
	constexpr const char* tracks = "tracks";

	if (!json.contains(tracks))
	{
		std::cerr << "ERROR: 'tracks' property not found\n";
		return false;
	}

	const auto* track = &json[tracks][0];

	for (int i = json[tracks].size() - 1; i; --i, ++track)
	{
		ids.push_back(track->value("id", 0));
	}

	return true;
}

bool Track::GetStreamingUrl(const Json& json)
{
	constexpr auto media = "media";
	constexpr auto transcodings = "transcodings";

	if (!json.contains(media))
	{
		std::cerr << "ERROR: 'media' property not found\n";
		return false;
	}

	if (!json[media].contains(transcodings))
	{
		std::cerr << "ERROR: 'transcodings' property not found\n";
		return false;
	}

	bool FoundLink = false;

	for (const auto& transcoding : json[media][transcodings])
	{
		const auto format = transcoding["format"];
		hls = format["protocol"] == "hls";

		if ((format["protocol"] == "progressive") || (hls && format["mime_type"] == "audio/mpeg"))
		{
			UrlData = transcoding["url"].get<std::string>() + "?client_id=" + cfg::cid;

			FoundLink = true;

			if (!hls) break;
		}
	}

	if (!FoundLink)
	{
		std::cerr << "ERROR: no valid transcodings located\n";
		return false;
	}

	std::cout << "\nTranscoding URL: " << UrlData << '\n';
	return true;
}

bool Track::DownloadTrack() const
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

	std::cout << "\nStreaming URL: " << url << '\n';

	// Sanitizing MP3 output path

	std::string path = cfg::TrackName.empty() ? title : cfg::TrackName;
	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg::TrackDir.empty()) path.insert(0, cfg::TrackDir);
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
	UrlData = "https://api-v2.soundcloud.com/tracks?ids=";
	const std::string CidSegment = "&client_id=" + cfg::cid;

	// When creating a track ID resolution request, the max amount
	// of track ID's allowed is 50

	int TrackIndex = 1;
	const bool NoCoverSrc = cfg::CoverSrc.empty();

	for (size_t segment = ids.size() / 50; segment; --segment)
	{
		const size_t sz = segment > 0 ? 50 : ids.size();

		for (size_t i = sz; i > 0; --i)
		{
			UrlData += std::to_string(ids[i]) + ',';
		}

		ids.erase(ids.begin(), ids.begin() + sz);

		UrlData.pop_back(); // removing final comma
		UrlData += CidSegment;

		// Requesting an array of resolved tracks from album

		const cpr::Response r = cpr::Get(cpr::Url{ UrlData });
		if (RequestFail(r))
		{
			FetchErr(r);
			return false;
		}

		std::cout << "\nResolution URL: " << UrlData << '\n';

		// Downloading tracks

		const Json tracks = Json::parse(r.text);

		for (int i = static_cast<int>(tracks.size()) - 1; i >= 0; --i)
		{
			const std::string permalink = tracks[i].value("permalink_url", std::string{});
			if (permalink.empty())
			{
				std::cout << "WARNING: 'permalink_url' is null (skipping track)\n";
				continue;
			}

			Track track(permalink);

			cfg::TrackNumber = TrackIndex;
			++TrackIndex;

			std::string& CoverSrc = cfg::CoverSrc;
			if (NoCoverSrc)
			{
				if (!track.CoverUrl.empty()) CoverSrc = track.CoverUrl;
				else if (!track.ArtistPfpUrl.empty()) CoverSrc = track.ArtistPfpUrl;
			}

			if (!track.DownloadTrack()) std::cout << "WARNING: failed to download track (skipping)\n";

			if (NoCoverSrc) cfg::CoverSrc.clear();
		}

		UrlData.erase(41); // erasing all ids
	}

	return true;
}

bool ScPost::DownloadCover() const
{
	const cpr::Response r = cpr::Get(cpr::Url{ CoverUrl });
	if (RequestFail(r))
	{
		FetchErr(r);
		return false;
	}

	std::cout << "\nArtwork URL: " << CoverUrl << '\n';

	std::string path = cfg::ArtName.empty() ? title : cfg::ArtName;
	path = std::regex_replace(path, std::regex("[<>:\"/\\|?*]"), "_");

	if (!cfg::ArtDir.empty()) path.insert(0, cfg::ArtDir);
	path += ".jpg";

	std::ofstream cover(path, std::ios::binary | std::ios::trunc);
	cover.write(r.text.data(), r.text.size());
	cover.close();

	return true;
}

Track::Track(std::string url, bool CoverOnly)
{
	// Erasing link tracking (if present)

	if (url.find('?') != std::string::npos)
	{
		url.erase(url.find_first_of('?'));
	}

	// Requesting track data

	const std::string ResUrl = "https://api-v2.soundcloud.com/resolve?url=" + url + "&client_id=" + cfg::cid;
	const cpr::Response r = cpr::Get(cpr::Url{ ResUrl });

	if (RequestFail(r))
	{
		FetchErr(r);
		flags |= Error;
		return;
	}

	std::cout << "\nResolution URL: " << ResUrl << '\n';

	// Setting member var values

	const Json json = Json::parse(r.text);

	const auto& ParsedUrl = json.value("artwork_url", json["user"].value("avatar_url", std::string{}));
	if (!ParsedUrl.empty()) CoverUrl = std::regex_replace(ParsedUrl, std::regex("-large."), "-original.");

	if (CoverOnly) return;

	GetJsonValue(json, "title", title);

	if (cfg::flags & cfg::DontGetAudio) return;

	GetJsonValue(json, "description", description);
	GetJsonValue(json, "created_at",  CreatedAt);
	GetJsonValue(json, "genre",       genre);
	GetJsonValue(json, "tag_list",    tags);

	constexpr auto kind = "kind";

	if (json.contains(kind))
	{
		if (std::string(json[kind])[0] == 't')
		{
			id = json.value("id", 0);
			type = tTrack;
		}
		else type = tAlbum;
	}

	// Getting track artist

	constexpr auto publisher_metadata = "publisher_metadata";
	constexpr auto user = "user";

	if (json.contains(publisher_metadata) && !json[publisher_metadata].is_null())
	{
		artist = json[publisher_metadata].value("artist", std::string{});
	}
	else if (json.contains(user) && !json[user].is_null())
	{
		artist = json[user].value("username", std::string{});
	}
	else
	{
		std::cerr << "ERROR: failed to get artist property (Track::Track)\n";
		flags |= Error;
		return;
	}

	if ((type == tTrack && !GetStreamingUrl(json)) || (type == tAlbum && !GetTrackIDs(json)))
	{
		flags |= Error;
		return;
	}
}