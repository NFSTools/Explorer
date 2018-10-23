#ifndef EXPLORER_UTILS_HPP
#define EXPLORER_UTILS_HPP

#include <iostream>
#include <sstream>
#include <string>
#include <streambuf>
#include <fstream>
#include <vector>

#define PACK __attribute__((__packed__))

std::string string_format(const std::string fmt_str, ...);

struct PACK vector3
{
    float x, y, z;

    bool operator==(vector3 other)
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct PACK vector4
{
    float x, y, z, w;

    bool operator==(vector4 other)
    {
        return x == other.x && y == other.y && z == other.z && w == other.w;
    }
};

struct PACK matrix4
{
    float m[16];

    float &operator[](int idx)
    {
        return m[idx];
    }
};

struct substreambuf : public std::streambuf
{
    explicit substreambuf(std::streambuf *sbuf, std::streampos pos, std::streampos len) :
            m_sbuf(sbuf),
            m_pos(pos),
            m_len(len),
            m_read(0)
    {
        m_sbuf->pubseekpos(pos);

        setbuf(nullptr, 0);
    }

protected:
    int underflow()
    {
        if (m_read >= m_len)
        {
            return std::istream::traits_type::eof();
        }

        return m_sbuf->sgetc();
    }

    int uflow()
    {
        if (m_read >= m_len)
        {
            return std::istream::traits_type::eof();
        }

        m_read += 1;

        return m_sbuf->sbumpc();
    }

    std::streampos seekoff(std::streamoff off, std::ios_base::seekdir seekdir,
                           std::ios_base::openmode openmode = std::ios_base::in | std::ios_base::out)
    {
        if (seekdir == std::ios_base::beg)
        {
            off += ((long) m_pos);
        } else if (seekdir == std::ios_base::cur)
        {
            off += ((long) m_pos) + ((long) m_read);
        } else if (seekdir == std::ios_base::end)
        {
            off += ((long) m_pos) + ((long) m_len);
        }

        return m_sbuf->pubseekpos(off, openmode) - m_pos;
    }

    std::streampos
    seekpos(std::streampos streampos, std::ios_base::openmode openmode = std::ios_base::in | std::ios_base::out)
    {
        streampos += m_pos;

        if (streampos > ((long) m_pos) + ((long) m_len))
        {
            return -1;
        }

        return m_sbuf->pubseekpos(streampos, openmode) - m_pos;
    }

private:
    std::streambuf *m_sbuf;
    std::streampos m_pos;
    std::streamsize m_len;
    std::streampos m_read;
};

class simple_filewriter
{
public:
    simple_filewriter(std::string filename) : m_filename(filename)
    {
        m_stream.open(filename, std::ios::trunc);
    }

    void write_line(std::string line)
    {
        m_stream.write(line.c_str(), line.length());
        m_stream.write("\n", strlen("\n"));
    }

private:
    std::string m_filename;
    std::ofstream m_stream;
};

struct nothing {};

template class std::vector<nothing>;

#endif //EXPLORER_UTILS_HPP
