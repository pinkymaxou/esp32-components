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

typedef bool (*validate_int32_fn)(const NVSJSON_SSettingEntry* setting_entry, int32_t value);
typedef bool (*validate_double_fn)(const NVSJSON_SSettingEntry* setting_entry, int32_t value);
typedef bool (*validate_string_fn)(const NVSJSON_SSettingEntry* setting_entry, const char* value);

typedef union
{
    struct
    {
       int32_t min;
       int32_t max;
       int32_t default_value;
       validate_int32_fn validator;
    } int32;
    struct
    {
       double min;
       double max;
       double default_value;
       validate_double_fn validator;
    } double_type;
    struct
    {
       const char* default_value;
       validate_string_fn validator;
    } string;
} NVSJSON_UConfig;

typedef struct _NVSJSON_SSettingEntry
{
    const char* key;
    const char* desc;
    NVSJSON_ETYPE type;
    NVSJSON_UConfig config;
    NVSJSON_EFLAGS flags;
} NVSJSON_SSettingEntry;

typedef struct
{
    const char* partition_name;

    const NVSJSON_SSettingEntry* setting_entries;
    uint32_t setting_entry_count;
} NVSJSON_SConfig;


typedef struct
{
	nvs_handle_t nvs;
    bool is_initialized;
	// Entries
    const NVSJSON_SConfig* config;
} NVSJSON_SHandle;

#define NVSJSON_GETVALUESTRING_MAXLEN (100)

#define NVSJSON_INITSTRING(_key, _desc, _default, _flags) { .key = _key,.desc = _desc, .type = NVSJSON_ETYPE_String, .config = { .string = { .default_value = _default, .validator = NULL } }, .flags = _flags }
#define NVSJSON_INITSTRING_VAL(_key, _desc, _default, _validator, _flags) { .key = _key,.desc = _desc, .type = NVSJSON_ETYPE_String, .config = { .string = { .default_value = _default, .validator = _validator } }, .flags = _flags }

#define NVSJSON_INITDOUBLE_RNG(_key, _desc, _default, _min, _max, _flags) { .key = _key,.desc = _desc, .type = NVSJSON_ETYPE_Double, .config = { .double_type = { .min = _min, .max = _max, .default_value = _default, .validator = NULL } }, .flags = _flags }
#define NVSJSON_INITDOUBLE_VAL(_key, _desc, _default, _validator, _flags) { .key = _key,.desc = _desc, .type = NVSJSON_ETYPE_Double, .config = { .double_type = { .default_value = _default, .validator = _validator } }, .flags = _flags }

#define NVSJSON_INITINT32_RNG(_key, _desc, _default, _min, _max, _flags) { .key = _key,.desc = _desc, .type = NVSJSON_ETYPE_Int32, .config = { .int32 = { .min = _min, .max = _max, .default_value = _default, .validator = NULL } }, .flags = _flags }
#define NVSJSON_INITINT32_VAL(_key, _desc, _default, _validator, _flags) { .key = _key,.desc = _desc, .type = NVSJSON_ETYPE_Int32, .config = { .int32 = { .default_value = _default, .validator = _validator } }, .flags = _flags }

NVSJSON_ESETRET NVSJSON_Init(NVSJSON_SHandle* handle, const NVSJSON_SConfig* config);
NVSJSON_ESETRET NVSJSON_Load(NVSJSON_SHandle* handle);
NVSJSON_ESETRET NVSJSON_Save(NVSJSON_SHandle* handle);

int32_t NVSJSON_GetValueInt32(NVSJSON_SHandle* handle, uint16_t entry);
NVSJSON_ESETRET NVSJSON_SetValueInt32(NVSJSON_SHandle* handle, uint16_t entry, bool is_dry_run, int32_t new_value);

double NVSJSON_GetValueDouble(NVSJSON_SHandle* handle, uint16_t entry);
NVSJSON_ESETRET NVSJSON_SetValueDouble(NVSJSON_SHandle* handle, uint16_t entry, bool is_dry_run, double new_value);

void NVSJSON_GetValueString(NVSJSON_SHandle* handle, uint16_t entry, char* out_value, size_t* length);
NVSJSON_ESETRET NVSJSON_SetValueString(NVSJSON_SHandle* handle, uint16_t entry, bool is_dry_run, const char* value);

char* NVSJSON_ExportJSON(NVSJSON_SHandle* handle);
bool NVSJSON_ImportJSON(NVSJSON_SHandle* handle, const char* json);

#ifdef __cplusplus
}
#endif

#endif