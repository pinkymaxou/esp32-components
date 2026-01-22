#include <stdio.h>
#include "nvsjson.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <assert.h>
#include "cJSON.h"
#include "esp_log.h"
#include <string.h>

#define TAG "nvsjson"

// JSON entries
#define JSON_ENTRIES_NAME "entries"

#define JSON_ENTRY_KEY_NAME "key"
#define JSON_ENTRY_VALUE_NAME "value"

#define JSON_ENTRY_INFO_NAME "info"

#define JSON_ENTRY_INFO_DESC_NAME "desc"
#define JSON_ENTRY_INFO_DEFAULT_NAME "default"
#define JSON_ENTRY_INFO_MIN_NAME "min"
#define JSON_ENTRY_INFO_MAX_NAME "max"
#define JSON_ENTRY_INFO_TYPE_NAME "type"
#define JSON_ENTRY_INFO_FLAG_REBOOT_NAME "flag_reboot"

static const NVSJSON_SSettingEntry* GetSettingEntry(NVSJSON_SHandle* handle, uint16_t entry);
static bool GetSettingEntryByKey(NVSJSON_SHandle* handle, const char* key, uint16_t* out_entry);

NVSJSON_ESETRET NVSJSON_Init(NVSJSON_SHandle* handle, const NVSJSON_SConfig* config)
{
    handle->is_initialized = false;

    if (config == NULL ||
        config->setting_entries == NULL ||
        config->setting_entry_count == 0 ||
        config->partition_name == NULL)
    {
        return NVSJSON_ESETRET_InvalidLibrarySettings;
    }

    handle->config = config;
    return NVSJSON_ESETRET_OK;
}

NVSJSON_ESETRET NVSJSON_Load(NVSJSON_SHandle* handle)
{
    if (handle->is_initialized)
    {
        nvs_close(handle->nvs);
        handle->is_initialized = false;
    }
    ESP_ERROR_CHECK(nvs_open(handle->config->partition_name, NVS_READWRITE, &handle->nvs));
    handle->is_initialized = true;
    return NVSJSON_ESETRET_OK;
}

NVSJSON_ESETRET NVSJSON_Save(NVSJSON_SHandle* handle)
{
    ESP_ERROR_CHECK(nvs_commit(handle->nvs));
    return NVSJSON_ESETRET_OK;
}

int32_t NVSJSON_GetValueInt32(NVSJSON_SHandle* handle, uint16_t entry)
{
    const NVSJSON_SSettingEntry* ent = GetSettingEntry(handle, entry);
    assert(ent != NULL && ent->type == NVSJSON_ETYPE_Int32);
    int32_t value = 0;
    if (nvs_get_i32(handle->nvs, ent->key, &value) == ESP_OK)
        return value;
    return ent->config.int32.default_value;
}

