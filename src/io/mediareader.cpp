#include "mediareader.h"

namespace Media
{

    Reader::~Reader()
    {
        close();
    }

    auto Reader::close() -> void
    {
    }

    auto Reader::getMetaData() const -> std::vector<uint8_t>
    {
        return {};
    }
}