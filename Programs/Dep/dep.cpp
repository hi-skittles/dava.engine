#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <iostream>
#include <fstream>
#include <string>
#include <set>
#include <vector>
#include <cstdint>
#include <sstream>
#include <list>
#include <algorithm>
#include "tinydir.h"

#ifdef _MSC_VER
#include <filesystem>
namespace fs = std::experimental::filesystem;
#endif

static const uint32_t crc32_tab[256] =
{
  0x00000000, 0x77073096, 0xee0e612c, 0x990951ba,
  0x076dc419, 0x706af48f, 0xe963a535, 0x9e6495a3,
  0x0edb8832, 0x79dcb8a4, 0xe0d5e91e, 0x97d2d988,
  0x09b64c2b, 0x7eb17cbd, 0xe7b82d07, 0x90bf1d91,
  0x1db71064, 0x6ab020f2, 0xf3b97148, 0x84be41de,
  0x1adad47d, 0x6ddde4eb, 0xf4d4b551, 0x83d385c7,
  0x136c9856, 0x646ba8c0, 0xfd62f97a, 0x8a65c9ec,
  0x14015c4f, 0x63066cd9, 0xfa0f3d63, 0x8d080df5,
  0x3b6e20c8, 0x4c69105e, 0xd56041e4, 0xa2677172,
  0x3c03e4d1, 0x4b04d447, 0xd20d85fd, 0xa50ab56b,
  0x35b5a8fa, 0x42b2986c, 0xdbbbc9d6, 0xacbcf940,
  0x32d86ce3, 0x45df5c75, 0xdcd60dcf, 0xabd13d59,
  0x26d930ac, 0x51de003a, 0xc8d75180, 0xbfd06116,
  0x21b4f4b5, 0x56b3c423, 0xcfba9599, 0xb8bda50f,
  0x2802b89e, 0x5f058808, 0xc60cd9b2, 0xb10be924,
  0x2f6f7c87, 0x58684c11, 0xc1611dab, 0xb6662d3d,
  0x76dc4190, 0x01db7106, 0x98d220bc, 0xefd5102a,
  0x71b18589, 0x06b6b51f, 0x9fbfe4a5, 0xe8b8d433,
  0x7807c9a2, 0x0f00f934, 0x9609a88e, 0xe10e9818,
  0x7f6a0dbb, 0x086d3d2d, 0x91646c97, 0xe6635c01,
  0x6b6b51f4, 0x1c6c6162, 0x856530d8, 0xf262004e,
  0x6c0695ed, 0x1b01a57b, 0x8208f4c1, 0xf50fc457,
  0x65b0d9c6, 0x12b7e950, 0x8bbeb8ea, 0xfcb9887c,
  0x62dd1ddf, 0x15da2d49, 0x8cd37cf3, 0xfbd44c65,
  0x4db26158, 0x3ab551ce, 0xa3bc0074, 0xd4bb30e2,
  0x4adfa541, 0x3dd895d7, 0xa4d1c46d, 0xd3d6f4fb,
  0x4369e96a, 0x346ed9fc, 0xad678846, 0xda60b8d0,
  0x44042d73, 0x33031de5, 0xaa0a4c5f, 0xdd0d7cc9,
  0x5005713c, 0x270241aa, 0xbe0b1010, 0xc90c2086,
  0x5768b525, 0x206f85b3, 0xb966d409, 0xce61e49f,
  0x5edef90e, 0x29d9c998, 0xb0d09822, 0xc7d7a8b4,
  0x59b33d17, 0x2eb40d81, 0xb7bd5c3b, 0xc0ba6cad,
  0xedb88320, 0x9abfb3b6, 0x03b6e20c, 0x74b1d29a,
  0xead54739, 0x9dd277af, 0x04db2615, 0x73dc1683,
  0xe3630b12, 0x94643b84, 0x0d6d6a3e, 0x7a6a5aa8,
  0xe40ecf0b, 0x9309ff9d, 0x0a00ae27, 0x7d079eb1,
  0xf00f9344, 0x8708a3d2, 0x1e01f268, 0x6906c2fe,
  0xf762575d, 0x806567cb, 0x196c3671, 0x6e6b06e7,
  0xfed41b76, 0x89d32be0, 0x10da7a5a, 0x67dd4acc,
  0xf9b9df6f, 0x8ebeeff9, 0x17b7be43, 0x60b08ed5,
  0xd6d6a3e8, 0xa1d1937e, 0x38d8c2c4, 0x4fdff252,
  0xd1bb67f1, 0xa6bc5767, 0x3fb506dd, 0x48b2364b,
  0xd80d2bda, 0xaf0a1b4c, 0x36034af6, 0x41047a60,
  0xdf60efc3, 0xa867df55, 0x316e8eef, 0x4669be79,
  0xcb61b38c, 0xbc66831a, 0x256fd2a0, 0x5268e236,
  0xcc0c7795, 0xbb0b4703, 0x220216b9, 0x5505262f,
  0xc5ba3bbe, 0xb2bd0b28, 0x2bb45a92, 0x5cb36a04,
  0xc2d7ffa7, 0xb5d0cf31, 0x2cd99e8b, 0x5bdeae1d,
  0x9b64c2b0, 0xec63f226, 0x756aa39c, 0x026d930a,
  0x9c0906a9, 0xeb0e363f, 0x72076785, 0x05005713,
  0x95bf4a82, 0xe2b87a14, 0x7bb12bae, 0x0cb61b38,
  0x92d28e9b, 0xe5d5be0d, 0x7cdcefb7, 0x0bdbdf21,
  0x86d3d2d4, 0xf1d4e242, 0x68ddb3f8, 0x1fda836e,
  0x81be16cd, 0xf6b9265b, 0x6fb077e1, 0x18b74777,
  0x88085ae6, 0xff0f6a70, 0x66063bca, 0x11010b5c,
  0x8f659eff, 0xf862ae69, 0x616bffd3, 0x166ccf45,
  0xa00ae278, 0xd70dd2ee, 0x4e048354, 0x3903b3c2,
  0xa7672661, 0xd06016f7, 0x4969474d, 0x3e6e77db,
  0xaed16a4a, 0xd9d65adc, 0x40df0b66, 0x37d83bf0,
  0xa9bcae53, 0xdebb9ec5, 0x47b2cf7f, 0x30b5ffe9,
  0xbdbdf21c, 0xcabac28a, 0x53b39330, 0x24b4a3a6,
  0xbad03605, 0xcdd70693, 0x54de5729, 0x23d967bf,
  0xb3667a2e, 0xc4614ab8, 0x5d681b02, 0x2a6f2b94,
  0xb40bbe37, 0xc30c8ea1, 0x5a05df1b, 0x2d02ef8d
};

