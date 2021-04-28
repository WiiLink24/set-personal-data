// We need gettime from LWP to match OSGetTime.
#include <codecvt>
#include <iomanip>
#include <iostream>
#include <kaitai/kaitaistream.h>
#include <ogc/lwp_watchdog.h>
#include <pd-kaitai-struct/pd.h>
#include <string>

extern "C" {
#include "helpers.h"
}

#include "pd_data.h"
#include "pd_info.h"

struct PDInfoData currentData = {};
std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> convert;

std::string WcharToStringPipeline(const wchar_t *convertee) {
    // wchar_t -> string
    return convert.to_bytes(convertee);
}

const wchar_t *StringToWcharPipeline(std::string convertee) {
    // string -> wstring*
    std::wstring *intermediate =
        new std::wstring(convert.from_bytes(convertee));

    // wstring* -> wchar_t
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

        // Populate current data
        const wchar_t *first_name =
            StringToWcharPipeline(infoBlock->first_name());
        const wchar_t *last_name = StringToWcharPipeline(infoBlock->surname());
        const wchar_t *email_address =
            StringToWcharPipeline(infoBlock->email_address());

        wcscpy(currentData.user_first_name, first_name);
        wcscpy(currentData.user_last_name, last_name);
        wcscpy(currentData.user_email, email_address);
    } catch (const std::exception &e) {
        std::cout << "A C++ exception occurred." << std::endl;
        std::cout << e.what() << std::endl;
        return false;
    }

    return true;
}
