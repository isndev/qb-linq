# TODO — qb-linq

## Planned improvements

### High-value container optimizations

- [ ] **Replace `std::unordered_map` / `std::unordered_set` with ska flat hash map/set**
  Operators `except`, `intersect`, `distinct`, `except_by`, `intersect_by`, `union_by`,
  `count_by`, `to_unordered_map`, `to_unordered_set`, `to_dictionary`, and `group_by` all
  allocate `std::unordered_*` containers internally. The qb framework ships flat ordered and
  unordered map/set implementations based on **ska hash map** that provide significantly
  better cache locality and faster insert/lookup for small-to-medium key sets — typical in
  LINQ pipelines. Migrating these internal containers would be a **drop-in performance win**
  with no API surface change.

  **Scope:** Replace `std::unordered_set<K>` / `std::unordered_map<K,V>` in
  `query_range.h` (materializers) and `extra_views.h` (`distinct_view`) with the framework's
  flat equivalents. Benchmark before/after with `qb_linq_benchmark`.