void echo_command(int argc, const char** argv, std::streambuf* coutbuf)
{
    if (argc <= 2)
    {
        throw std::runtime_error("nothing to echo");
    }

    std::string aprefix;
    std::string rprefix;
    std::ofstream output;
    std::set<std::string> files;

    // collect all files and options
    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-ap" && (i + 1) < argc)
        {
            ++i;
            aprefix = argv[i];
        }
        else if (arg == "-rp" && (i + 1) < argc)
        {
            ++i;
            rprefix = argv[i];
        }
        else if (arg == "-o" && (i + 1) < argc)
        {
            ++i;
            std::string output_name = argv[i];
            output.open(output_name);
            if (!output)
            {
                std::string err = std::strerror(errno);
                throw std::runtime_error("can't open output file: " + output_name + " cause: " + err);
            }
        }
        else
        {
            files.insert(arg);
        }
    }

    if (output.is_open())
    {
        std::cout.rdbuf(output.rdbuf());
    }
    else
    {
        // write to console
    }

    // process files
    for (auto& path : files)
    {
        std::ifstream file(path);
        if (file)
        {
            std::string name = path;

            // should remove prefix?
            if (!rprefix.empty())
            {
                std::string::size_type pos = name.find(rprefix);
                if (0 == pos)
                {
                    name.erase(0, rprefix.size());
                }
            }

            std::cout << aprefix << name << '\n';
        }
    }

    if (output.is_open())
    {
        output.flush();
        std::cout.rdbuf(coutbuf);
        output.close();
    }
}

// Wildcard matching algorithms
// http://xoomer.virgilio.it/acantato/dev/wildcard/wildmatch.html
// Non recursive final version
bool szWildMatch8(const char* pat, const char* str)
{
    const char* s;
    const char* p;
    bool star = false;

loopStart:
    for (s = str, p = pat; *s; ++s, ++p)
    {
        switch (*p)
        {
        case '?':
            if (*s == '.')
                goto starCheck;
            break;
        case '*':
            star = true;
            str = s, pat = p;
            if (!*++pat)
                return true;
            goto loopStart;
        default:
            if (*s != *p)
                goto starCheck;
            break;
        }
    }
    if (*p == '*')
        ++p;
    return (!*p);

starCheck:
    if (!star)
        return false;
    str++;
    goto loopStart;
}

