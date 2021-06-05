meta:
  id: pd
  file-extension: dat
  endian: be
seq:
  - id: file_header
    type: header
    size: 0x80
  - id: info
    type: info_block
    size: 0x256F
  - id: kana
    type: kana_block
    size: 0x040c
types:
  header:
    seq:
      - id: magic
        contents: 'PDFF'
      - id: version
        contents: [2]
        doc: <|
          This is checked by all known PD SDKs to be less than or equal to 2.
          It's assumed that 1 exists with lesser features - presumably without profiles.
  info_block:
    seq:
      - id: magic
        contents: 'INFO'
      - id: block_size
        type: u4
      - id: intentionally_null
        type: u4
      - id: preset_values
        contents: [0x01, 0x08, 0x0]
        doc: These do not appear to be read yet are set if uninitialized.
      - id: first_timestamp
        type: u8
      - id: profile_name
        type: str
        size: 56
        encoding: UTF-16BE
        doc: This is used within additional profiles. We do not support it.
      - id: surname
        type: str
        size: 64
        encoding: UTF-16BE
        doc: Tyically written with Romaji.
      - id: first_name
        type: str
        size: 64
        encoding: UTF-16BE
        doc: Tyically written with Romaji.
      - id: postal_code
        type: str
        size: 34
        encoding: UTF-16BE
      - id: padding_null
        type: u2
      - id: state_or_prefecture
        type: str
        size: 64
        encoding: UTF-16BE
        doc: <|
            The first two bytes of this string (overlapping with padding_null above)
            are hardcoded to 0x019e.
            However, the 0x9e is used within the prefecture's name.

      - id: city
        type: str
        size: 64
        encoding: UTF-16BE
      - id: home_address
        type: str
        size: 256
        encoding: UTF-16BE
      - id: apartment_number
        type: str
        size: 256
        encoding: UTF-16BE
      - id: phone_number
        type: str
        size: 64
        encoding: UTF-16BE
      - id: email_address
        type: str
        size: 256
        encoding: UTF-16BE
      - id: padding
        contents: [0]
      - id: second_timestamp
        type: u8

  kana_block:
    seq:
      - id: magic
        contents: 'KANA'
      - id: block_size
        type: u4
      - id: preset_value
        contents: [0x01, 0x00, 0x00, 0x00]
        doc: <|
          The literal 0x1 is additionally written over 1 byte via memcpy.
          As this area has been memset to 0 prior to run and never modified,
          we assume the next few bytes are null as well.
      - id: surname
        type: str
        size: 64
        encoding: UTF-16BE
      - id: first_name
        type: str
        size: 64
        encoding: UTF-16BE
