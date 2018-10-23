#ifndef EXPLORER_SOLID_LIST_STREAM_HPP
#define EXPLORER_SOLID_LIST_STREAM_HPP

#include <memory>
#include <vector>
#include "chunk_stream.hpp"
#include <boost/filesystem.hpp>

struct solid_mesh_face
{
public:
    unsigned short face1;
    unsigned short face2;
    unsigned short face3;
    unsigned char material_index;
};

struct solid_mesh_vertex
{
public:
    float x;
    float y;
    float z;
    unsigned int color;
    float u;
    float v;
};

struct vertex_buffer
{
public:
    unsigned int position;
    unsigned int length;
    unsigned int num_verts;
    unsigned int stride;

    float *data;

    solid_mesh_vertex get_vertex();
};

struct solid_mesh_material
{
public:
    unsigned int num_vertices;
    unsigned int num_tris;
    unsigned int num_indices;
    unsigned int hash;
    unsigned int texture_hash;
    unsigned int vertex_stream_index;
    std::string name;
    vector3 min_point, max_point;
};

struct solid_mesh
{
public:
    unsigned int flags;
    unsigned int num_materials;
    unsigned int num_vertex_buffers;
    unsigned int num_vertices;
    unsigned int num_tris;

    std::vector<std::shared_ptr<vertex_buffer>> vertex_buffers;
    std::vector<std::shared_ptr<solid_mesh_material>> materials;
    std::vector<solid_mesh_face> faces;

    void process_data();
};

class solid_object
{
public:
    std::string name;
    unsigned int hash;
    float posX, posY, posZ;

    vector3 min_point, max_point;
    std::shared_ptr<solid_mesh> mesh;
    std::vector<unsigned int> texture_hashes;

    solid_object()
    {
        name = "UNNAMED";
        hash = 0;
        posX = 0.0f;
        posY = 0.0f;
        posZ = 0.0f;
        min_point = vector3();
        max_point = vector3();
    }

    void write_to_file(std::string filename) const
    {
        auto stem_path = boost::filesystem::path(filename).stem();
        auto base_directory = stem_path.parent_path();
        auto material_library_path = boost::filesystem::path(stem_path).concat(".mtl").string();
        auto object_path = boost::filesystem::path(stem_path).concat(".obj").string();

        {
            simple_filewriter sfw(material_library_path);

            for (auto &material : this->mesh->materials)
            {
                sfw.write_line(string_format("newmtl %s", material->name.c_str()));
                sfw.write_line("Ka 255 255 255");
                sfw.write_line("Kd 255 255 255");
                sfw.write_line("Ks 255 255 255");

                auto texture_path = boost::filesystem::path(base_directory).append(string_format("%08X.dds", material->texture_hash)).string();
                sfw.write_line(string_format("map_Ka %s", texture_path.c_str()));
                sfw.write_line(string_format("map_Kd %s", texture_path.c_str()));
                sfw.write_line(string_format("map_Ks %s", texture_path.c_str()));
            }
        }

        {
            simple_filewriter sfw(object_path);

            sfw.write_line(string_format("g %s", name.c_str()));
            sfw.write_line(string_format("mtllib %s", material_library_path.c_str()));

            for (auto &vb : mesh->vertex_buffers)
            {
                for (auto i = 0; i < vb->num_verts; i++)
                {
                    auto vertex = vb->get_vertex();
                    sfw.write_line(string_format("# buffer - %d/%d", i + 1, vb->num_verts));
                    sfw.write_line(string_format("v %f %f %f", vertex.x, vertex.y, vertex.z));
                    sfw.write_line(string_format("vt %f %f", vertex.u, vertex.v));
                }
            }

            auto faceIdx = 0;

            for (auto i = 0; i < mesh->num_materials; i++)
            {
                auto &material = mesh->materials[i];

                sfw.write_line(string_format("usemtl %s", material->name.c_str()));

                for (auto j = 0; j < material->num_tris; j++)
                {
                    auto face = mesh->faces[faceIdx + j];

                    sfw.write_line(string_format("f %d/%d %d/%d %d/%d", face.face1 + 1, face.face1 + 1, face.face2 + 1,
                                                 face.face2 + 1, face.face3 + 1, face.face3 + 1));
                }

                faceIdx += material->num_tris;
            }

        }
    }
};

class solid_list : public base_data_resource
{
public:
    std::string pipeline_path;
    std::string class_type;
    std::vector<std::shared_ptr<solid_object>> solid_objects;

    ~solid_list()
    {
        solid_objects.clear();
    }
};

class solid_list_stream
{
public:
    solid_list_stream(chunk_stream *chunk_stream, std::shared_ptr<chunk> chunk);

    std::shared_ptr<solid_list> get()
    {
        return m_solid_list;
    }

private:
    chunk_stream *m_chunk_stream;
    std::shared_ptr<solid_list> m_solid_list;
    std::shared_ptr<solid_object> m_current_object;
    int m_named_materials;
    int m_object_count;

    void debug();

    void read_chunks(unsigned int offset, unsigned int length, chunk_stream *stream);

    void handle_chunk(std::shared_ptr<chunk> chunk, chunk_stream *stream);
};


#endif //EXPLORER_SOLID_LIST_STREAM_HPP
