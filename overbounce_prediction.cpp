#include "overbounce_prediction.hpp"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <vector>


namespace overbounce_prediction {

CumulativeFrametimeStats::CumulativeFrametimeStats() {
    setConsecutiveFrameHorizon(500);
    setConsecutiveTimeHorizon(4000);
}

void CumulativeFrametimeStats::setConsecutiveFrameHorizon(const std::size_t frame_count)
{
    const std::size_t index_previous = m_frametime_partial_sums.empty() ? 0 : (m_frametime_partial_sums.size() - 1);
    const std::size_t default_frametime = 8;
    m_frametime_partial_sums.resize(frame_count + 1, default_frametime);
    std::partial_sum(m_frametime_partial_sums.begin() + index_previous,
                     m_frametime_partial_sums.end(),
                     m_frametime_partial_sums.begin() + index_previous);
}

void CumulativeFrametimeStats::setConsecutiveTimeHorizon(const std::size_t msec)
{
    m_cumulative_frametime_frequency.resize(msec + 1);
}

void CumulativeFrametimeStats::setUpdateFactor(const double update_factor)
{
    m_update_factor = update_factor;
}

void CumulativeFrametimeStats::reportLastFrametime(const int last_frametime)
{
    m_num_reported += 1;

    // add last_frametime to every entry and move over by one to the right.
    // do this backwards to avoid overwriting the next entries that we still
    // need to update.
    std::transform(m_frametime_partial_sums.crbegin() + 1,
                   m_frametime_partial_sums.crend(),
                   m_frametime_partial_sums.rbegin(),
                   [last_frametime](const int cumulative_frametime) {
                       return cumulative_frametime + last_frametime;
                   });
    assert(!m_frametime_partial_sums.empty());
    m_frametime_partial_sums.front() = last_frametime;

    const double update_factor = std::max(1.0 / m_num_reported, m_update_factor);
    std::transform(m_cumulative_frametime_frequency.cbegin(),
                   m_cumulative_frametime_frequency.cend(),
                   m_cumulative_frametime_frequency.begin(),
                   [update_factor](const double value) {
                       return (1.0 - update_factor) * value;
                   });

    assert(std::is_sorted(m_frametime_partial_sums.cbegin(),
                          m_frametime_partial_sums.cend()));
    for (const int cumulative_frametime : m_frametime_partial_sums) {
        assert(cumulative_frametime >= 0);
        if (cumulative_frametime >= m_cumulative_frametime_frequency.size()) {
            return;
        }
        m_cumulative_frametime_frequency[cumulative_frametime] += update_factor;
    }
}

const std::vector<double>& CumulativeFrametimeStats::relativeFrequencies() const
{
    return m_cumulative_frametime_frequency;
}

static double square(const double value)
{
    return value * value;
}

double computeProbability(
    const std::vector<double>& cumulative_frametime_probabilities,
    const double height_difference, const double velocity, const double gravity)
{
    const double t = velocity / gravity + std::sqrt(square(velocity / gravity) + 2.0 * height_difference / gravity);
    const auto msec_hit = static_cast<std::size_t>(t * 1000.0);
    if (msec_hit >= cumulative_frametime_probabilities.size())
    {
        return 0.0;
    }

    const double t_hit = msec_hit / 1000.0;
    const double hit_height_difference = -(velocity * t_hit - gravity / 2.0 * square(t_hit));

    if (hit_height_difference >= height_difference) {
        // exactly on the edge: have to reduce the probability by how often
        // due to floating point errors we will go past the height difference
        // TODO: think about the actual reduction instead of just assuming half.
        return cumulative_frametime_probabilities[msec_hit] / 2.0;
    }

    assert(hit_height_difference <= std::nextafter(height_difference, std::numeric_limits<double>::infinity()));
    if (hit_height_difference > height_difference - 0.25)
    {
        return cumulative_frametime_probabilities[msec_hit];
    }
    return 0.0;
}

OverbounceLevel closestOverbounceLevel(
    const std::vector<double>& cumulative_frametime_probabilities,
    double height_difference, double velocity, double gravity)
{
    const double t = velocity / gravity + std::sqrt(square(velocity / gravity) + 2.0 * height_difference / gravity);
    const auto msec_hit = static_cast<std::size_t>(t * 1000.0);
    if (msec_hit >= cumulative_frametime_probabilities.size())
    {
        return {height_difference + 0.25, height_difference, 0.0};
    }

    const double t_hit = msec_hit / 1000.0;
    const double hit_height_difference = -(velocity * t_hit - gravity / 2.0 * square(t_hit));

    return {hit_height_difference + 0.25, hit_height_difference, cumulative_frametime_probabilities[msec_hit]};
}

}  // namespace overbounce_prediction
