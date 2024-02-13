#pragma once

#include <fmindex-collection/fmindex/BiFMIndex.h>
#include <fmindex-collection/occtable/EPR.h>

size_t constexpr Sigma = 5; // DNA + Sentinel
using Table = fmindex_collection::occtable::EprV2_16<Sigma>;
using FloxerFMIndex = fmindex_collection::BiFMIndex<Table>;

struct FloxerFMIndexWithMetaData {
    FloxerFMIndex index;
    std::vector<std::string> reference_tags;

    template<class Archive>
    void serialize(Archive & ar) {
        ar(index, reference_tags);
    }
};
