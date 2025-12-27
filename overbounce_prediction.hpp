#pragma once

#include <cstddef>
#include <vector>

namespace overbounce_prediction {

class CumulativeFrametimeStats {
public:
    explicit CumulativeFrametimeStats();

    /**
     * Sets the number of consecutive frames that can be tracked.
     */
    void setConsecutiveFrameHorizon(std::size_t frame_count);

    /**
     * Sets the total frametime duration of consecutive frames that can be tracked.
     */
    void setConsecutiveTimeHorizon(std::size_t msec);

    /**
     * Sets how quickly the relative frequencies respond to new frametime patterns.
     *
     * 1.0 means to take the most recent frametimes immediately and forget all
     * other history, 0.0 means never update. Default is 0.001.
     */
    void setUpdateFactor(double update_factor);

    /**
     * Adds the frametime of a frame to the statistics.
     */
    void reportLastFrametime(int frametime);

    /**
     * Returns the observed relative frequency of each cumulative frametime in recent history.
     *
     * The value [i] of the returned vector is a measure of how often (relative)
     * a large enough consecutive range of frames had a total frametime of
     * exactly i milliseconds in recent history.
     */
    const std::vector<double>& relativeFrequencies() const;

private:
    /**
     * Stores the partial sums of the last n frametimes.
     *
     * Say we are currently at physics frame i, then:
     * The value at [0] is the frametime of frame i.
     * The value at [1] is the sum of the frametimes of frame i and frame i-1.
     * The value at [2] is the sum of the frametimes of frame i, frame i-1 and frame i-2.
     * And so on.
     */
    std::vector<int> m_frametime_partial_sums;

    std::vector<double> m_cumulative_frametime_frequency;
    std::size_t m_num_reported = 0;
    double m_update_factor = 0.001;
};

double computeProbability(const std::vector<double>& cumulative_frametime_probabilities,
                          double height_difference, double velocity, double gravity);

struct OverbounceLevel {
    double max_height_difference;
    double min_height_difference;
    double probability;
};

OverbounceLevel closestOverbounceLevel(
    const std::vector<double>& cumulative_frametime_probabilities,
    double height_difference, double velocity, double gravity
);

}  // namespace overbounce_prediction
