#ifndef __CONFIGURATION_H__
#define __CONFIGURATION_H__

// Adjust the CPU clock frequency here (note: overclocking doesn't help - keep at 133 MHz)
#define CPU_FREQ_MHZ 133
#define CPU_FREQ_KHZ (CPU_FREQ_MHZ * 1000)

// Adjust the minimum time between each edge here (in tests, timing has a variance of -0/+40 ns)
// - 300 ns should achieve a max throughput of 2 mbps but average throughput around 1.7 mbps
// - The Dreamcast achieves an average throughput of about 2 mbps by clocking more efficiently
#define MIN_CLOCK_PERIOD_NS 300
#define CPU_TICKS_PER_PERIOD (int)(MIN_CLOCK_PERIOD_NS * CPU_FREQ_MHZ / 1000.0 + 0.5)

#endif // __CONFIGURATION_H__
