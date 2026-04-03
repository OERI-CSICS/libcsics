#pragma once
#include <memory>
#include <optional>

#include "csics/queue/SPSCQueue.hpp"
#include "csics/radio/Common.hpp"

namespace csics::radio {

/** @brief Abstract base class for a radio receiver.
 * Abstracts over different radio hardware implementations.
 * Provides a common interface for opening the receiver.
 * Represents a single channel.
 * The returned queue from start_stream() will provide raw IQ samples.
 * The queue is owned by the RadioRx implementation and will be valid until the
 * radio is destroyed or start_stream() is called again.
 */
class IRadioRx {
   public:
    struct StartStatus;
    struct BlockHeader;

    virtual ~IRadioRx() = default;

    /**
     * @brief Starts the radio stream.
     * @param stream_config Configuration for the stream.
     * @return StartStatus indicating success or failure, and the queue for
     * receiving samples. Will invalidate any previously returned queue.
     */
    virtual StartStatus start_stream(
        const StreamConfiguration& stream_config) noexcept = 0;

    /**
     * @brief Stops the radio stream.
     *
     * Stops the radio stream if it is currently streaming.
     * If the stream is not active, this function has no effect.
     * If the stream is active, no more samples will be produced after this
     * call. The queue still remains valid until the RadioRx object is
     * destroyed.
     */
    virtual void stop_stream() noexcept = 0;

    virtual bool is_streaming() const noexcept = 0;

    virtual double get_sample_rate() const noexcept = 0;
    virtual Timestamp set_sample_rate(double rate) noexcept = 0;
    virtual double get_max_sample_rate() const noexcept = 0;

    virtual double get_center_frequency() const noexcept = 0;
    virtual Timestamp set_center_frequency(double freq) noexcept = 0;

    virtual double get_gain() const noexcept = 0;
    virtual Timestamp set_gain(double gain) noexcept = 0;

    virtual RadioConfiguration get_configuration() const noexcept = 0;
    virtual Timestamp set_configuration(
        const RadioConfiguration& config) noexcept = 0;
    virtual RadioDeviceInfo get_device_info() const noexcept = 0;

    [[maybe_unused]]
    static std::unique_ptr<IRadioRx> create_radio_rx(
        const RadioDeviceArgs& device_args, const RadioConfiguration& config);

    struct StartStatus {
        enum class Code {
            SUCCESS,
            HARDWARE_FAILURE,
            CONFIGURATION_ERROR,
        } code;
        std::optional<queue::SPSCQueue::ReadHandle> rx_handle;

        operator bool() const noexcept { return code == Code::SUCCESS; }
    };

    enum BlockFlags : uint32_t {
        BF_NONE = 0,
        BF_OVERFLOW =
            1 << 0,  // Indicates that samples were dropped due to overflow.
        BF_DISCONTINUITY =
            1 << 1,  // Indicates a discontinuity in the sample stream, such
                     // as a gap in timestamps or a reset in the hardware.
        BF_TIMEOUT = 1 << 2,  // Indicates that a timeout occurred while waiting
                           // for samples.
        BF_BAD_FRAME = 1 << 3,  // Indicates that the received samples contain errors
                          // or are corrupted.
        /* Control Plane Flags */
        BF_CONFIG_CHANGE =
            1 << 8,  // Indicates that a configuration change occurred during the
                     // reception of this block, which may affect the validity
                     // of the samples.
        BF_LO_UNLOCKED =
            1 << 9,  // Indicates that the local oscillator is unlocked, which
                     // may lead to frequency instability.
        BF_LO_UNKNOWN = 1 << 10,  // Indicates that the status of the local oscillator is
                              // unknown and cannot be determined.
    };

    struct BlockHeader {
        Timestamp timestamp_ns;  // Timestamp in nanoseconds since epoch.
                                 // Derived from system clock.
        Timestamp timestamp_hw;  // Timestamp from the hardware, if available.
                                 // If not available, will be set to the same
                                 // value as timestamp_ns.
        uint64_t num_samples;
        uint32_t flags;
        char reserved[252];  // reserved for future use, should be zeroed out by
                             // implementations
    };
};
};  // namespace csics::radio