NVSJSON_ESETRET NVSJSON_SetValueInt32(NVSJSON_SHandle* handle, uint16_t entry, bool is_dry_run, int32_t new_value)
{
    const NVSJSON_SSettingEntry* ent = GetSettingEntry(handle, entry);
    assert(ent != NULL && ent->type == NVSJSON_ETYPE_Int32);

    if (ent->config.int32.validator != NULL)
    {
        if (!ent->config.int32.validator(ent, new_value))
            return NVSJSON_ESETRET_ValidatorFailed;
    }
    else
    {
        if (new_value < ent->config.int32.min || new_value > ent->config.int32.max)
            return NVSJSON_ESETRET_InvalidRange;
    }

    if (!is_dry_run)
    {
        const esp_err_t err = nvs_set_i32(handle->nvs, ent->key, new_value);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

double NVSJSON_GetValueDouble(NVSJSON_SHandle* handle, uint16_t entry)
{
    const NVSJSON_SSettingEntry* ent = GetSettingEntry(handle, entry);
    assert(ent != NULL && ent->type == NVSJSON_ETYPE_Double);
    double value = 0;
    size_t len = sizeof(double);
    if (nvs_get_blob(handle->nvs, ent->key, (void*)&value, &len) == ESP_OK)
        return value;
    return ent->config.double_type.default_value;
}

NVSJSON_ESETRET NVSJSON_SetValueDouble(NVSJSON_SHandle* handle, uint16_t entry, bool is_dry_run, double new_value)
{
    const NVSJSON_SSettingEntry* ent = GetSettingEntry(handle, entry);
    assert(ent != NULL && ent->type == NVSJSON_ETYPE_Double);

    // Default value if it cannot retrieve it ...
    if (ent->config.double_type.validator != NULL)
    {
        if (!ent->config.double_type.validator(ent, new_value))
            return NVSJSON_ESETRET_ValidatorFailed;
    }
    else
    {
        if (new_value < ent->config.double_type.min || new_value > ent->config.double_type.max)
            return NVSJSON_ESETRET_InvalidRange;
    }

    if (!is_dry_run)
    {
        const esp_err_t err = nvs_set_blob(handle->nvs, ent->key, &new_value, sizeof(double));
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

void NVSJSON_GetValueString(NVSJSON_SHandle* handle, uint16_t entry, char* out_value, size_t* length)
{
    const NVSJSON_SSettingEntry* ent = GetSettingEntry(handle, entry);
    assert(ent != NULL && ent->type == NVSJSON_ETYPE_String);

    if (nvs_get_str(handle->nvs, ent->key, out_value, length) != ESP_OK)
    {
        if (ent->config.string.default_value != NULL)
        {
            *length = strlen(ent->config.string.default_value);
            strncpy(out_value, ent->config.string.default_value, *length);
        }
    }
}

NVSJSON_ESETRET NVSJSON_SetValueString(NVSJSON_SHandle* handle, uint16_t entry, bool is_dry_run, const char* value)
{
    const NVSJSON_SSettingEntry* ent = GetSettingEntry(handle, entry);
    assert(ent != NULL && ent->type == NVSJSON_ETYPE_String);

    // Default value if it cannot retrieve it ...
    if (ent->config.string.validator != NULL)
    {
        if (!ent->config.string.validator(ent, value))
            return NVSJSON_ESETRET_ValidatorFailed;
    }

    if (!is_dry_run)
    {
        const esp_err_t err = nvs_set_str(handle->nvs, ent->key, value);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (err != ESP_OK)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

char* NVSJSON_ExportJSON(NVSJSON_SHandle* handle)
{
    cJSON* root = cJSON_CreateObject();
    if (root == NULL)
        goto ERROR;

    cJSON* entries = cJSON_AddArrayToObject(root, JSON_ENTRIES_NAME);

    for(int i = 0; i < handle->config->setting_entry_count; i++)
    {
        uint16_t entry_idx = (uint16_t)i;
        const NVSJSON_SSettingEntry* entry = GetSettingEntry(handle, entry_idx);

        cJSON* entry_json = cJSON_CreateObject();
        cJSON_AddItemToObject(entry_json, JSON_ENTRY_KEY_NAME, cJSON_CreateString(entry->key));

        cJSON* entry_info_json = cJSON_CreateObject();

        // Description and flags apply everywhere
        cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_DESC_NAME, cJSON_CreateString(entry->desc));
        cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_FLAG_REBOOT_NAME, cJSON_CreateNumber((entry->flags & NVSJSON_EFLAGS_NeedsReboot)? 1 : 0));

        if (entry->type == NVSJSON_ETYPE_Int32)
        {
            if ((entry->flags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
                cJSON_AddItemToObject(entry_json, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(NVSJSON_GetValueInt32(handle, entry_idx)));

            cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(entry->config.int32.default_value));

            if (entry->config.int32.validator == NULL)
            {
                cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(entry->config.int32.min));
                cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(entry->config.int32.max));
            }
            cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("int32"));
        }
        else if (entry->type == NVSJSON_ETYPE_Double)
        {
            if ((entry->flags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
                cJSON_AddItemToObject(entry_json, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(NVSJSON_GetValueDouble(handle, entry_idx)));

            cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(entry->config.double_type.default_value));
            if (entry->config.double_type.validator == NULL)
            {
                cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(entry->config.double_type.min));
                cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(entry->config.double_type.max));
            }
            cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("double"));
        }
        else if (entry->type == NVSJSON_ETYPE_String)
        {
            char value[NVSJSON_GETVALUESTRING_MAXLEN+1] = {0,};
            size_t length = NVSJSON_GETVALUESTRING_MAXLEN;
            if ((entry->flags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
            {
                NVSJSON_GetValueString(handle, entry_idx, value, &length);
                cJSON_AddItemToObject(entry_json, JSON_ENTRY_VALUE_NAME, cJSON_CreateString(value));
            }
            cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateString(entry->config.string.default_value));
            cJSON_AddItemToObject(entry_info_json, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("string"));
        }

        cJSON_AddItemToObject(entry_json, JSON_ENTRY_INFO_NAME, entry_info_json);

        cJSON_AddItemToArray(entries, entry_json);
    }
    char* str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return str;
    ERROR:
    cJSON_Delete(root);
    return NULL;
}

bool NVSJSON_ImportJSON(NVSJSON_SHandle* handle, const char* json)
{
    bool ret = true;
    cJSON* root = cJSON_Parse(json);

    cJSON* entries_array = cJSON_GetObjectItem(root, JSON_ENTRIES_NAME);
    if (!cJSON_IsArray(entries_array))
    {
        ESP_LOGE(TAG, "Entries array is not valid");
        goto ERROR;
    }

    for(int pass = 0; pass < 2; pass++)
    {
        const bool is_dry_run = pass == 0;

        for(int i = 0; i < cJSON_GetArraySize(entries_array); i++)
        {
            cJSON* entry_json = cJSON_GetArrayItem(entries_array, i);

            cJSON* key_json = cJSON_GetObjectItem(entry_json, JSON_ENTRY_KEY_NAME);
            if (key_json == NULL || !cJSON_IsString(key_json))
            {
                ESP_LOGE(TAG, "Cannot find JSON key element");
                goto ERROR;
            }

            cJSON* value_json = cJSON_GetObjectItem(entry_json, JSON_ENTRY_VALUE_NAME);
            if (value_json == NULL)
            {
                // We just ignore changing the setting if the value property is not there.
                // it allows us to handle secret cases.
                ESP_LOGD(TAG, "JSON value is not there, skipping it");
                continue;
            }

            uint16_t entry_idx;
            if (!GetSettingEntryByKey(handle, key_json->valuestring, &entry_idx))
            {
                ESP_LOGE(TAG, "Key: '%s' is not valid", key_json->valuestring);
                goto ERROR;
            }

            const NVSJSON_SSettingEntry* setting_entry = GetSettingEntry(handle, entry_idx);

            if (setting_entry->type == NVSJSON_ETYPE_Int32)
            {
                if (!cJSON_IsNumber(value_json))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                int32_t value = value_json->valueint;
                NVSJSON_ESETRET set_ret;
                if ((set_ret = NVSJSON_SetValueInt32(handle, entry_idx, is_dry_run, value)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, is_dry_run: %d, ret: %d", setting_entry->key, is_dry_run, set_ret);
                    goto ERROR;
                }
            }
            else if (setting_entry->type == NVSJSON_ETYPE_Double)
            {
                if (!cJSON_IsNumber(value_json))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                double value = (float)value_json->valuedouble;
                NVSJSON_ESETRET set_ret;
                if ((set_ret = NVSJSON_SetValueDouble(handle, entry_idx, is_dry_run, value)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, is_dry_run: %d, ret: %d", setting_entry->key, is_dry_run, set_ret);
                    goto ERROR;
                }
            }
            else if (setting_entry->type == NVSJSON_ETYPE_String)
            {
                if (!cJSON_IsString(value_json))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a string");
                    goto ERROR;
                }

                const char* str = value_json->valuestring;
                NVSJSON_ESETRET set_ret;
                if ((set_ret = NVSJSON_SetValueString(handle, entry_idx, is_dry_run, str)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, is_dry_run: %d, ret: %d", setting_entry->key, is_dry_run, set_ret);
                    goto ERROR;
                }
            }
        }
    }

    ret = true;
    ESP_LOGI(TAG, "Import JSON completed");
    goto END;
    ERROR:
    ret = false;
    END:
    cJSON_free(root);
    return ret;
}

static const NVSJSON_SSettingEntry* GetSettingEntry(NVSJSON_SHandle* handle, uint16_t entry)
{
    if ((int)entry >= handle->config->setting_entry_count)
        return NULL;
    return &handle->config->setting_entries[(int)entry];
}

static bool GetSettingEntryByKey(NVSJSON_SHandle* handle, const char* key, uint16_t* out_entry)
{
    for(int i = 0; i < handle->config->setting_entry_count; i++)
    {
        if (strcmp(handle->config->setting_entries[i].key, key) == 0)
        {
            *out_entry = (uint16_t)i;
            return true;
        }
    }
    return false;
}