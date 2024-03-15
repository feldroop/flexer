#pragma once

#include <about_floxer.hpp>
#include <fmindex.hpp>
#include <input.hpp>
#include <verification.hpp>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <map>

namespace output {

void save_index(fmindex const& _index, std::filesystem::path const& _index_path);

class sam_output {
private:
    std::ofstream output_stream;

public:
    sam_output(
        std::filesystem::path const& output_path,
        std::vector<input::reference_record> const& reference_records
    );

    void output_for_query(
        input::query_record const& fastq_query,
        std::vector<input::reference_record> const& references,
        std::vector<std::map<size_t, verification::query_alignment>> const& alignments
    );
};

} // namespace output
