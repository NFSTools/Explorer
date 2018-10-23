#include "chunk_stream.hpp"
#include "utils.hpp"
#include "solid_list_stream.hpp"
#include "texture_pack_stream.hpp"

const unsigned int kSolidListChunk = 0x80134000;

read_result chunk_stream::read(void *buf, size_t size)
{
    // Check internal stream position
    if (this->m_streamPos + size > this->m_endPos)
    {
        throw std::runtime_error(string_format(
                "STREAM ERROR: Read operation would pass the end of the stream. Currently at %d, requested %zu bytes.",
                this->m_streamPos, size));
    }

    this->m_stream.read((char *) buf, size);
    this->m_streamPos += size;

    return RESULT_OK;
}

std::shared_ptr<chunk> chunk_stream::read_chunk()
{
    chunk_header header{};

    this->read(&header, sizeof(chunk_header));

    return std::make_shared<chunk>(header.type, header.length, this->m_streamPos);
}

void chunk_stream::seek(int position, unsigned int direction)
{
    // 0 = SEEK_SET
    // 1 = SEEK_CUR
    // 2 = SEEK_END

    if (direction == 0)
    {
        this->m_stream.seekg(position);
        this->m_streamPos = position;
    } else if (direction == 1)
    {
        this->m_stream.seekg(position, std::ios::cur);
        this->m_streamPos += position;
    } else if (direction == 2)
    {
        if (position > 0)
        {
            throw std::runtime_error("Can't seek past the end.");
        } else if (abs(position) > m_streamLength)
        {
            throw std::runtime_error("Can't seek before the beginning.");
        }

        this->m_stream.seekg(position, std::ios::end);
        this->m_streamPos = this->m_streamLength - position;
    }
}

void chunk_stream::skip_chunk(std::shared_ptr<chunk> chunk)
{
    this->seek(chunk->end_offset, 0);
}

void chunk_stream::process_chunk(std::shared_ptr<chunk> chunk)
{
    if (chunk->type == kSolidListChunk)
    {
        auto sls = std::make_shared<solid_list_stream>(this, chunk);

        if (auto np = std::dynamic_pointer_cast<base_data_resource>(sls->get())) {
            this->resources.push_back(np);
        }
//        this->resources.push_back(std::dynamic_pointer_cast<std::shared_ptr<base_data_resource>>(sls->get()));
    } else if (chunk->type == 0xB3300000)
    {
        auto tpk_stream = std::make_shared<texture_pack_stream>(this, chunk);

        if (auto np = std::dynamic_pointer_cast<base_data_resource>(tpk_stream->get())) {
            this->resources.push_back(np);
        }
    }
}
