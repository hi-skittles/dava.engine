#pragma once

#include <cstdint>
#include <string>
#include <vector>

class pack_meta_data
{
public:
    /** Create meta from serialized bytes
		    Throw exception on error
		*/
    pack_meta_data(const void* ptr, std::size_t size);

    uint32_t get_num_files() const;
    uint32_t get_num_packs() const;
    uint32_t get_pack_index_for_file(const uint32_t file_index) const;
    const std::tuple<std::string, std::string>& get_pack_info(const uint32_t packIndex) const;
    const std::vector<uint32_t>& get_children(const uint32_t pack_index) const;

    std::vector<uint8_t> serialize() const;
    void deserialize(const void* ptr, size_t size);

private:
    // fileNames already in DVPK format
    // table 1.
    // fileName -> fileIndex(0-NUM_FILES) -> packIndex(0-NUM_PACKS)
    std::vector<uint32_t> table_files;
    // table 2.
    // packIndex(0-NUM_PACKS) -> packName, dependencies
    std::vector<std::tuple<std::string, std::string>> table_packs;

    std::vector<std::vector<uint32_t>> table_childs;
};
