//===----------------------------------------------------------------------===//
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include<type_traits>
#pragma once
#include <malloc.h>
#include <string.h>
namespace Concurrency {

    template<class T>
        class GMACAllocator {
        public:
            T* allocate(unsigned n) {
                return new T[n];
            }
        };

    template<class T>
        class GMACDeleter {
        public:
            void operator()(T* ptr) {
                delete [] ptr;
            }
        };

    template <typename T>
        class _data_host: public std::shared_ptr<T> {
        public:
            _data_host(const _data_host &other):std::shared_ptr<T>(other) {}
            _data_host(std::nullptr_t x = nullptr):std::shared_ptr<T>(nullptr) {}
        };
    // Wrap a shared pointer to the gmac buffer and the cache state. Act
    // as if a shared pointer to the gmac buffer itself.
    template <typename T>
        class _data_host_view {
        private:
            typedef typename std::remove_const<T>::type nc_T;
            friend _data_host_view<const T>;

            __attribute__((cpu)) std::shared_ptr<nc_T> gmac_buffer;
            __attribute__((cpu)) T* home_ptr;
            __attribute__((cpu)) size_t buffer_size;

        public:
            _data_host_view(nc_T* cache, T* home, size_t size) :
                gmac_buffer(cache), home_ptr(home), buffer_size(size) {}

            template <class Deleter>
                _data_host_view(nc_T* cache, Deleter d, T* home, size_t size) :
                    gmac_buffer(cache, d), home_ptr(home), buffer_size(size) {}

            template <class Deleter>
                _data_host_view(nc_T* cache, Deleter d) :
                    gmac_buffer(cache, d), home_ptr(nullptr), buffer_size(0) {}

            _data_host_view(const _data_host_view<T> &other) :
                gmac_buffer(other.gmac_buffer),
                home_ptr(other.home_ptr), buffer_size(other.buffer_size) {}

            _data_host_view(const _data_host<T> &other) :
                gmac_buffer(other), home_ptr(nullptr), buffer_size(0) {}

            _data_host_view(std::nullptr_t x = nullptr):gmac_buffer(nullptr), buffer_size(0) {}

            template <class = typename std::enable_if<std::is_const<T>::value>::type>
                _data_host_view(const _data_host<nc_T> &other) :
                    gmac_buffer(other), home_ptr(nullptr), buffer_size(0) {}

            template <class = typename std::enable_if<std::is_const<T>::value>::type>
                _data_host_view(const _data_host_view<nc_T> &other) :
                    gmac_buffer(other.gmac_buffer),
                    home_ptr(other.home_ptr), buffer_size(other.buffer_size) {}

            void reset() {
                gmac_buffer.reset();
            }

            void refresh() const {}

            //If the cache is currently owned by the gmac buffer, copy it back to the host
            //buffer and change the state to shared.
            void synchronize() const {}

            //Check the gmac buffer use count to see if we are the last copy of it. This
            //is used to check if we are the last copy of an array_view and need to
            //implicitly synchronize.
            bool is_last() const { return gmac_buffer.unique(); }

            //Return the home location ptr, synchronizing first if necessary. The pointer
            //returned is mutable, so we set it as host owned. If this is an array or array_view
            //without a host buffer just return a pointer to the gmac buffer.
            T* get_mutable() const {
                if (home_ptr) {
                    synchronize();
                    return home_ptr;
                } else {
                    return gmac_buffer.get();
                }
            }
            T* get() const {
                return get_mutable();
            }
            T* get_data() const {
                return get_mutable();
            }

            //Return the home location ptr, synchronizing first if necessary. The pointer
            //returned is const, so we can leave the cache in a shared state. If this is
            //an array or array_view without a host buffer just return a pointer to the gmac buffer.
            const T* get() {
                if (home_ptr) {
                    synchronize();
                    return home_ptr;
                } else {
                    return gmac_buffer.get();
                }
            }
        };
} // namespace Concurrency
