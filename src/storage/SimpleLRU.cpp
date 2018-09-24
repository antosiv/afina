#include "SimpleLRU.h"

namespace Afina {
    namespace Backend {

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Put(const std::string &key, const std::string &value) {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                long int req_space = value.size() - match->second->value.size();

                move_to_head(key);

                clear_space(value.size() - match->second->value.size());

                size += req_space;
                _lru_head->value = value;

            } else {

                clear_space(value.size() + key.size());

                if (_lru_head == nullptr) {

                    _lru_head = new lru_node;
                    _lru_tail = _lru_head;

                } else {

                    auto tmp = new lru_node;
                    _lru_head->next = tmp;
                    tmp->prev = _lru_head;
                    _lru_head = tmp;

                }

                _lru_head->key = key;
                _lru_head->value = value;
                size += _lru_head->key.size() + _lru_head->value.size();
                _lru_index[std::reference_wrapper<const std::string>(_lru_head->key)] = _lru_head;

            }

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::PutIfAbsent(const std::string &key, const std::string &value) {

            if (_lru_index.find(std::reference_wrapper<const std::string>(key)) == _lru_index.end()) Put(key, value);
            else return false;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Set(const std::string &key, const std::string &value) {

            if (_lru_index.find(std::reference_wrapper<const std::string>(key)) != _lru_index.end()) Put(key, value);
            else return false;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Delete(const std::string &key) {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                if (match->second->prev != nullptr) match->second->prev->next = match->second->next;
                else _lru_tail = match->second->next;

                if (match->second->next != nullptr) match->second->next->prev = match->second->prev;
                else _lru_head = match->second->prev;

                size -= match->second->key.size() + match->second->value.size();
                _lru_index.erase(match);
                delete match->second;

            } else return false;

            return true;
        }

// See MapBasedGlobalLockImpl.h
        bool SimpleLRU::Get(const std::string &key, std::string &value) const {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match != _lru_index.end()) {

                move_to_head(key);
                value = match->second->value;


            } else return false;

            return true;
        }

        bool SimpleLRU::move_to_head(const std::string &key) const {

            auto match = _lru_index.find(std::reference_wrapper<const std::string>(key));

            if (match->second->key == _lru_tail->key and _lru_tail != _lru_head) {

                auto tmp = match->second;
                _lru_tail = _lru_tail->next;
                _lru_tail->prev = nullptr;
                tmp->next = nullptr;
                tmp->prev = _lru_head;
                _lru_head->next = tmp;
                _lru_head = tmp;

            } else if (_lru_head->key != match->second->key) {

                match->second->prev->next = match->second->next;
                match->second->next->prev = match->second->prev;
                match->second->prev = _lru_head;
                _lru_head->next = match->second;
                _lru_head = match->second;

            }

            return true;
        }

        bool SimpleLRU::clear_space(long int req_space) {

            if (req_space > _max_size) return false;
            else if (req_space < 0) return true;

            while (req_space + size > _max_size) {

                auto tmp = _lru_tail->next;
                size -= _lru_tail->key.size() + _lru_tail->value.size();
                tmp->prev = nullptr;
                _lru_index.erase(_lru_index.find(std::reference_wrapper<const std::string>(_lru_tail->key)));
                delete _lru_tail;
                _lru_tail = tmp;

            }

            return true;
        }

    } // namespace Backend
} // namespace Afina
