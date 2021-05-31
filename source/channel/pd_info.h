#include <wchar.h>

struct PDInfoData {
    wchar_t user_first_name[32];
    wchar_t user_last_name[32];
    wchar_t user_email[128];
    wchar_t user_address[128];
};

bool PD_PopulateData();
bool PD_WriteData();
