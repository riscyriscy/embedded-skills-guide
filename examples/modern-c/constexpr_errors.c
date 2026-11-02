int read_gain(void);

constexpr int limit = 4;
constexpr int gain = read_gain();

int main(void)
{
    limit = 5;
    return limit + gain;
}
