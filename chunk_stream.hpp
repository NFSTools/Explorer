#ifndef EXPLORER_CHUNK_STREAM_HPP
#define EXPLORER_CHUNK_STREAM_HPP

#include "utils.hpp"
#include <fstream>

enum read_result
{
    RESULT_OK,
    RESULT_ERROR
};

class base_data_resource {
public:
    virtual ~base_data_resource() = default;
};

class chunk_stream;

struct chunk
{
    unsigned int type;
    unsigned int length;
    unsigned int full_length;
    unsigned int offset;
    unsigned int end_offset;
    bool is_parent;

    chunk(unsigned int type, unsigned int length, unsigned int offset)
    {
        this->type = type;
        this->length = length;
        this->offset = offset;
        this->end_offset = offset + length;
        this->full_length = length + 8;
        this->is_parent = (type & 0x80000000) == 0x80000000;
    }
};

struct PACK chunk_header
{
    unsigned int type;
    unsigned int length;
};

class chunk_stream
{
public:
    explicit chunk_stream(std::istream &stream) : m_streamLength(0),
                                                  m_streamPos(0),
                                                  m_stream(stream)
    {
        stream.seekg(0, std::ios::end);

        m_streamLength = (long) stream.tellg();
        m_endPos = m_streamLength;

        stream.seekg(0);
    }

    chunk_stream(std::istream &stream, unsigned int streamPos, unsigned int size) : m_streamLength(size),
                                                                                    m_streamPos(streamPos),
                                                                                    m_stream(stream)
    {
        m_stream.seekg(streamPos);
        m_endPos = m_streamPos + m_streamLength;
    }

    std::shared_ptr<chunk_stream> substream(unsigned int pos, unsigned int size)
    {
        return std::make_shared<chunk_stream>(m_stream, pos, size);
    }

    read_result read(void *buf, size_t size);

    template<typename T>
    T read(size_t size = sizeof(T))
    {
        T result{};
        memset(&result, '\0', size);

        read(&result, size);

        return result;
    }

    std::string read_string()
    {
        std::string str = "";
        char c;

        while ((c = read<char>()) != 0x00)
        {
            str += c;
        }

        return str;
    }

    /**
     * @return
     */
    std::shared_ptr<chunk> read_chunk();

    /**
     * @param boundary
     */
    void align_padding(std::shared_ptr<chunk> chunk)
    {
        auto padding = 0;
        while (read<int>() == 0x11111111) padding += 4;

        m_streamPos -= 4;
        m_stream.seekg(-4, std::ios::cur);

        chunk->offset += padding;
        chunk->length -= padding;
        chunk->full_length -= padding;
        chunk->end_offset = chunk->offset + chunk->length;
    }

    /**
     * @param position
     * @param direction
     */
    void seek(int position, unsigned int direction);

    /**
     * @param chunk
     */
    void skip_chunk(std::shared_ptr<chunk> chunk);

    /**
     * @param chunk
     */
    void process_chunk(std::shared_ptr<chunk> chunk);

    bool data_remaining()
    {
        return m_streamPos < m_endPos;
    }

    long get_length()
    {
        return m_streamLength;
    }

    chunk_stream(const chunk_stream &stream) = delete;

    chunk_stream(const chunk_stream &&stream) = delete;

    chunk_stream &operator=(const chunk_stream &stream) = delete;

    chunk_stream &operator=(const chunk_stream &&stream) = delete;

    std::vector<std::shared_ptr<base_data_resource>> resources;
private:
    std::istream &m_stream;
    long m_streamLength;
    long m_streamPos;
    long m_endPos;
};


#endif //EXPLORER_CHUNK_STREAM_HPP
