#include <csignal>
#include <map>
#include <cassert>
#include "solid_list_stream.hpp"

struct PACK solid_list_info_struct
{
    unsigned long long blank;
    unsigned int unknown1;
    unsigned int object_count;
    char pipeline_path[0x38];
    char class_type[0x20];
    unsigned long long blank2;
    unsigned int unknown_offset;
};

struct PACK solid_object_header_struct
{
    unsigned int blank1;
    unsigned int blank2;
    unsigned int blank3;
    unsigned int unknown1;
    unsigned int hash;
    unsigned int num_tris;
    unsigned int unknown2;
    unsigned int unknown3;
    float bounds_min[4];
    float bounds_max[4];
    float transform[16]; // 4x4 matrix
    unsigned int blank4[6];
    float unknown4[2];
};

struct PACK mesh_descriptor_struct
{
    unsigned long long unknown1;
    unsigned int unknown2;
    unsigned int flags;
    unsigned int num_materials;
    unsigned int blank1;
    unsigned int num_vertex_buffers;
    unsigned int blank2[3];
    unsigned int num_tris;
    unsigned int num_indices;
    unsigned int blank3;
};

struct PACK mesh_material_struct
{
    unsigned int flags;
    unsigned int hash;
    unsigned int unknown1, unknown2;
    float min_point[3];
    float max_point[3];
    unsigned int unknown3;
    unsigned char texture_assignments[4];
    unsigned char unknown4[16];
    unsigned int num_vertices;
    unsigned int num_indices;
    unsigned int num_tris;
    unsigned int offset; // index buffer, not triangles
    unsigned char unknown5[36];
};

static_assert(sizeof(mesh_material_struct) == 116);

solid_list_stream::solid_list_stream(chunk_stream *chunk_stream, std::shared_ptr<chunk> chunk)
{
    this->m_named_materials = 0;
    this->m_object_count = 0;
    this->m_solid_list.reset(new solid_list);
    this->m_chunk_stream = chunk_stream;
    this->read_chunks(chunk->offset, chunk->length, chunk_stream);
//    this->debug();
}

