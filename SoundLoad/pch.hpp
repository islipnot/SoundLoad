#pragma once

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <regex>
#include <algorithm>

#include <Windows.h>

#include "TagLib/attachedpictureframe.h"
#include "TagLib/id3v2tag.h"
#include "TagLib/mpegfile.h"
#include "TagLib/fileref.h"

#include "cpr.h"

#include "json.hpp"

// Debugging macros

#define FetchErr(response) std::cerr << "REQUEST FAILED: " << response.url << " (" << response.error.message << " )\n" // response must be a cpr::Response object

#define RequestFail(response) response.status_code != 200 // response must be a cpr::Response object

#ifdef _DEBUG
	#define DBG_MSG(msg) std::cout << msg << '\n'
#else
	#define DBG_MSG(output)
#endif

/* TODO
- download just the cover of a song
- cover retrieval paths
- expand config usage
*/