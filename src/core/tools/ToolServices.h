#pragma once

#include "platform/ArchiveExtractor.h"
#include "platform/FileOps.h"
#include "platform/Hasher.h"
#include "platform/HttpClient.h"

namespace westie::core {

struct ToolServices {
    platform::FileOps& files;
    platform::HttpClient& http;
    const platform::Hasher& hasher;
    const platform::ArchiveExtractor& archives;
};

}