void merge_command(int argc, const char** argv, std::streambuf* coutbuf)
{
    if (argc <= 2)
    {
        throw std::runtime_error("nothing to echo");
    }

    std::ofstream output;
    std::set<std::string> patterns;

    // collect all files and options
    for (int i = 2; i < argc; ++i)
    {
        std::string arg = argv[i];
        if (arg == "-o" && (i + 1) < argc)
        {
            ++i;
            std::string output_name = argv[i];
            output.open(output_name, std::ios_base::out | std::ios_base::binary);
            if (!output)
            {
                std::string err = std::strerror(errno);
                throw std::runtime_error("can't open output file: " + output_name + " cause: " + err);
            }
        }
        else if (arg == "--")
        {
            // don't parse args after --
            break;
        }
        else
        {
            patterns.insert(arg);
        }
    }

    if (output.is_open())
    {
        std::cout.rdbuf(output.rdbuf());
    }
    else
    {
        // write to console
    }

    std::list<std::string> files;

    for (auto& pat : patterns)
    {
        if ((pat.find('?') != std::string::npos) || (pat.find('*') != std::string::npos))
        {
            std::string directory = "./";
            std::string mask = pat;

            size_t d = pat.find_last_of("\\/");
            if (d != std::string::npos)
            {
                directory = mask.substr(0, d);
                mask = mask.substr(d + 1);
            }

            tinydir_dir tiny_dir;
            tinydir_open(&tiny_dir, directory.c_str());

            while (tiny_dir.has_next)
            {
                tinydir_file tiny_file;
                tinydir_readfile(&tiny_dir, &tiny_file);

                if (!tiny_file.is_dir)
                {
                    if (szWildMatch8(mask.c_str(), tiny_file.name))
                    {
                        files.push_back(tiny_file.path);
                    }
                }

                tinydir_next(&tiny_dir);
            }
        }
        else
        {
            files.push_back(pat);
        }
    }

    for (auto& path : files)
    {
        std::ifstream file;

        file.open(path, std::ios::in | std::ios::binary | std::ios::ate);
        if (file)
        {
            if (file.tellg() > 0)
            {
                file.seekg(0, std::ios::beg);
                std::cout << file.rdbuf();
            }

            file.close();
        }
    }

    if (output.is_open())
    {
        output.flush();
        std::cout.rdbuf(coutbuf);
        output.close();
    }
}

uint32_t calculate_crc32(const std::vector<char>& buff, std::streamsize sz)
{
    uint32_t crc32 = 0xffffffff;
    for (auto i = 0u; i < sz; i++)
    {
        crc32 = (crc32 >> 8) ^ crc32_tab[(crc32 ^ buff[i]) & 0xff];
    }
    crc32 ^= 0xffffffff;
    return crc32;
}

std::string calculate_crc32(std::ifstream& file)
{
    using namespace std;

    streamsize file_size = file.tellg();
    vector<char> buff(static_cast<size_t>(file_size));

    file.seekg(0, ios::beg);
    file.read(buff.data(), buff.size());
    streamsize sz = file.gcount();
    if (sz != file_size)
    {
        throw runtime_error("can't read file in memory");
    }

    uint32_t crc32 = calculate_crc32(buff, sz);

    std::stringstream ss;
    ss << uppercase << hex << crc32;

    return ss.str();
}

void create_hash_file(int argc, const char** argv, std::streambuf* coutbuf)
{
    using namespace std;
    if (argc <= 2)
    {
        throw runtime_error("no filename");
    }

    ofstream output;
    string path;
    ifstream file;

    for (int i = 2; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-o" && (i + 1) < argc)
        {
            i++;
            output.open(argv[i]);
        }
        else
        {
            if (path.empty())
            {
                path = arg;
            }
        }
    }

    if (output.is_open())
    {
        cout.rdbuf(output.rdbuf());
    }

    file.open(path, ios::binary | ios::ate);
    if (file)
    {
        cout << calculate_crc32(file) << '\n';
    }
    else
    {
        throw runtime_error("can't open file to calculate hash: " + path);
    }

    if (output.is_open())
    {
        cout.rdbuf(coutbuf);
        output.close();
    }
}

