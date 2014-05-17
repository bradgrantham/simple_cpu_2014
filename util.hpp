#include <cstring>
#include <boost/cstdint.hpp> // boost 1.55, -I/opt/include/local

struct memory {
    virtual void w(uint32_t byteaddr, uint32_t data) = 0;
};

struct test_memory : public memory {
    static const int memdwords = 2048;
    uint32_t memory[memdwords];

    test_memory()
    {
        memset(memory, 0, sizeof(memory));
    }

    virtual void w(uint32_t byteaddr, uint32_t data)
    {
        // XXX only handles 4-align
        if(byteaddr / 4 < memdwords)
            memory[byteaddr / 4] = data;
    }
};

void write_vectors(memory& m, uint32_t reset);
