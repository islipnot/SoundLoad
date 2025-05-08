#pragma once

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <unordered_map>
#include <regex>

#include <Windows.h>

#include "textidentificationframe.h"
#include "attachedpictureframe.h"
#include "id3v2framefactory.h"
#include "id3v2tag.h"
#include "mpegfile.h"
#include "fileref.h"
#include "tag.h"

#include "cpr.h"

#include "json.hpp"

#ifdef _DEBUG
	#define DBG_MSG(msg) std::cout << msg << '\n'
#else
	#define DBG_MSG(output)
#endif

/* TODO
- fix filesystem errors when ppl include things like stars in track names
- global environment var
- sharex 
- option to download just the cover of a song
*/