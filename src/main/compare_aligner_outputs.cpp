#include <about_floxer.hpp>

#include <algorithm>
#include <unordered_map>
#include <vector>

#include <seqan3/io/sam_file/input.hpp>
#include <sharg/all.hpp>
#include <spdlog/spdlog.h>

struct alignment_data_t {
    bool is_unmapped_floxer{true};
    bool is_unmapped_minimap{true};

    bool is_chimeric_minimap{false};

    bool mentioned_by_floxer{false};
    bool mentioned_by_minimap{false};
};

void read_alignments(
    std::filesystem::path const& alignment_file_path,
    std::unordered_map<std::string, alignment_data_t>& alignment_data_by_query_id,
    bool const is_floxer
) {
    seqan3::sam_file_input input{alignment_file_path};

    for (auto const& record : input) {
        auto iter = alignment_data_by_query_id.find(record.id());
        if (iter == alignment_data_by_query_id.end()) {
            auto emplace_result = alignment_data_by_query_id.emplace(record.id(), alignment_data_t{});
            assert(emplace_result.second);
            iter = emplace_result.first;
        }

        auto& alignment_data = iter->second;

        if (is_floxer) {
            alignment_data.mentioned_by_floxer = true;
        } else {
            alignment_data.mentioned_by_minimap = true;
        }

        if (static_cast<bool>(record.flag() & seqan3::sam_flag::unmapped)) {
            continue;
        }

        if (static_cast<bool>(record.flag() & seqan3::sam_flag::supplementary_alignment)) {
            if (is_floxer) {
                spdlog::warn("Unexpected chimeric floxer alignment");
            }
            alignment_data.is_chimeric_minimap = true;
        }

        if (is_floxer) {
            alignment_data.is_unmapped_floxer = false;
        } else {
            alignment_data.is_unmapped_minimap = false;
        }
    }
}

int main(int argc, char** argv) {
    sharg::parser parser{ "compare_aligner_outputs", argc, argv, sharg::update_notifications::off };

    parser.info.author = about_floxer::author;
    parser.info.description = {
        "Compares the alignment output of two readmappers regarding alignments found, edit distance and large indels. "
        "This program was created to compare specifically minimap2 and floxer."
    };
    parser.info.email = about_floxer::email;
    parser.info.url = about_floxer::url;
    parser.info.short_description = "Compare the alignment output of two readmappers.";
    parser.info.synopsis = {
        "./compare_aligner_outputs --reference minimap2_alignments.sam --new floxer_alignments.sam"
    };
    parser.info.version = "1.0.0";
    parser.info.date = "03.06.2024";

    std::filesystem::path minimap_input_path{};
    std::filesystem::path floxer_input_path{};

    parser.add_option(minimap_input_path, sharg::config{
        .short_id = 'r',
        .long_id = "reference",
        .description = "The sam file of the reference read mapper (e.g. minimap2).",
        .required = true,
        .validator = sharg::input_file_validator{}
    });

    parser.add_option(floxer_input_path, sharg::config{
        .short_id = 'n',
        .long_id = "new",
        .description = "The sam file of the new read mapper (e.g. floxer).",
        .required = true,
        .validator = sharg::input_file_validator{}
    });

    parser.parse();

    std::unordered_map<std::string, alignment_data_t> alignment_data_by_query_id{};

    read_alignments(minimap_input_path, alignment_data_by_query_id, false);
    read_alignments(floxer_input_path, alignment_data_by_query_id, true);

    size_t const num_queries = alignment_data_by_query_id.size();

    size_t num_unmapped_floxer = 0;
    size_t num_unmapped_minimap = 0;

    size_t num_chimeric_minimap = 0;

    size_t num_unmapped_both = 0;
    size_t num_minimap_unmapped_floxer_mapped = 0;
    size_t num_floxer_unmapped_minimap_linear_mapped = 0;
    size_t num_floxer_unmapped_minimap_chimeric_mapped = 0;
    size_t num_mapped_both_minimap_linear = 0;
    size_t num_mapped_both_minimap_chimeric = 0;

    for (auto const& [query_id, alignment_data] : alignment_data_by_query_id) {
        if (!alignment_data.mentioned_by_floxer) {
            spdlog::warn("Query {} not mentioned by floxer", query_id);
        }

        if (!alignment_data.mentioned_by_minimap) {
            spdlog::warn("Query {} not mentioned by minimap", query_id);
        }

        if (alignment_data.is_unmapped_floxer) {
            ++num_unmapped_floxer;
        }

        if (alignment_data.is_unmapped_minimap) {
            ++num_unmapped_minimap;
        }

        if (alignment_data.is_chimeric_minimap) {
            ++num_chimeric_minimap;
        }

        if (alignment_data.is_unmapped_floxer && alignment_data.is_unmapped_minimap) {
            ++num_unmapped_both;
        }

        if (!alignment_data.is_unmapped_floxer && alignment_data.is_unmapped_minimap) {
            ++num_minimap_unmapped_floxer_mapped;
        }

        if (alignment_data.is_unmapped_floxer && !alignment_data.is_unmapped_minimap) {
            if (alignment_data.is_chimeric_minimap) {
                ++num_floxer_unmapped_minimap_chimeric_mapped;
            } else {
                ++num_floxer_unmapped_minimap_linear_mapped;
            }
        }

        if (!alignment_data.is_unmapped_floxer && !alignment_data.is_unmapped_minimap) {
            if (alignment_data.is_chimeric_minimap) {
                ++num_mapped_both_minimap_chimeric;
            } else {
                ++num_mapped_both_minimap_linear;
            }
        }
    }

    double const num_queries_d = static_cast<double>(num_queries);

    spdlog::info("Numer of queries: {} ({:.2f}%)", num_queries, num_queries / num_queries_d);
    spdlog::info("Floxer unmapped queries: {} ({:.2f}%)", num_unmapped_floxer, num_unmapped_floxer / num_queries_d);
    spdlog::info("Minimap unmapped queries: {} ({:.2f}%)", num_unmapped_minimap, num_unmapped_minimap / num_queries_d);
    spdlog::info(
        "Minimap chimeric mapped queries: {} ({:.2f}%)",
        num_chimeric_minimap,
        num_chimeric_minimap / num_queries_d
    );
    spdlog::info("Both unmapped: {} ({:.2f}%)", num_unmapped_both, num_unmapped_both / num_queries_d);
    spdlog::info(
        "Floxer mapped, minimap unmapped: {} ({:.2f}%)",
        num_minimap_unmapped_floxer_mapped,
        num_minimap_unmapped_floxer_mapped / num_queries_d
    );
    spdlog::info(
        "Floxer unmapped, minimap linear mapped: {} ({:.2f}%)",
        num_floxer_unmapped_minimap_linear_mapped,
        num_floxer_unmapped_minimap_linear_mapped / num_queries_d
    );
    spdlog::info(
        "Floxer unmapped, minimap chimeric mapped: {} ({:.2f}%)",
        num_floxer_unmapped_minimap_chimeric_mapped,
        num_floxer_unmapped_minimap_chimeric_mapped / num_queries_d
    );
    spdlog::info(
        "Both mapped, minimap linear: {} ({:.2f}%)",
        num_mapped_both_minimap_linear,
        num_mapped_both_minimap_linear / num_queries_d
    );
    spdlog::info(
        "Both mapped, minimap chimeric: {} ({:.2f}%)",
        num_mapped_both_minimap_chimeric,
        num_mapped_both_minimap_chimeric / num_queries_d
    );

    return 0;
}
