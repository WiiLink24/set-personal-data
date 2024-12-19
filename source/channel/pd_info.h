#include <wchar.h>

struct PDInfoData {
    wchar_t user_first_name[32];
    wchar_t user_last_name[32];
    wchar_t user_email_address[128];
    wchar_t user_home_address[128];
    wchar_t user_city[32];
    wchar_t user_phone_number[32];
    wchar_t user_zip_code[17];
    wchar_t user_pin[8];
    wchar_t user_state[32];
    wchar_t user_apt_number[128];
    bool passwordProtected;
};

bool PD_PopulateData();
bool PD_WriteData();
