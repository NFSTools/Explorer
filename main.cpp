#include <iostream>
#include <boost/filesystem.hpp>
#include "chunk_stream.hpp"
#include "texture_pack_stream.hpp"
#include "solid_list_stream.hpp"

void read_child_chunks(std::shared_ptr<chunk> chunk, std::shared_ptr<chunk_stream> stream)
{
    auto tmpStream = stream->substream(chunk->offset, chunk->length);

    while (tmpStream->data_remaining())
    {
        auto tmpChunk = tmpStream->read_chunk();

//        fprintf(stdout, "\t%08X [%d bytes] @ %08X\n", tmpChunk->type, tmpChunk->length, tmpChunk->offset);

        if (tmpChunk->is_parent)
        {
            read_child_chunks(tmpChunk, tmpStream);
        }

        tmpStream->skip_chunk(tmpChunk);
    }
}

int main(int argc, char **argv)
{
    if (argc < 2)
    {
        std::cerr << "Not enough arguments" << std::endl;
        return 1;
    }

    std::string inputFile(argv[1]);
    boost::filesystem::path path(inputFile);

    if (!boost::filesystem::is_regular_file(path))
    {
        std::cerr << "Not a file: " << path << std::endl;
        return 1;
    }

    std::ifstream stream(inputFile, std::ios::binary);
    auto cstream = std::make_shared<chunk_stream>(stream);

    printf("stream length -> %lu bytes\n", cstream->get_length());

    clock_t begin = clock();

    while (cstream->data_remaining())
    {
        auto chunk = cstream->read_chunk();

        cstream->process_chunk(chunk);
        cstream->skip_chunk(chunk);
    }

    clock_t end = clock();

    printf("read in %f seconds\n", double(end - begin) / CLOCKS_PER_SEC);

    for (auto &resource : cstream->resources)
    {
        if (auto tp = std::dynamic_pointer_cast<texture_pack>(resource))
        {
            printf("Texture Pack: %s [%s]\n", tp->name.c_str(), tp->pipeline_path.c_str());

            for (auto &texture : tp->textures)
            {
                texture->write_to_file(string_format("%08X.dds", texture->texture_hash));
            }
        } else if (auto slp = std::dynamic_pointer_cast<solid_list>(resource))
        {
            printf("Solid List: %s [%s]\n", slp->pipeline_path.c_str(), slp->class_type.c_str());

            for (auto& slo : slp->solid_objects) {
                slo->write_to_file(string_format("%s.obj", slo->name.c_str()));
            }
        }
    }

    return 0;
}