// This is a generated file! Please edit source .ksy file and use kaitai-struct-compiler to rebuild

#include "pd.h"
#include "kaitai/exceptions.h"

pd_t::pd_t(kaitai::kstream* p__io, kaitai::kstruct* p__parent, pd_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = this;
    m_file_header = nullptr;
    m__io__raw_file_header = nullptr;
    m_info = nullptr;
    m__io__raw_info = nullptr;
    m_kana = nullptr;
    m__io__raw_kana = nullptr;
    _read();
}

void pd_t::_read() {
    m__raw_file_header = m__io->read_bytes(128);
    m__io__raw_file_header = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_file_header));
    m_file_header = std::unique_ptr<header_t>(new header_t(m__io__raw_file_header.get(), this, m__root));
    m__raw_info = m__io->read_bytes(9583);
    m__io__raw_info = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_info));
    m_info = std::unique_ptr<info_block_t>(new info_block_t(m__io__raw_info.get(), this, m__root));
    m__raw_kana = m__io->read_bytes(1036);
    m__io__raw_kana = std::unique_ptr<kaitai::kstream>(new kaitai::kstream(m__raw_kana));
    m_kana = std::unique_ptr<kana_block_t>(new kana_block_t(m__io__raw_kana.get(), this, m__root));
}

pd_t::~pd_t() {
    _clean_up();
}

void pd_t::_clean_up() {
}

pd_t::header_t::header_t(kaitai::kstream* p__io, pd_t* p__parent, pd_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void pd_t::header_t::_read() {
    m_magic = m__io->read_bytes(4);
    if (!(magic() == std::string("\x50\x44\x46\x46", 4))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x50\x44\x46\x46", 4), magic(), _io(), std::string("/types/header/seq/0"));
    }
    m_version = m__io->read_bytes(1);
    if (!(version() == std::string("\x02", 1))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x02", 1), version(), _io(), std::string("/types/header/seq/1"));
    }
}

pd_t::header_t::~header_t() {
    _clean_up();
}

void pd_t::header_t::_clean_up() {
}

pd_t::info_block_t::info_block_t(kaitai::kstream* p__io, pd_t* p__parent, pd_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void pd_t::info_block_t::_read() {
    m_magic = m__io->read_bytes(4);
    if (!(magic() == std::string("\x49\x4E\x46\x4F", 4))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x49\x4E\x46\x4F", 4), magic(), _io(), std::string("/types/info_block/seq/0"));
    }
    m_block_size = m__io->read_u4be();
    m_intentionally_null = m__io->read_u4be();
    m_preset_values = m__io->read_bytes(3);
    if (!(preset_values() == std::string("\x01\x08\x00", 3))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x01\x08\x00", 3), preset_values(), _io(), std::string("/types/info_block/seq/3"));
    }
    m_first_timestamp = m__io->read_u8be();
    m_profile_name = kaitai::kstream::bytes_to_str(m__io->read_bytes(56), std::string("UTF-16BE"));
    m_surname = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
    m_first_name = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
    m_postal_code = kaitai::kstream::bytes_to_str(m__io->read_bytes(34), std::string("UTF-16BE"));
    m_padding_null = m__io->read_u2be();
    m_state_or_prefecture = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
    m_city = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
    m_home_address = kaitai::kstream::bytes_to_str(m__io->read_bytes(256), std::string("UTF-16BE"));
    m_apartment_number = kaitai::kstream::bytes_to_str(m__io->read_bytes(256), std::string("UTF-16BE"));
    m_phone_number = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
    m_email_address = kaitai::kstream::bytes_to_str(m__io->read_bytes(256), std::string("UTF-16BE"));
    m_padding = m__io->read_bytes(1);
    if (!(padding() == std::string("\x00", 1))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x00", 1), padding(), _io(), std::string("/types/info_block/seq/16"));
    }
    m_second_timestamp = m__io->read_u8be();
}

pd_t::info_block_t::~info_block_t() {
    _clean_up();
}

void pd_t::info_block_t::_clean_up() {
}

pd_t::kana_block_t::kana_block_t(kaitai::kstream* p__io, pd_t* p__parent, pd_t* p__root) : kaitai::kstruct(p__io) {
    m__parent = p__parent;
    m__root = p__root;
    _read();
}

void pd_t::kana_block_t::_read() {
    m_magic = m__io->read_bytes(4);
    if (!(magic() == std::string("\x4B\x41\x4E\x41", 4))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x4B\x41\x4E\x41", 4), magic(), _io(), std::string("/types/kana_block/seq/0"));
    }
    m_block_size = m__io->read_u4be();
    m_preset_value = m__io->read_bytes(4);
    if (!(preset_value() == std::string("\x01\x00\x00\x00", 4))) {
        throw kaitai::validation_not_equal_error<std::string>(std::string("\x01\x00\x00\x00", 4), preset_value(), _io(), std::string("/types/kana_block/seq/2"));
    }
    m_surname = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
    m_first_name = kaitai::kstream::bytes_to_str(m__io->read_bytes(64), std::string("UTF-16BE"));
}

pd_t::kana_block_t::~kana_block_t() {
    _clean_up();
}

void pd_t::kana_block_t::_clean_up() {
}
