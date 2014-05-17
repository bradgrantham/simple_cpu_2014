#include "simple_cpu_2014.hpp"
#include "util.hpp"

void write_vectors(memory& m, uint32_t reset)
{
    using namespace simple_cpu_2014;

    m.w(0, JMP(reset));
    // XXX interrupt vectors, also
}

