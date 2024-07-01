// Include additional libraries if necessary
#include <vector>

// Define an allpass filter class
class AllpassDelay
{
  public:
    void  Init(float delay, float feedback);
    float Process(float input);

  private:
    std::vector<float> buffer_;
    int                buf_size_;
    int                write_pos_;
    int                read_pos_;
    float              feedback_;
};

void AllpassDelay::Init(float delay, float feedback)
{
    buf_size_ = static_cast<int>(delay * sample_rate_);
    buffer_.resize(buf_size_, 0.0f);
    write_pos_ = 0;
    read_pos_  = 0;
    feedback_  = feedback;
}

float AllpassDelay::Process(float input)
{
    float output        = buffer_[read_pos_] - input * feedback_;
    buffer_[write_pos_] = input + output * feedback_;

    // Update positions
    if(++write_pos_ >= buf_size_)
        write_pos_ = 0;
    if(++read_pos_ >= buf_size_)
        read_pos_ = 0;

    return output;
}
