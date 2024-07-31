#include <cstdint>
#include <vector>

enum CanFlags {
    STANDARD_ID = 0,    
    EXTENDED_ID = 1,    
    ERROR_FRAME = 1 << 1,    
    REMOTE_FRAME = 1 << 2,   
    INVALID_FRAME = 1 << 3  
};

struct CanFrame {
    CanFrame(uint32_t id, uint32_t requested_length);
    CanFrame(uint32_t id, uint32_t requested_length, uint32_t flags);
    CanFrame(uint32_t id, const std::vector<char>& message);
    CanFrame(uint32_t id, const std::vector<char>& message, uint32_t flags);

    private:
    const uint32_t m_id {0};
    const uint32_t m_requested_length {0};
    const std::vector<char> m_message;
    const uint32_t m_flags {CanFlags::INVALID_FRAME};
};