void solid_list_stream::read_chunks(unsigned int offset, unsigned int length, chunk_stream *stream)
{
    auto tmpStream = m_chunk_stream->substream(offset, length);

    while (tmpStream->data_remaining())
    {
        auto tmpChunk = tmpStream->read_chunk();

        if (tmpChunk->type == 0x80134010)
        {
            m_named_materials = 0;
            m_current_object.reset(new solid_object);
            m_solid_list->solid_objects[m_object_count++] = m_current_object;
        }

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

void solid_list_stream::handle_chunk(std::shared_ptr<chunk> chunk, chunk_stream *stream)
{
    switch (chunk->type)
    {
        case 0x134002:
        {
            solid_list_info_struct solidListInfo{};
            stream->read(&solidListInfo, sizeof(solidListInfo));

            m_solid_list->pipeline_path = std::string(solidListInfo.pipeline_path);
            m_solid_list->class_type = std::string(solidListInfo.class_type);

            m_solid_list->solid_objects.resize(solidListInfo.object_count);

            break;
        }
        case 0x134011:
        {
            stream->align_padding(chunk);
            auto solidObjectHeader = stream->read<solid_object_header_struct>();
            auto name = m_chunk_stream->read_string();

            m_current_object->name = name;
            m_current_object->hash = solidObjectHeader.hash;
            m_current_object->min_point.x = solidObjectHeader.bounds_min[0];
            m_current_object->min_point.y = solidObjectHeader.bounds_min[1];
            m_current_object->min_point.z = solidObjectHeader.bounds_min[2];
            m_current_object->max_point.x = solidObjectHeader.bounds_max[0];
            m_current_object->max_point.y = solidObjectHeader.bounds_max[1];
            m_current_object->max_point.z = solidObjectHeader.bounds_max[2];
            m_current_object->posX = solidObjectHeader.transform[12];
            m_current_object->posY = solidObjectHeader.transform[13];
            m_current_object->posZ = solidObjectHeader.transform[14];

            break;
        }
        case 0x134012:
        {
            for (auto i = 0; i < chunk->length >> 3; i++)
            {
                m_current_object->texture_hashes.push_back(stream->read<unsigned int>());
                stream->seek(4, SEEK_CUR);
            }

            break;
        }
        case 0x134900:
        {
            stream->align_padding(chunk);
            auto descriptor = stream->read<mesh_descriptor_struct>();

            m_current_object->mesh.reset(new solid_mesh);
            m_current_object->mesh->flags = descriptor.flags;
            m_current_object->mesh->num_materials = descriptor.num_materials;
            m_current_object->mesh->num_vertex_buffers = descriptor.num_vertex_buffers;
            m_current_object->mesh->num_vertices = 0;
            m_current_object->mesh->num_tris =
                    descriptor.num_tris == 0 ? descriptor.num_indices / 3 : descriptor.num_tris;
            m_current_object->mesh->faces.resize(m_current_object->mesh->num_tris);

            break;
        }
        case 0x134b01:
        {
            stream->align_padding(chunk);
            auto vb = std::make_shared<vertex_buffer>();
            vb->position = 0;
            vb->length = chunk->length / 4;
            vb->data = (float *) calloc(vb->length, 4);

            stream->read(vb->data, chunk->length);

            m_current_object->mesh->vertex_buffers.push_back(vb);

            break;
        }
        case 0x134b02:
        {
            stream->align_padding(chunk);

            auto vertex_stream_index = 0u;
            auto last_unknown1 = 0;

            for (auto i = 0; i < m_current_object->mesh->num_materials; i++)
            {
                auto mat_struct = stream->read<mesh_material_struct>();
                auto material = std::make_shared<solid_mesh_material>();

                if (i == 0)
                {
                    vertex_stream_index = 0;
                } else
                {
                    if (mat_struct.unknown1 != last_unknown1)
                    {
                        vertex_stream_index++;
                    }
                }

                material->texture_hash = m_current_object->texture_hashes[mat_struct.texture_assignments[0]];
                material->num_tris = mat_struct.num_tris == 0 ? mat_struct.num_indices / 3 : mat_struct.num_tris;
                material->num_indices = mat_struct.num_indices;
                material->num_vertices = mat_struct.num_vertices;

                material->min_point.x = mat_struct.min_point[0];
                material->min_point.y = mat_struct.min_point[1];
                material->min_point.z = mat_struct.min_point[2];

                material->max_point.x = mat_struct.max_point[0];
                material->max_point.y = mat_struct.max_point[1];
                material->max_point.z = mat_struct.max_point[2];

                material->hash = mat_struct.hash;
                material->name = string_format("unnamed-material-%08X", material->texture_hash);

                material->vertex_stream_index = vertex_stream_index;

                m_current_object->mesh->materials.push_back(material);
                m_current_object->mesh->num_vertices += material->num_vertices;

                last_unknown1 = mat_struct.unknown1;
            }

            break;
        }
        case 0x134b03:
        {
            stream->align_padding(chunk);

            auto face_idx = 0;

            for (auto i = 0; i < m_current_object->mesh->num_materials; i++)
            {
                auto &material = m_current_object->mesh->materials[i];

                for (auto j = 0; j < material->num_tris; j++)
                {
                    auto face = &m_current_object->mesh->faces[face_idx + j];

                    face->material_index = (unsigned char) i;
                    stream->read(&face->face1, sizeof(unsigned short));
                    stream->read(&face->face2, sizeof(unsigned short));
                    stream->read(&face->face3, sizeof(unsigned short));
                }

                face_idx += material->num_tris;
            }

            break;
        }
        case 0x134c02:
        {
            m_current_object->mesh->materials[m_named_materials]->name = stream->read_string();
            m_current_object->mesh->materials[m_named_materials]->name += string_format("_%d", m_named_materials);

            std::replace(m_current_object->mesh->materials[m_named_materials]->name.begin(), m_current_object->mesh->materials[m_named_materials]->name.end(), ' ', '_');

            m_named_materials++;

            if (m_named_materials == m_current_object->mesh->num_materials)
            {
                m_current_object->mesh->process_data();
            }

            break;
        }
        default:
            if (chunk->type)
            {
            }

            break;
    }
}

void solid_list_stream::debug()
{
    for (auto &solid_object : this->m_solid_list->solid_objects)
    {
        std::string filename = string_format("%s-%s.obj", this->m_solid_list->class_type.c_str(),
                                             solid_object->name.c_str());
        simple_filewriter sfw(filename);

        sfw.write_line(string_format("g %s", solid_object->name.c_str()));

        for (auto &vb : solid_object->mesh->vertex_buffers)
        {
            for (auto i = 0; i < vb->num_verts; i++)
            {
                auto vertex = vb->get_vertex();
                sfw.write_line(string_format("# buffer - %d/%d", i + 1, vb->num_verts));
                sfw.write_line(string_format("v %f %f %f", vertex.x, vertex.y, vertex.z));
            }
        }

        auto faceIdx = 0;

        for (auto i = 0; i < solid_object->mesh->num_materials; i++)
        {
            auto &material = solid_object->mesh->materials[i];

            std::string mat_name(material->name);

            std::replace(mat_name.begin(), mat_name.end(), ' ', '_');

            sfw.write_line(string_format("usemtl %s", mat_name.c_str()));

            for (auto j = 0; j < material->num_tris; j++)
            {
                auto face = solid_object->mesh->faces[faceIdx + j];

                sfw.write_line(string_format("f %d/%d %d/%d %d/%d", face.face1 + 1, face.face1 + 1, face.face2 + 1,
                                             face.face2 + 1, face.face3 + 1, face.face3 + 1));
            }

            faceIdx += material->num_tris;
        }
    }
}

solid_mesh_vertex vertex_buffer::get_vertex()
{
    float x = this->data[this->position + 0];
    float y = this->data[this->position + 2];
    float z = this->data[this->position + 1];
    float u = this->data[this->position + 5];
    float v = -this->data[this->position + 6];
    unsigned int color = *reinterpret_cast<unsigned int *>(&this->data[this->position + 3]);

    this->position += this->stride;

    solid_mesh_vertex vertex{};
    vertex.x = x;
    vertex.y = y;
    vertex.z = z;
    vertex.u = u;
    vertex.v = v;
    vertex.color = color;

    return vertex;
}

void solid_mesh::process_data()
{
    std::map<unsigned int, unsigned int> buffer_count_map;

    for (auto &material : this->materials)
    {
        buffer_count_map[material->vertex_stream_index] = 0;
    }

    for (auto &material : this->materials)
    {
        buffer_count_map[material->vertex_stream_index] += material->num_vertices;
        this->vertex_buffers[material->vertex_stream_index]->num_verts += material->num_vertices;
    }

    auto numVerts = 0u;
    auto curFaceIdx = 0u;

    for (auto i = 0; i < this->materials.size(); i++)
    {
        auto &material = this->materials[i];
        auto stream_index = material->vertex_stream_index;
        auto stream = this->vertex_buffers[stream_index];
        auto stride = stream->length / stream->num_verts;
        auto shift = numVerts - stream->position / stride;
        auto vertCount = stream->num_verts;

        stream->stride = stride;

        for (auto j = 0; j < vertCount; j++)
        {
            if (stream->position >= stream->length) break;

            stream->position += stride;
//            if (this->vertex_buffers[stream_index]->position >= this->vertex_buffers[stream_index]->length) break;

//            this->vertex_buffers[stream_index]->get_vertex();
            numVerts++;
        }

        auto triCount = material->num_tris;

        if (triCount == 0)
        {
            triCount = material->num_indices / 3;
        }

        for (auto j = 0; j < triCount; j++)
        {
            auto faceIdx = curFaceIdx + j;
            auto face = faces[faceIdx];

            if (face.face1 != face.face2
                && face.face1 != face.face3
                && face.face2 != face.face3)
            {
                unsigned short original_face[] = {
                        face.face1, face.face2, face.face3
                };

                faces[faceIdx].face1 = (unsigned short) (shift + original_face[0]);
                faces[faceIdx].face2 = (unsigned short) (shift + original_face[2]);
                faces[faceIdx].face3 = (unsigned short) (shift + original_face[1]);
            }

            faces[faceIdx].material_index = (unsigned char) i;
        }

        curFaceIdx += triCount;

        if (curFaceIdx >= this->num_tris) break;
    }

    for (auto &vb : vertex_buffers)
    {
        vb->position = 0;
    }
}
