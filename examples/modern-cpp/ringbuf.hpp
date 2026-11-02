#ifndef RINGBUF_HPP
#define RINGBUF_HPP

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

/* The ring buffer of lib/ as a class template. The storage is a member, so the object lives
   wherever the caller puts it: in .bss, on the stack or inside another object. Nothing uses the
   heap. */
template <typename T, std::size_t N> class RingBuffer
{
    static_assert(N > 0, "a ring buffer needs at least one slot");

  public:
    /* Returns true when the oldest sample was overwritten. */
    constexpr bool push(T sample) noexcept
    {
        const bool overwrote = count_ == N;
        data_[head_] = sample;
        head_ = (head_ + 1u) % N;
        if (!overwrote) {
            ++count_;
        }
        return overwrote;
    }

    [[nodiscard]] constexpr std::size_t count() const noexcept
    {
        return count_;
    }

    /* Index 0 is the oldest sample. The result is empty when the index is out of range. */
    [[nodiscard]] constexpr std::optional<T> get(std::size_t index) const noexcept
    {
        if (index >= count_) {
            return std::nullopt;
        }
        const std::size_t oldest = (head_ + N - count_) % N;
        return data_[(oldest + index) % N];
    }

  private:
    std::array<T, N> data_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
};

#endif
