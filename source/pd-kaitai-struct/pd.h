#pragma once

// This is a generated file! Please edit source .ksy file and use
// kaitai-struct-compiler to rebuild

#include "kaitai/kaitaistruct.h"
#include <memory>
#include <stdint.h>

#if KAITAI_STRUCT_VERSION < 9000L
#error                                                                         \
    "Incompatible Kaitai Struct C++/STL API: version 0.9 or later is required"
#endif

class pd_t : public kaitai::kstruct {

  public:
    class header_t;
    class info_block_t;
    class kana_block_t;

    pd_t(kaitai::kstream *p__io, kaitai::kstruct *p__parent = nullptr,
         pd_t *p__root = nullptr);

  private:
    void _read();
    void _clean_up();

  public:
    ~pd_t();

    class header_t : public kaitai::kstruct {

      public:
        header_t(kaitai::kstream *p__io, pd_t *p__parent = nullptr,
                 pd_t *p__root = nullptr);

      private:
        void _read();
        void _clean_up();

      public:
        ~header_t();

      private:
        std::string m_magic;
        std::string m_version;
        pd_t *m__root;
        pd_t *m__parent;

      public:
        std::string magic() const { return m_magic; }

        /**
         * <| This is checked by all known PD SDKs to be less than or equal
         * to 2. It's assumed that 1 exists with lesser features - presumably
         * without profiles.
         */
        std::string version() const { return m_version; }
        pd_t *_root() const { return m__root; }
        pd_t *_parent() const { return m__parent; }
    };

    class info_block_t : public kaitai::kstruct {

      public:
        info_block_t(kaitai::kstream *p__io, pd_t *p__parent = nullptr,
                     pd_t *p__root = nullptr);

      private:
        void _read();
        void _clean_up();

      public:
        ~info_block_t();

      private:
        std::string m_magic;
        uint32_t m_block_size;
        uint32_t m_intentionally_null;
        std::string m_preset_values;
        uint64_t m_first_timestamp;
        std::string m_profile_name;
        std::string m_surname;
        std::string m_first_name;
        std::string m_postal_code;
        uint16_t m_padding_null;
        std::string m_state_or_prefecture;
        std::string m_city;
        std::string m_home_address;
        std::string m_apartment_number;
        std::string m_phone_number;
        std::string m_email_address;
        std::string m_padding;
        uint64_t m_second_timestamp;
        pd_t *m__root;
        pd_t *m__parent;

      public:
        std::string magic() const { return m_magic; }
        uint32_t block_size() const { return m_block_size; }
        uint32_t intentionally_null() const { return m_intentionally_null; }

        /**
         * These do not appear to be read yet are set if uninitialized.
         */
        std::string preset_values() const { return m_preset_values; }
        uint64_t first_timestamp() const { return m_first_timestamp; }

        /**
         * This is used within additional profiles. We do not support it.
         */
        std::string profile_name() const { return m_profile_name; }

        /**
         * Tyically written with Romaji.
         */
        std::string surname() const { return m_surname; }

        /**
         * Tyically written with Romaji.
         */
        std::string first_name() const { return m_first_name; }
        std::string postal_code() const { return m_postal_code; }
        uint16_t padding_null() const { return m_padding_null; }

        /**
         * <| The first two bytes of this string (overlapping with padding_null
         * above) are hardcoded to 0x019e. However, the 0x9e is used within the
         * prefecture's name.
         */
        std::string state_or_prefecture() const {
            return m_state_or_prefecture;
        }
        std::string city() const { return m_city; }
        std::string home_address() const { return m_home_address; }
        std::string apartment_number() const { return m_apartment_number; }
        std::string phone_number() const { return m_phone_number; }
        std::string email_address() const { return m_email_address; }
        std::string padding() const { return m_padding; }
        uint64_t second_timestamp() const { return m_second_timestamp; }
        pd_t *_root() const { return m__root; }
        pd_t *_parent() const { return m__parent; }
    };

    class kana_block_t : public kaitai::kstruct {

      public:
        kana_block_t(kaitai::kstream *p__io, pd_t *p__parent = nullptr,
                     pd_t *p__root = nullptr);

      private:
        void _read();
        void _clean_up();

      public:
        ~kana_block_t();

      private:
        std::string m_magic;
        uint32_t m_block_size;
        std::string m_preset_value;
        std::string m_surname;
        std::string m_first_name;
        pd_t *m__root;
        pd_t *m__parent;

      public:
        std::string magic() const { return m_magic; }
        uint32_t block_size() const { return m_block_size; }

        /**
         * <| The literal 0x1 is additionally written over 1 byte via memcpy. As
         * this area has been memset to 0 prior to run and never modified, we
         * assume the next few bytes are null as well.
         */
        std::string preset_value() const { return m_preset_value; }
        std::string surname() const { return m_surname; }
        std::string first_name() const { return m_first_name; }
        pd_t *_root() const { return m__root; }
        pd_t *_parent() const { return m__parent; }
    };

  private:
    std::unique_ptr<header_t> m_file_header;
    std::unique_ptr<info_block_t> m_info;
    std::unique_ptr<kana_block_t> m_kana;
    pd_t *m__root;
    kaitai::kstruct *m__parent;
    std::string m__raw_file_header;
    std::unique_ptr<kaitai::kstream> m__io__raw_file_header;
    std::string m__raw_info;
    std::unique_ptr<kaitai::kstream> m__io__raw_info;
    std::string m__raw_kana;
    std::unique_ptr<kaitai::kstream> m__io__raw_kana;

  public:
    header_t *file_header() const { return m_file_header.get(); }
    info_block_t *info() const { return m_info.get(); }
    kana_block_t *kana() const { return m_kana.get(); }
    pd_t *_root() const { return m__root; }
    kaitai::kstruct *_parent() const { return m__parent; }
    std::string _raw_file_header() const { return m__raw_file_header; }
    kaitai::kstream *_io__raw_file_header() const {
        return m__io__raw_file_header.get();
    }
    std::string _raw_info() const { return m__raw_info; }
    kaitai::kstream *_io__raw_info() const { return m__io__raw_info.get(); }
    std::string _raw_kana() const { return m__raw_kana; }
    kaitai::kstream *_io__raw_kana() const { return m__io__raw_kana.get(); }
};
