#pragma once

#define WIN32_LEAN_AND_MEAN

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <regex>
#include <algorithm>

#include <Windows.h>

#include "TagLib/attachedpictureframe.h"
#include "TagLib/id3v2tag.h"
#include "TagLib/mpegfile.h"
#include "TagLib/fileref.h"

#include "cpr/cpr.h"

#include "nlohmann/json.hpp"