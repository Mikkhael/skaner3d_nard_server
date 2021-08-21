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
    
    namespace Config{
        namespace Network{
            namespace Set{
                struct Request{
                    static constexpr DatagramId Id = 100;
                    uint32_t ip;
                    uint32_t mask;
                    uint32_t gateway;
                    uint32_t dns1;
                    uint32_t dns2;
                    uint32_t isDynamic;
                };
                struct Response{
                    static constexpr DatagramId Id_Success = 101;
                    static constexpr DatagramId Id_Fail = 102;
                };
            }
        }
        namespace Device{
            
            namespace Set{
                struct Request{
                    static constexpr DatagramId Id = 110;
                    // char []
                };
                struct Response{
                    static constexpr DatagramId Id_Success = 111;
                    static constexpr DatagramId Id_Fail = 112;
                };
            }
            namespace Get{
                struct Request{
                    static constexpr DatagramId Id = 120;
                };
                struct Response{
                    static constexpr DatagramId Id = 121;
                    // char []
                };
            }
        }
    }
    
    struct Reboot{
        static constexpr DatagramId Id = 130;
    };
    
    struct Snap{
        static constexpr DatagramId Id = 200;
        uint32_t seriesid;
    };
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
    
    namespace SnapFrame{
        struct Request{
            static constexpr DatagramId Id = 140;
        };
        // Response is the SendComplete response
    }
    
    namespace DownloadSnap{
        struct Request{
            static constexpr DatagramId Id = 200;
            static constexpr DatagramId Id_CheckIfExists = 205;
            uint32_t seriesid;
        };
        struct Response{
            static constexpr DatagramId Id_Success = 201;
            static constexpr DatagramId Id_NotFound = 202;
            // potential file transfer, if expected
        };
    }
}
