#include "SimpleLRU.h"
#include <iostream>

namespace Afina {
    namespace Backend {

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Put(const std::string &key, const std::string &value) {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                if (!put_by_match(match, value)) return false;

            } else {

                if (!put_to_head(key, value)) return false;

            }

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match == _lru_index.end()) {

                if (!put_to_head(key, value)) return false;

            }
            else return false;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Set(const std::string &key, const std::string &value) {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                if (!put_by_match(match, value)) return false;

            }
            else return false;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Delete(const std::string &key) {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                size -= match->second->key.size() + match->second->value.size();

                if (match->second->prev != nullptr) match->second->prev->next = match->second->next;
                else _lru_tail = match->second->next;

                if (match->second->next != nullptr) {

                    match->second->next->prev = std::move(match->second->prev);

                }
                else _lru_head = std::move(match->second->prev);

                _lru_index.erase(match);

            } else return false;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Get(const std::string &key, std::string &value) const {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                move_to_head(match);
                value = match->second->value;


            } else return false;

            return true;
        }

        bool SimpleLRU::move_to_head(Afina::Backend::SimpleLRU::index_it match) const {

            if (match->second->key == _lru_tail->key and _lru_tail != _lru_head.get()) {

                auto tmp = match->second;
                _lru_tail = tmp->next;
                tmp->next = nullptr;
                _lru_tail->prev.release();
                _lru_head->next = tmp;
                tmp->prev = std::move(_lru_head);
                _lru_head.reset(tmp);

            } else if (_lru_head->key != match->second->key) {

                match->second->prev->next = match->second->next;
                match->second->next->prev.release();
                match->second->next->prev = std::move(match->second->prev);
                _lru_head->next = match->second;
                match->second->prev = std::move(_lru_head);
                _lru_head.reset(match->second);

            }

            return true;
        }

        bool SimpleLRU::clear_space(long int req_space) {

            if (req_space > _max_size) return false;
            else if (req_space < 0) return true;

            while (req_space + size > _max_size) {

                size -= _lru_tail->key.size() + _lru_tail->value.size();
                _lru_index.erase(_lru_index.find(std::reference_wrapper<const std::string>(_lru_tail->key)));
                auto tmp = _lru_tail->next;
                tmp->prev = nullptr;
                _lru_tail = tmp;

            }

            return true;
        }

        bool SimpleLRU::put_to_head(const std::string &key, const std::string &value) {

            if (!clear_space(value.size() + key.size())) return false;

            if (_lru_head == nullptr) {

                _lru_tail = new lru_node;
                _lru_head.reset(_lru_tail);

            } else {

                auto tmp = new lru_node;
                _lru_head->next = tmp;
                tmp->prev = std::move(_lru_head);
                _lru_head.reset(tmp);

            }

            _lru_head->key = key;
            _lru_head->value = value;
            size += _lru_head->key.size() + _lru_head->value.size();
            _lru_index[std::reference_wrapper<const std::string>(_lru_head->key)] = _lru_head.get();

            return true;
        }

        bool SimpleLRU::put_by_match(Afina::Backend::SimpleLRU::index_it match, const std::string& value) {

            move_to_head(match);

            if (!clear_space(value.size() - match->second->value.size())) return false;

            size += value.size() - match->second->value.size();
            _lru_head->value = value;

            return true;
        }

    } // namespace Backend
} // namespace Afina
