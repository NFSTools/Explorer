#include "texture_pack_stream.hpp"

struct PACK texture_pack_info_struct
{
    unsigned int version;
    char name[28];
    char pipeline_path[64];
    unsigned int hash;
};

struct PACK texture_info_struct
{
    unsigned char blank[12];
    unsigned int tex_hash, type_hash;
    unsigned int unknown1;
    unsigned int data_size;
    unsigned int unknown2;
    unsigned int width, height, mip_count;
    unsigned int unknown3, unknown4;
    unsigned char unknown5[24];
    unsigned int unknown6;
    unsigned int data_offset;
    unsigned char unknown7[60];
    unsigned char name_length;
};

texture_pack_stream::texture_pack_stream(chunk_stream *chunk_stream, std::shared_ptr<chunk> chunk)
{
    m_texture_count = 0;
    this->m_texture_pack.reset(new texture_pack);
    this->m_chunk_stream = chunk_stream;
    this->read_chunks(chunk->offset, chunk->length, chunk_stream);
}

void texture_pack_stream::read_chunks(unsigned int offset, unsigned int length, chunk_stream *stream)
{
    auto tmpStream = m_chunk_stream->substream(offset, length);

    while (tmpStream->data_remaining())
    {
        auto tmpChunk = tmpStream->read_chunk();

        if (tmpChunk->is_parent)
        {
            read_chunks(tmpChunk->offset, tmpChunk->length, &*tmpStream);
        } else
        {
            this->handle_chunk(tmpChunk, &*tmpStream);
        }

        tmpStream->skip_chunk(tmpChunk);
    }
}

void texture_pack_stream::handle_chunk(std::shared_ptr<chunk> chunk, chunk_stream *stream)
{
    switch (chunk->type)
    {
        case 0x33310001:
        {
            auto tpk_info = stream->read<texture_pack_info_struct>();

            m_texture_pack->pipeline_path = std::string(tpk_info.pipeline_path);
            m_texture_pack->name = std::string(tpk_info.name);
            m_texture_pack->hash = tpk_info.hash;

            break;
        }
        case 0x33310002:
        {
            m_texture_pack->textures.resize(chunk->length >> 3);
            break;
        }
        case 0x33310004:
        {
            for (auto i = 0; i < m_texture_pack->textures.size(); i++)
            {
                auto texture_info = stream->read<texture_info_struct>();

                char *name_tmp = (char *) malloc(texture_info.name_length);
                stream->read(name_tmp, texture_info.name_length);
                std::string name(name_tmp);
                free(name_tmp);

                m_texture_pack->textures[m_texture_count].reset(new texture);
                m_texture_pack->textures[m_texture_count]->height = texture_info.height;
                m_texture_pack->textures[m_texture_count]->width = texture_info.width;
                m_texture_pack->textures[m_texture_count]->mipmaps = texture_info.mip_count;
                m_texture_pack->textures[m_texture_count]->texture_hash = texture_info.tex_hash;
                m_texture_pack->textures[m_texture_count]->type_hash = texture_info.type_hash;
                m_texture_pack->textures[m_texture_count]->data_offset = texture_info.data_offset;
                m_texture_pack->textures[m_texture_count]->data_size = texture_info.data_size;
                m_texture_pack->textures[m_texture_count]->name = name;

                m_texture_count++;
            }

            break;
        }
        case 0x33310005:
        {
            for (auto i = 0; i < m_texture_pack->textures.size(); i++)
            {
                stream->seek(12, SEEK_CUR);
                stream->read(&m_texture_pack->textures[i]->dds_type, 4);
                stream->seek(16, SEEK_CUR);
//                m_texture_pack->textures[i]->dds_type = stream->read
            }

            break;
        }
        case 0x33320002: // data
        {
            stream->align_padding(chunk);

            unsigned char* data_buffer = (unsigned char*) malloc(chunk->length);
            stream->read(data_buffer, chunk->length);

            for (auto i = 0; i < m_texture_pack->textures.size(); i++)
            {
                m_texture_pack->textures[i]->data_buffer = data_buffer;
            }

            break;
        }
        default:
            break;
    }
}

void texture_pack_stream::debug()
{

}