//
// Created by liao on 2024/5/3.
//

#ifndef LCC_STRINGS_H
#define LCC_STRINGS_H

namespace Lcc {
    namespace Utils {
        /**
         * 复制字符串
         * @param dest 复制到该缓冲区
         * @param dsize 缓存区大小
         * @param src 源数据
         * @param ssize 源数据大小
         */
        inline void StringCopy(char *dest, unsigned int dsize, const char *src, unsigned int ssize) {
            strncpy(dest, src, ssize > dsize ? dsize : ssize);
        }

        inline void StringToLower(std::string &in) {
            std::transform(in.begin(), in.end(), in.begin(), ::tolower);
        }

        inline void StringToUpper(std::string &in) {
            std::transform(in.begin(), in.end(), in.begin(), ::toupper);
        }

        template<typename T>
        bool StringToNumber(std::string &&in, T *out) {
            if (out) {
                std::istringstream is(in);
                is >> *out;
                if (is.eof() && !is.fail()) {
                    return true;
                }
            }
            return false;
        }
    }
}

#endif //LCC_STRINGS_H
