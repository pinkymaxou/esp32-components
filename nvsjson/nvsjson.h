#ifndef _NVSJSON_H_
#define _NVSJSON_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "nvs.h"

typedef enum
{
    NVSJSON_ESETRET_OK = 0,
    NVSJSON_ESETRET_CannotSet = 1,
    NVSJSON_ESETRET_InvalidRange = 2,
    NVSJSON_ESETRET_ValidatorFailed = 3,

    NVSJSON_ESETRET_InvalidLibrarySettings = 4,
} NVSJSON_ESETRET;

typedef enum
{
    NVSJSON_EFLAGS_None = 0,
    NVSJSON_EFLAGS_Secret = 1,      // Indicate it cannot be retrieved, only set
    NVSJSON_EFLAGS_NeedsReboot = 2
} NVSJSON_EFLAGS;

typedef enum
{
    NVSJSON_ETYPE_Int32,
    NVSJSON_ETYPE_String,
    NVSJSON_ETYPE_Double
} NVSJSON_ETYPE;

typedef struct _NVSJSON_SSettingEntry NVSJSON_SSettingEntry;

typedef bool (*PtrValidateInt32)(const NVSJSON_SSettingEntry* pSettingEntry, int32_t value);
typedef bool (*PtrValidateDouble)(const NVSJSON_SSettingEntry* pSettingEntry, double value);
typedef bool (*PtrValidateString)(const NVSJSON_SSettingEntry* pSettingEntry, const char* value);

typedef union
{
    struct
    {
       int32_t min;
       int32_t max;
       int32_t defaultValue;
       PtrValidateInt32 ptrValidator;
    } sInt32;
    struct
    {
       double min;
       double max;
       double defaultValue;
       PtrValidateDouble ptrValidator;
    } sDouble;
    struct
    {
       const char* defaultValue;
       PtrValidateString ptrValidator;
    } sString;
} NVSJSON_UConfig;

typedef struct _NVSJSON_SSettingEntry
{
    const char* key;
    const char* desc;
    NVSJSON_ETYPE type;
    NVSJSON_UConfig uConfig;
    NVSJSON_EFLAGS flags;
} NVSJSON_SSettingEntry;

typedef struct
{
    const char* partitionName;

    const NVSJSON_SSettingEntry* settingEntries;
    uint32_t settingEntryCount;
} NVSJSON_SConfig;

typedef struct
{
    nvs_handle_t nvs;
    bool isInitialized;
    // Entries
    const NVSJSON_SConfig* config;
} NVSJSON_SHandle;

#define NVSJSON_GETVALUESTRING_MAXLEN (100)

#define NVSJSON_INITSTRING(_szKey, _szDesc, _szDefault, _eFlags) { .key = _szKey, .desc = _szDesc, .type = NVSJSON_ETYPE_String, .uConfig = { .sString = { .defaultValue = _szDefault, .ptrValidator = NULL } }, .flags = _eFlags }
#define NVSJSON_INITSTRING_VAL(_szKey, _szDesc, _szDefault, _ptrValidateString, _eFlags) { .key = _szKey, .desc = _szDesc, .type = NVSJSON_ETYPE_String, .uConfig = { .sString = { .defaultValue = _szDefault, .ptrValidator = _ptrValidateString } }, .flags = _eFlags }

#define NVSJSON_INITDOUBLE_RNG(_szKey, _szDesc, _dDefault, _dMin, _dMax, _eFlags) { .key = _szKey, .desc = _szDesc, .type = NVSJSON_ETYPE_Double, .uConfig = { .sDouble = { .min = _dMin, .max = _dMax, .defaultValue = _dDefault, .ptrValidator = NULL } }, .flags = _eFlags }
#define NVSJSON_INITDOUBLE_VAL(_szKey, _szDesc, _dDefault, _ptrValidateDouble, _eFlags) { .key = _szKey, .desc = _szDesc, .type = NVSJSON_ETYPE_Double, .uConfig = { .sDouble = { .defaultValue = _dDefault, .ptrValidator = _ptrValidateDouble } }, .flags = _eFlags }

#define NVSJSON_INITINT32_RNG(_szKey, _szDesc, _s32Default, _s32Min, _s32Max, _eFlags) { .key = _szKey, .desc = _szDesc, .type = NVSJSON_ETYPE_Int32, .uConfig = { .sInt32 = { .min = _s32Min, .max = _s32Max, .defaultValue = _s32Default, .ptrValidator = NULL } }, .flags = _eFlags }
#define NVSJSON_INITINT32_VAL(_szKey, _szDesc, _s32Default, _ptrValidateInt32, _eFlags) { .key = _szKey, .desc = _szDesc, .type = NVSJSON_ETYPE_Int32, .uConfig = { .sInt32 = { .defaultValue = _s32Default, .ptrValidator = _ptrValidateInt32 } }, .flags = _eFlags }

NVSJSON_ESETRET NVSJSON_Init(NVSJSON_SHandle* pHandle, const NVSJSON_SConfig* config);
NVSJSON_ESETRET NVSJSON_Load(NVSJSON_SHandle* pHandle);
NVSJSON_ESETRET NVSJSON_Save(NVSJSON_SHandle* pHandle);

int32_t NVSJSON_GetValueInt32(NVSJSON_SHandle* pHandle, uint16_t entry);
NVSJSON_ESETRET NVSJSON_SetValueInt32(NVSJSON_SHandle* pHandle, uint16_t entry, bool dry_run, int32_t new_value);

double NVSJSON_GetValueDouble(NVSJSON_SHandle* pHandle, uint16_t entry);
NVSJSON_ESETRET NVSJSON_SetValueDouble(NVSJSON_SHandle* pHandle, uint16_t entry, bool dry_run, double new_value);

void NVSJSON_GetValueString(NVSJSON_SHandle* pHandle, uint16_t entry, char* out_value, size_t* length);
NVSJSON_ESETRET NVSJSON_SetValueString(NVSJSON_SHandle* pHandle, uint16_t entry, bool dry_run, const char* value);

char* NVSJSON_ExportJSON(NVSJSON_SHandle* pHandle);
bool NVSJSON_ImportJSON(NVSJSON_SHandle* pHandle, const char* json);

#ifdef __cplusplus
}
#endif

#endif
