#ifndef EXPLORER_TEXTURE_PACK_STREAM_HPP
#define EXPLORER_TEXTURE_PACK_STREAM_HPP

#include <string>
#include <memory>
#include <vector>
#include "chunk_stream.hpp"
#include "DDS.h"

class texture
{
public:
    std::string name;
    unsigned int width, height, mipmaps;
    unsigned int dds_type;
    unsigned int texture_hash;
    unsigned int type_hash;
    unsigned int data_offset, data_size;
    unsigned char *data_buffer;

    void write_to_file(std::string filename) const
    {
        std::ofstream stream(filename, std::ios::trunc | std::ios::binary);

        stream.write((const char*) &DirectX::DDS_MAGIC, 4);
        DirectX::DDS_HEADER dds_header{};

        dds_header.dwSize = 0x7C;
        dds_header.dwWidth = width;
        dds_header.dwHeight = height;
        dds_header.dwMipMapCount = 0;
        dds_header.ddspf.dwSize = 32;

        if (this->dds_type == 0x15)
        {
            dds_header.ddspf.dwFlags = 0x41;
            dds_header.ddspf.dwRGBBitCount = 0x20;
            dds_header.ddspf.dwRBitMask = 0xFF0000;
            dds_header.ddspf.dwGBitMask = 0xFF00;
            dds_header.ddspf.dwBBitMask = 0xFF;
            dds_header.ddspf.dwABitMask = 0xFF000000;
            dds_header.dwCaps = 0x40100a;
        } else
        {
            dds_header.ddspf.dwFlags = 0x4;
            dds_header.ddspf.dwFourCC = this->dds_type;
            dds_header.dwCaps = 0x401008;
        }

        stream.write((const char*) &dds_header, sizeof(DirectX::DDS_HEADER));
        stream.write((const char*) (data_buffer + data_offset), data_size);
    }
};

class texture_pack : public base_data_resource
{
public:
    std::string pipeline_path;
    std::string name;
    std::vector<std::shared_ptr<texture>> textures;
    unsigned int hash;

    ~texture_pack()
    {
        textures.clear();
    }
};

class texture_pack_stream
{
public:
    texture_pack_stream(chunk_stream *chunk_stream, std::shared_ptr<chunk> chunk);

    std::shared_ptr<texture_pack> get()
    {
        return m_texture_pack;
    }

private:
    chunk_stream *m_chunk_stream;
    std::shared_ptr<texture_pack> m_texture_pack;
    int m_texture_count;

    void debug();

    void read_chunks(unsigned int offset, unsigned int length, chunk_stream *stream);

    void handle_chunk(std::shared_ptr<chunk> chunk, chunk_stream *stream);
};


#endif //EXPLORER_TEXTURE_PACK_STREAM_HPP
