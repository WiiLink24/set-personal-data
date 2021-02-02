#include <ogcsys.h>

// This patch allows us to read tickets/TMDs/so forth.
static const u16 isfs_permissions_old[] = {0x428B, 0xD001, 0x2566};
static const u16 isfs_permissions_patch[] = {0x428B, 0xE001, 0x2566};

// This patch allows us to gain access to the AHBPROT register.
static const u16 ticket_check_old[] = {
    0x685B,         // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052, // movls r2, 0x1D8
    0x189B,         // adds r3, r3, r2; add offset of access rights field in TMD
    0x681B,         // ldr r3, [r3]   ; load access rights (haxxme!)
    0x4698,         // mov r8, r3  ; store it for the DVD video bitcheck later
    0x07DB          // lsls r3, r3, #31; check AHBPROT bit
};
static const u16 ticket_check_patch[] = {
    0x685B,         // ldr r3,[r3,#4] ; get TMD pointer
    0x22EC, 0x0052, // movls r2, 0x1D8
    0x189B,         // adds r3, r3, r2; add offset of access rights field in TMD
    0x23FF,         // li r3, 0xFF  ; <--- 0xFF gives us all access bits
    0x4698,         // mov r8, r3  ; store it for the DVD video bitcheck later
    0x07DB          // lsls r3, r3, #31; check AHBPROT bit
};

// If a new IOS patch is added, please update accordingly.
#define ISFS_PERMISSIONS_SIZE sizeof(isfs_permissions_patch)
#define TICKET_CHECK_SIZE sizeof(ticket_check_patch)

bool patch_memory_range(u32 *start, u32 *end, const u16 original_patch[],
                        const u16 new_patch[], u32 patch_size);
bool patch_ahbprot_reset();
bool apply_patches();
