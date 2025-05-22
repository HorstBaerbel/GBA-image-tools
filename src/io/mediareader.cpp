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
}