#pragma once
#include <cinttypes>
#include <ostream>

using DatagramId = uint8_t;

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

namespace Trans{
    namespace Echo{
         struct Request{
            static constexpr DatagramId Id = 12;
            uint32_t length;
            // char[] message
        };
        struct Response{
            uint32_t length;
            // char[] message
        };
    }
    
    struct FilePart{
        static constexpr DatagramId Id_Start = 120;
        static constexpr DatagramId Id_Part = 121;
        static constexpr DatagramId Id_Success = 122;
        static constexpr DatagramId Id_Fail = 123;
        uint32_t fileId;
        uint32_t partSize;
        // char[] data
    };
    
    namespace CustomFile{
        struct Request{
            static constexpr DatagramId Id = 130;
            uint32_t filepathLength;
            uint32_t fromEnd;
            // char[] filepath
        };
        // Response is the SendComplete response
    }
}
