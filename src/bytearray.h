/**
 * 二进制数组(序列化/反序列化)
 * 网络传输过程中的数据序列化问题
 *
 * 网络中的消息一般需要序列化才能进行传输
 */


#ifndef SERVER_FRAMWORK_BYTEARRAY_H
#define SERVER_FRAMWORK_BYTEARRAY_H

#include <memory>
#include <string>
#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <vector>


namespace dht{

class ByteArray{
public:
    typedef std::shared_ptr<ByteArray> ptr;
    //ByteArray的存储节点
    struct Node {
        Node(size_t s); //构造指定大小的内存块
        Node();
        ~Node();

        char* ptr; //内存块地址指针
        size_t size; //内存块大小
        Node* next;  //下一个内存块地址
    };

    //使用指定长度的内存块构造ByteArray
    ByteArray(size_t base_size = 4096);
    ~ByteArray();

    //write
    void writeFint8  (int8_t value); //写入固定长度int8_t类型的数据
    void writeFuint8 (uint8_t value);
    void writeFint16 (int16_t value);
    void writeFuint16(uint16_t value);
    void writeFint32 (int32_t value);
    void writeFuint32(uint32_t value);
    void writeFint64 (int64_t value);
    void writeFuint64(uint64_t value);
    void writeInt32  (int32_t value);
    void writeUint32 (uint32_t value);
    void writeInt64  (int64_t value);
    void writeUint64 (uint64_t value);
    void writeFloat  (float value);
    void writeDouble (double value);
    void writeStringF16(const std::string& value);
    void writeStringF32(const std::string& value);
    void writeStringF64(const std::string& value);
    void writeStringVint(const std::string& value);
    void writeStringWithoutLength(const std::string& value);
    //read
    int8_t   readFint8();
    uint8_t  readFuint8();
    int16_t  readFint16();
    uint16_t readFuint16();
    int32_t  readFint32();
    uint32_t readFuint32();
    int64_t  readFint64();
    uint64_t readFuint64();
    int32_t  readInt32();
    uint32_t readUint32();
    int64_t  readInt64();
    uint64_t readUint64();
    float    readFloat();
    double   readDouble();
    std::string readStringF16();
    std::string readStringF32();
    std::string readStringF64();
    std::string readStringVint();

    void clear();//清空ByteArray

    /**
     * @brief 写入size长度的数据
     * @param[in] buf 内存缓存指针
     * @param[in] size 数据大小
     * @post m_position += size, 如果m_position > m_size 则 m_size = m_position
     */
    void write(const void* buf, size_t size);
    /**
     * @brief 读取size长度的数据
     * @param[out] buf 内存缓存指针
     * @param[in] size 数据大小
     * @post m_position += size, 如果m_position > m_size 则 m_size = m_position
     * @exception 如果getReadSize() < size 则抛出 std::out_of_range
     */
    void read(void* buf, size_t size);
    /**
     * @brief 读取size长度的数据
     * @param[out] buf 内存缓存指针
     * @param[in] size 数据大小
     * @param[in] position 读取开始位置
     */
    void read(void* buf, size_t size, size_t position) const;
    size_t getPosition() const { return m_position;}
    void setPosition(size_t v);
    bool writeToFile(const std::string& name) const;
    bool readFromFile(const std::string& name);
    //返回内存块大小
    size_t getBaseSize() const { return m_baseSize;}
    //返回可读数据大小
    size_t getReadSize() const { return m_size - m_position;}
    bool isLittleEndian() const;
    void setIsLittleEndian(bool val); //true->小端, false -> 大端
    //将ByteArray里面的数据[m_position, m_size)转成std::string
    std::string toString() const;
    //将ByteArray里面的数据[m_position, m_size)转成16进制的std::string(格式:FF FF FF)
    std::string toHexString() const;
    /**
     * @brief 获取可读取的缓存,保存成iovec数组
     * @param[out] buffers 保存可读取数据的iovec数组
     * @param[in] len 读取数据的长度,如果len > getReadSize() 则 len = getReadSize()
     * @return 返回实际数据的长度
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len = ~0ull) const;
    /**
     * 从position读获取可读取的缓存
     */
    uint64_t getReadBuffers(std::vector<iovec>& buffers, uint64_t len, uint64_t position) const;
    /**
     * @brief 获取可写入的缓存,保存成iovec数组
     * @param[out] buffers 保存可写入的内存的iovec数组
     * @param[in] len 写入的长度
     * @return 返回实际的长度
     * @post 如果(m_position + len) > m_capacity 则 m_capacity扩容N个节点以容纳len长度
     */
    uint64_t getWriteBuffers(std::vector<iovec>& buffers, uint64_t len);
    /**
     * 返回数据的长度
     */
    size_t getSize() const { return m_size;}
private:
    /**
     * @brief 扩容ByteArray,使其可以容纳size个数据(如果原本可以可以容纳,则不扩容)
     */
    void addCapacity(size_t size);
    /**
     * @brief 获取当前的可写入容量
     */
    size_t getCapacity() const { return m_capacity - m_position;}
private:
    size_t m_baseSize;/// 内存块的大小
    size_t m_position;/// 当前操作位置
    size_t m_capacity;/// 当前的总容量
    size_t m_size;/// 当前数据的大小
    int8_t m_endian;/// 字节序,默认大端
    Node* m_root;/// 第一个内存块指针
    Node* m_cur;/// 当前操作的内存块指针
};

}


#endif //SERVER_FRAMWORK_BYTEARRAY_H
