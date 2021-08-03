#include <cinttypes>

using DatagramId = int8_t;

namespace Diag{
    namespace Ping{
        struct Request{
            static constexpr DatagramId Id = 10;
        };
        struct Response{
            static constexpr DatagramId Id = 11;
        };
    }
    
    namespace Echo{
        struct Request{
            static constexpr DatagramId Id = 12;
        };
        struct Response{
            static constexpr DatagramId Id = 13;
        };
    }
    
    namespace Mult{
        struct Request{
            static constexpr DatagramId Id = 14;
            int32_t operands[2];
        };
        struct Response{
            static constexpr DatagramId Id = 15;
            int32_t result;
        };
    }
}

