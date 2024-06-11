#include "videoreader.h"

namespace Video
{

    Reader::~Reader()
    {
        close();
    }

    auto Reader::close() -> void
    {
    }
}