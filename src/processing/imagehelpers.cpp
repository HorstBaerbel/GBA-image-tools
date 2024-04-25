#include "imagehelpers.h"

#include "color/colorhelpers.h"
#include "datahelpers.h"
#include "exception.h"

namespace ImageHelpers
{

    std::vector<uint8_t> convertDataTo1Bit(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> result;
        // size must be a multiple of 8
        const auto nrOfIndices = data.size();
        REQUIRE((nrOfIndices & 7) == 0, std::runtime_error, "Number of indices must be divisible by 8");
        for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 8)
        {
            REQUIRE(data[i] <= 1 && data[i + 1] <= 1 && data[i + 2] <= 1 && data[i + 3] <= 1 && data[i + 4] <= 1 && data[i + 5] <= 1 && data[i + 6] <= 1 && data[i + 7] <= 1, std::runtime_error, "Index values must be < 2");
            uint8_t v = (((data[i + 7]) & 0x01) << 7);
            v |= (((data[i + 6]) & 0x01) << 6);
            v |= (((data[i + 5]) & 0x01) << 5);
            v |= (((data[i + 4]) & 0x01) << 4);
            v |= (((data[i + 3]) & 0x01) << 3);
            v |= (((data[i + 2]) & 0x01) << 2);
            v |= (((data[i + 1]) & 0x01) << 1);
            v |= ((data[i]) & 0x01);
            result.push_back(v);
        }
        return result;
    }

    std::vector<uint8_t> convertDataTo2Bit(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> result;
        // size must be a multiple of 4
        const auto nrOfIndices = data.size();
        REQUIRE((nrOfIndices & 3) == 0, std::runtime_error, "Number of indices must be divisible by 4");
        for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 4)
        {
            REQUIRE(data[i] <= 3 && data[i + 1] <= 3 && data[i + 2] <= 3 && data[i + 3] <= 3, std::runtime_error, "Index values must be < 4");
            uint8_t v = (((data[i + 3]) & 0x03) << 6);
            v |= (((data[i + 2]) & 0x03) << 4);
            v |= (((data[i + 1]) & 0x03) << 2);
            v |= ((data[i]) & 0x03);
            result.push_back(v);
        }
        return result;
    }

    std::vector<uint8_t> convertDataTo4Bit(const std::vector<uint8_t> &data)
    {
        std::vector<uint8_t> result;
        // size must be a multiple of 2
        const auto nrOfIndices = data.size();
        REQUIRE((nrOfIndices & 1) == 0, std::runtime_error, "Number of indices must be even");
        for (std::remove_const<decltype(nrOfIndices)>::type i = 0; i < nrOfIndices; i += 2)
        {
            REQUIRE(data[i] <= 15 && data[i + 1] <= 15, std::runtime_error, "Index values must be < 16");
            uint8_t v = (((data[i + 1]) & 0x0F) << 4);
            v |= ((data[i]) & 0x0F);
            result.push_back(v);
        }
        return result;
    }

    std::vector<uint8_t> incValuesBy1(const std::vector<uint8_t> &data)
    {
        auto tempData = data;
        std::for_each(tempData.begin(), tempData.end(), [](auto &index)
                      { REQUIRE(index < 255, std::runtime_error, "Indices must be < 255"); index++; });
        return tempData;
    }

    std::vector<uint8_t> swapValueWith0(const std::vector<uint8_t> &data, uint8_t value)
    {
        auto tempData = data;
        for (size_t i = 0; i < tempData.size(); ++i)
        {
            if (tempData[i] == value)
            {
                tempData[i] = 0;
            }
            else if (tempData[i] == 0)
            {
                tempData[i] = value;
            }
        }
        return tempData;
    }

    std::vector<uint8_t> swapValues(const std::vector<uint8_t> &data, const std::vector<uint8_t> &newValues)
    {
        if (data.empty() || newValues.empty())
        {
            return data;
        }
        // find max value in data and make sure the new values table has an entry for it
        auto maxValue = *std::max_element(data.cbegin(), data.cend());
        REQUIRE(newValues.size() > maxValue, std::runtime_error, "Size of new values table must be >= max. value in data");
        // reverse new value table
        /*std::vector<uint8_t> reverseValues(newValues.size(), 0);
        for (uint32_t i = 0; i < newValues.size(); i++)
        {
            reverseValues[newValues[i]] = i;
        }*/
        // apply mapping to data
        auto tempData = data;
        std::for_each(tempData.begin(), tempData.end(), [&newValues](auto &i)
                      { i = newValues[i]; });
        return tempData;
    }

}