void generate_sql(int argc, const char** argv, std::streambuf* coutbuf)
{
    using namespace std;
    if (argc <= 3)
    {
        throw std::runtime_error("no params");
    }

    vector<string> packs;
    string listpath;
    string packpath;
    bool is_gpu = false;
    ofstream output;

    for (int i = 2; i < argc; ++i)
    {
        string arg = argv[i];
        if (arg == "-l" && (i + 1) < argc)
        {
            ++i;
            listpath = argv[i];
        }
        else if (arg == "-o" && (i + 1) < argc)
        {
            ++i;
            output.open(argv[i]);
        }
        else if (arg == "-g" && (i + 1) < argc)
        {
            ++i;
            string next_arg = argv[i];
            is_gpu = (next_arg == "true");
        }
        else
        {
            if (packpath.empty())
            {
                packpath = arg;
            }
            else
            {
                packs.push_back(arg);
            }
        }
    }

    if (output.is_open())
    {
        cout.rdbuf(output.rdbuf());
    }

    if (packs.size() > 0)
    {
        string pack = packs[0];

        cout << "CREATE TABLE IF NOT EXISTS packs (name TEXT PRIMARY KEY NOT NULL, hash TEXT NOT NULL, is_gpu INTEGER NOT NULL, size INTEGER NOT NULL, dependency TEXT NOT NULL);" << '\n';
        cout << "CREATE TABLE IF NOT EXISTS files (path TEXT PRIMARY KEY, pack TEXT NOT NULL, hash TEXT NOT NULL, FOREIGN KEY(pack) REFERENCES packs(name));" << '\n';

        if (listpath.empty())
        {
            listpath = pack + ".list";
        }

        ifstream packfile(packpath, ios::ate | ios::binary);

        streamsize size = packfile.tellg();
        if (size == -1)
        {
            throw runtime_error("can't open file: " + packpath);
        }

        string hash = calculate_crc32(packfile);

        stringstream dependency;
        for (size_t i = 1; i < packs.size(); ++i)
        {
            const string& pack_name = packs[i];
            if (pack_name.find(' ') != string::npos)
            {
                throw runtime_error("pack name with SPACE char: " + pack_name);
            }
            dependency << pack_name << ' ';
        }

        cout << "INSERT INTO packs VALUES('" << pack << "', '" << hash << "', '" << is_gpu << "', '" << size << "', '" << dependency.str() << "');" << '\n';

        ifstream listfile(listpath);
        if (listfile)
        {
            string fileName;
            while (getline(listfile, fileName))
            {
                ifstream file(fileName, ios::binary | ios::ate);
                if (!file)
                {
                    throw runtime_error("can't open file to calculate hash: " + fileName);
                }
                vector<char> file_content(static_cast<size_t>(file.tellg()));
                file.seekg(0, ios::beg);
                file.read(file_content.data(), file_content.size());
                uint32_t file_hash = calculate_crc32(file_content, file_content.size());
                cout << "INSERT INTO files VALUES('" << fileName << "', '" << pack << "', '" << hex << file_hash << "');" << '\n';
            }

            listfile.close();
        }
    }

    if (output.is_open())
    {
        cout.rdbuf(coutbuf);
        output.close();
    }
}

void print_help()
{
    std::cerr << '\n'
              << "Usage: dep <command> [<args>]" << '\n'
              << '\n'
              << "Commands:" << '\n'
              << "    echo [options] [<file>...] - Try to open each file and if success prints it name" << '\n'
              << "        -ap <prefix>           - add prefix to printed name" << '\n'
              << "        -rp <prefix>           - remove prefix from printed name" << '\n'
              << "        -o <output>            - print into output file" << '\n'
              << '\n'
              << "    merge [<file_wildcard>...] - Try to open files that are matched wildcard and if success prints it content" << '\n'
              << "        -o <output>            - print into output file" << '\n'
              << '\n'
              << "    hash <file> - Calculate CRC32 for file" << '\n'
              << "        -o <output>            - print into output file" << '\n'
              << '\n'
              << "    sql [options] <packpath> <packname> [dependencies...]" << '\n'
              << "        -l <file>           - read file for pack content" << '\n'
              << "        -o <output>         - print into output file" << '\n'
              << "        -g {true|false}     - is gpu pack(default false)" << '\n';
}

int main(int argc, const char* argv[])
{
    using namespace std;

    streambuf* coutbuf = cout.rdbuf();

    try
    {
        if (argc > 1)
        {
            const string cmd = argv[1];

            if (cmd == "help" || cmd == "--help")
            {
                print_help();
            }
            else if (cmd == "echo")
            {
                echo_command(argc, argv, coutbuf);
            }
            else if (cmd == "merge")
            {
                merge_command(argc, argv, coutbuf);
            }
            else if (cmd == "hash")
            {
                create_hash_file(argc, argv, coutbuf);
            }
            else if (cmd == "sql")
            {
                generate_sql(argc, argv, coutbuf);
            }
            else
            {
                throw std::runtime_error("unknown command: " + cmd);
            }
        }
        else
        {
            throw runtime_error("no command");
        }
    }
    catch (exception& ex)
    {
        cerr << ex.what() << '\n';
        cerr << "input params:\n";
        for (int i = 0; i < argc; ++i)
        {
            cerr << argv[i] << ' ';
        }
        cerr << '\n';
#ifdef _MSC_VER
        cerr << "current pwd: " << fs::current_path() << '\n';
#endif
        cerr << "Use --help for help" << '\n';

        return EXIT_FAILURE;
    }

    // restore initial buffer (in any case)
    std::cout.rdbuf(coutbuf);

    return EXIT_SUCCESS;
}
