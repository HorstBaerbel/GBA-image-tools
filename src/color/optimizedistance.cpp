#include "optimizedistance.h"

namespace Color
{

    auto calculateDistanceRMS(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap) -> float
    {
        float sumOfSquares = 0.0F;
        for (uint32_t i = 0; i < (indices.size() - 1); i++)
        {
            sumOfSquares += distancesSqrMap.at(indices.at(i)).at(indices.at(i + 1));
        }
        return std::sqrt(sumOfSquares / indices.size());
    }

    auto insertIndexOptimal(const std::vector<uint8_t> &indices, const std::map<uint8_t, std::vector<float>> &distancesSqrMap, uint8_t indexToInsert) -> std::vector<uint8_t>
    {
        float bestDistance = std::numeric_limits<float>::max();
        std::vector<uint8_t> bestOrder = indices;
        // insert index at all possible positions and calculate distance of vector
        for (uint32_t i = 0; i <= indices.size(); i++)
        {
            std::vector<uint8_t> candidate = indices;
            candidate.insert(std::next(candidate.begin(), i), indexToInsert);
            auto candidateDistance = calculateDistanceRMS(candidate, distancesSqrMap);
            if (candidateDistance < bestDistance)
            {
                bestDistance = candidateDistance;
                bestOrder = candidate;
            }
        }
        return bestOrder;
    }

}
