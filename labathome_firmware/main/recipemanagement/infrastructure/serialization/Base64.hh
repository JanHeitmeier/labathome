#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <cstring>

/**
 * @brief Hardware-agnostic Base64 encoder/decoder (RFC 4648)
 * 
 * Used for encoding binary TimeSeries data for transmission over JSON/WebSocket.
 * Static class - no instances needed.
 */
class Base64 {
public:
    /**
     * @brief Encode binary data to Base64 string
     * @param data Binary data to encode
     * @return Base64-encoded string
     */
    static std::string encode(const std::vector<uint8_t>& data) {
        static constexpr const char* BASE64_CHARS = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::string result;
        result.reserve(((data.size() + 2) / 3) * 4);
        
        size_t i = 0;
        uint8_t array3[3];
        uint8_t array4[4];
        
        for (size_t idx = 0; idx < data.size(); idx++) {
            array3[i++] = data[idx];
            if (i == 3) {
                array4[0] = (array3[0] & 0xfc) >> 2;
                array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
                array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
                array4[3] = array3[2] & 0x3f;
                
                for (i = 0; i < 4; i++) {
                    result += BASE64_CHARS[array4[i]];
                }
                i = 0;
            }
        }
        
        if (i > 0) {
            for (size_t j = i; j < 3; j++) {
                array3[j] = '\0';
            }
            
            array4[0] = (array3[0] & 0xfc) >> 2;
            array4[1] = ((array3[0] & 0x03) << 4) + ((array3[1] & 0xf0) >> 4);
            array4[2] = ((array3[1] & 0x0f) << 2) + ((array3[2] & 0xc0) >> 6);
            
            for (size_t j = 0; j < i + 1; j++) {
                result += BASE64_CHARS[array4[j]];
            }
            
            while (i++ < 3) {
                result += '=';
            }
        }
        
        return result;
    }
    
    /**
     * @brief Decode Base64 string to binary data (optional, not used in backend)
     * @param base64 Base64-encoded string
     * @return Decoded binary data
     */
    static std::vector<uint8_t> decode(const std::string& base64) {
        static constexpr const char* BASE64_CHARS = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::vector<uint8_t> result;
        result.reserve((base64.size() / 4) * 3);
        
        size_t i = 0;
        uint8_t array4[4];
        uint8_t array3[3];
        
        for (char c : base64) {
            if (c == '=') break;
            
            const char* pos = strchr(BASE64_CHARS, c);
            if (!pos) continue;
            
            array4[i++] = static_cast<uint8_t>(pos - BASE64_CHARS);
            
            if (i == 4) {
                array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
                array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
                array3[2] = ((array4[2] & 0x3) << 6) + array4[3];
                
                for (i = 0; i < 3; i++) {
                    result.push_back(array3[i]);
                }
                i = 0;
            }
        }
        
        if (i > 0) {
            for (size_t j = i; j < 4; j++) {
                array4[j] = 0;
            }
            
            array3[0] = (array4[0] << 2) + ((array4[1] & 0x30) >> 4);
            array3[1] = ((array4[1] & 0xf) << 4) + ((array4[2] & 0x3c) >> 2);
            
            for (size_t j = 0; j < i - 1; j++) {
                result.push_back(array3[j]);
            }
        }
        
        return result;
    }
    
private:
    Base64() = delete;  // Static class, no instances
};
