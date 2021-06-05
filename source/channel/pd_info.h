#include <wchar.h>

struct PDInfoData {
    wchar_t user_first_name[32];
    wchar_t user_last_name[32];
    wchar_t user_email_address[128];
    wchar_t user_home_address[128];
    wchar_t user_city[32];
};

bool PD_PopulateData();
bool PD_WriteData();
