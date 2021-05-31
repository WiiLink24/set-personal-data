// We need gettime from LWP to match OSGetTime.
#include <codecvt>
#include <iomanip>
#include <iostream>
#include <kaitai/kaitaistream.h>
#include <ogc/lwp_watchdog.h>
#include <pd-kaitai-struct/pd.h>
#include <stdio.h>
#include <string.h>
#include <string>

extern "C" {
#include "helpers.h"
}

#include "pd_data.h"
#include "pd_info.h"

struct PDInfoData currentData = {};

std::string *WcharToStringPipeline(const wchar_t *convertee, size_t expected) {
    std::wstring_convert<std::codecvt_utf16<wchar_t>, wchar_t> convert;

    // wchar_t* -> std::string*
    std::string *intermediate = new std::string(convert.to_bytes(convertee));

    // Resize to full
    intermediate->resize(expected);

    return intermediate;
}

const wchar_t *StringToWcharPipeline(std::string convertee) {
    std::wstring_convert<std::codecvt_utf8<wchar_t>> convert;

    // std::string -> std::wstring
    std::wstring *intermediate =
        new std::wstring(convert.from_bytes(convertee));

    // std::wstring -> wchar_t*
    return intermediate->c_str();
}

bool PD_PopulateData() {
    void *pdLocation = PD_GetFileContents();
    if (pdLocation == NULL) {
        std::cout << "Unable to read pd.dat!" << std::endl;
        return false;
    }

    try {
        // Load decrypted contents into stream
        std::stringstream is;
        is.write((const char *)pdLocation, 0x4000);
        kaitai::kstream ks(&is);

        // Attempt to parse via the provided Kaitai Struct
        pd_t data(&ks);
        pd_t::info_block_t *infoBlock = data.info();

        // Convert current data from pd.dat
        const wchar_t *first_name =
            StringToWcharPipeline(infoBlock->first_name());
        const wchar_t *last_name = StringToWcharPipeline(infoBlock->surname());
        const wchar_t *email_address =
            StringToWcharPipeline(infoBlock->email_address());
        const wchar_t *home_address = StringToWcharPipeline(infoBlock->address());
        const wchar_t *city = StringToWcharPipeline(infoBlock->city());

        // Populate data
        wcsncpy(currentData.user_first_name, first_name, 32);
        wcsncpy(currentData.user_last_name, last_name, 32);
        wcsncpy(currentData.user_email, email_address, 128);
        wcsncpy(currentData.user_address, home_address, 128);
        wcsncpy(currentData.user_city, city, 32);
    } catch (const std::exception &e) {
        std::cout << "A C++ exception occurred." << std::endl;
        std::cout << e.what() << std::endl;
        return false;
    }

    return true;
}

bool PD_WriteData() {
    void *pdLocation = PD_GetFileContents();
    if (pdLocation == NULL) {
        std::cout << "Unable to read pd.dat!" << std::endl;
        return false;
    }

    try {
        // All string lengths must be twice such due to being UTF-16 BE.
        std::string *first_name =
            WcharToStringPipeline(currentData.user_first_name, 32 * 2);
        std::string *last_name =
            WcharToStringPipeline(currentData.user_last_name, 32 * 2);
        std::string *email_address =
            WcharToStringPipeline(currentData.user_email, 128 * 2);
        std::string *home_address =
            WcharToStringPipeline(currentData.user_address, 128 * 2);
        std::string *city =
            WcharToStringPipeline(currentData.user_city, 32 * 2);

        // We specify specific offsets within the file.
        // TODO: Replace with Kaitai serialization?
        void *filePointer = PD_GetFileContents();

        // Update first name
        // INFO block
        memcpy(filePointer + 207, last_name->c_str(), last_name->length());
        // KANA block
        memcpy(filePointer + 9723, last_name->c_str(), last_name->length());

        // Update last name
        // INFO block
        memcpy(filePointer + 271, first_name->c_str(), first_name->length());
        // KANA block
        memcpy(filePointer + 9787, first_name->c_str(), first_name->length());

        // Update email
        memcpy(filePointer + 1075, email_address->c_str(),
               email_address->length());

        // Update home address
        memcpy(filePointer + 499, home_address->c_str(),
               home_address->length());

        // Update city
        memcpy(filePointer + 435, city->c_str(),
               city->length());

        return PD_SaveFileContents();
    } catch (const std::exception &e) {
        std::cout << "A C++ exception occurred." << std::endl;
        std::cout << e.what() << std::endl;
        return false;
    }
    return true;
}
