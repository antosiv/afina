#ifndef AFINA_STORAGE_SIMPLE_LRU_H
#define AFINA_STORAGE_SIMPLE_LRU_H

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include <afina/Storage.h>

namespace Afina {
namespace Backend {

/**
 * # Map based implementation
 * That is NOT thread safe implementaiton!!
 */
class SimpleLRU : public Afina::Storage {
public:
    SimpleLRU(size_t max_size = 1024) : _max_size(max_size), size(0) {}

    ~SimpleLRU() override {
        _lru_index.clear();
        while (_lru_head != nullptr) {
            _lru_head = std::move(_lru_head->prev);
        }
    }

    // Implements Afina::Storage interface
    bool Put(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool PutIfAbsent(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Set(const std::string &key, const std::string &value) override;

    // Implements Afina::Storage interface
    bool Delete(const std::string &key) override;

    // Implements Afina::Storage interface
    bool Get(const std::string &key, std::string &value) const override;

private:
    // LRU cache node
    struct lru_node {
        std::string key;
        std::string value;
        std::unique_ptr<lru_node> prev = nullptr;
        lru_node* next = nullptr;
    };

    using index_it = std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>>::const_iterator;

    bool move_to_head(index_it match) const;

    bool clear_space(long int req_space);

    bool put_to_head(const std::string &key, const std::string &value);

    bool put_by_match(index_it match, const std::string& value);

    // Maximum number of bytes could be stored in this cache.
    // i.e all (keys+values) must be less the _max_size
    std::size_t _max_size;
    std::size_t size;
    // Main storage of lru_nodes, elements in this list ordered descending by "freshness": in the head
    // element that wasn't used for longest time.
    //
    // List owns all nodes
    mutable std::unique_ptr<lru_node> _lru_head = nullptr;
    mutable lru_node* _lru_tail = nullptr;

    // Index of nodes from list above, allows fast random access to elements by lru_node#key
    std::map<std::reference_wrapper<const std::string>, std::reference_wrapper<lru_node>, std::less<std::string>> _lru_index;
};

} // namespace Backend
} // namespace Afina

#endif // AFINA_STORAGE_SIMPLE_LRU_H
