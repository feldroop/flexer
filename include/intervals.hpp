#pragma once

#include <mutex_wrapper.hpp>

#include <cstddef>
#include <optional>
#include <set>
#include <vector>

#include <interval-tree/interval_tree.hpp>

namespace intervals {

enum interval_relationship {
    completely_above,
    completely_below,
    contains,
    equal,
    inside,
    overlapping_or_touching_above,
    overlapping_or_touching_below
};

// [start, end), must be non-empty (start < end)
struct half_open_interval {
    size_t start;
    size_t end;

    size_t size() const;

    half_open_interval overlap_interval_with(half_open_interval const other) const;

    // <this> has returned relationship with <other>
    interval_relationship relationship_with(half_open_interval const other) const;

    half_open_interval trim_from_both_sides(size_t const amount) const;

    lib_interval_tree::interval<size_t> to_lib_intervaltree_interval() const;

};

bool operator==(half_open_interval const& interval1, half_open_interval const& interval2);

// order intervals by their END position, because it works better with the std::set functions
auto operator<(half_open_interval const& interval1, half_open_interval const& interval2);

enum use_interval_optimization {
    on, off
};

// NOT internally thread safe, for a single reference
class verified_intervals {
public:
    verified_intervals() = default;

    using intervals_t = std::set<half_open_interval>;

    // workaround because this needs to be default constructible
    void configure(
        use_interval_optimization const activity_status
    );

    void insert(half_open_interval const new_interval);

    // true if an interval in this set contains the target interval or contains an equal interval,
    // or if the set contains an interval that covers overlap_rate_that_counts_as_contained of the target_interval
    bool contains(half_open_interval const target_interval) const;

private:
    use_interval_optimization activity_status = use_interval_optimization::on;

    // inside this tree I use closed intervals, because I want [0,5) and [5, 10) to count as overlapping
    lib_interval_tree::interval_tree_t<size_t> intervals{};
};

using verified_intervals_for_all_references = std::vector<shared_mutex_guarded<verified_intervals>>;

verified_intervals_for_all_references create_thread_safe_verified_intervals(
    size_t const num_references,
    use_interval_optimization const activity_status
);

} // namespace intervals
