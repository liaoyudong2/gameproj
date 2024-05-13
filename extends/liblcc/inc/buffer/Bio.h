//
// Created by liao on 2024/5/13.
//

#ifndef LCC_BIO_H
#define LCC_BIO_H

namespace Lcc {
    class BufferBio {
    public:
        BufferBio();

        virtual ~BufferBio();

        /**
         * 清空缓冲区
         */
        void Clear();

        /**
         * 获取当前缓冲区数据头部
         * @return 缓冲区数据
         */
        char *Data() const;

        /**
         * 获取当前容量大小
         * @return 容量大小
         */
        unsigned int Capacity() const;

        /**
         * 获取已使用缓冲区大小
         * @return 已使用缓冲区大小
         */
        unsigned int UsedSize() const;

        /**
         * 获取可用缓存区大小
         * @return 可用大小
         */
        unsigned int AvailSize() const;

        /**
         * 从缓存区读取数据
         * @param out 输出缓存区
         * @param size 输出缓存区大小
         * @return 实际读取字节数
         */
        unsigned int Read(char *out, unsigned int size);

        /**
         * 写入缓存区数据
         * @param in 输入缓存区
         * @param size 输入缓存区大小
         * @return 实际写入字节数
         */
        unsigned int Write(const char *in, unsigned int size);

    private:
        unsigned int _in;
        unsigned int _out;
        unsigned int _size;
        unsigned char *_chunk;
    };
}

#endif //LCC_BIO_H
