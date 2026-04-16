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

static const NVSJSON_SSettingEntry* GetSettingEntry(NVSJSON_SHandle* pHandle, uint16_t entry);
static bool GetSettingEntryByKey(NVSJSON_SHandle* pHandle, const char* key, uint16_t* p_entry);

NVSJSON_ESETRET NVSJSON_Init(NVSJSON_SHandle* pHandle, const NVSJSON_SConfig* config)
{
    pHandle->isInitialized = false;

    if (NULL == config ||
        NULL == config->settingEntries ||
        0 == config->settingEntryCount ||
        NULL == config->partitionName)
    {
        return NVSJSON_ESETRET_InvalidLibrarySettings;
    }

    pHandle->config = config;
    return NVSJSON_ESETRET_OK;
}

NVSJSON_ESETRET NVSJSON_Load(NVSJSON_SHandle* pHandle)
{
    if (pHandle->isInitialized)
    {
        nvs_close(pHandle->nvs);
        pHandle->isInitialized = false;
    }
    ESP_ERROR_CHECK(nvs_open(pHandle->config->partitionName, NVS_READWRITE, &pHandle->nvs));
    pHandle->isInitialized = true;
    return NVSJSON_ESETRET_OK;
}

NVSJSON_ESETRET NVSJSON_Save(NVSJSON_SHandle* pHandle)
{
    ESP_ERROR_CHECK(nvs_commit(pHandle->nvs));
    return NVSJSON_ESETRET_OK;
}

int32_t NVSJSON_GetValueInt32(NVSJSON_SHandle* pHandle, uint16_t entry)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, entry);
    assert(NULL != pEnt && pEnt->type == NVSJSON_ETYPE_Int32);
    int32_t s32 = 0;
    if (ESP_OK == nvs_get_i32(pHandle->nvs, pEnt->key, &s32))
        return s32;
    return pEnt->uConfig.sInt32.defaultValue;
}

NVSJSON_ESETRET NVSJSON_SetValueInt32(NVSJSON_SHandle* pHandle, uint16_t entry, bool dry_run, int32_t new_value)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, entry);
    assert(NULL != pEnt && pEnt->type == NVSJSON_ETYPE_Int32);

    if (NULL != pEnt->uConfig.sInt32.ptrValidator)
    {
        if (!pEnt->uConfig.sInt32.ptrValidator(pEnt, new_value))
            return NVSJSON_ESETRET_ValidatorFailed;
    }
    else
    {
        if (new_value < pEnt->uConfig.sInt32.min || new_value > pEnt->uConfig.sInt32.max)
            return NVSJSON_ESETRET_InvalidRange;
    }

    if (!dry_run)
    {
        const esp_err_t err = nvs_set_i32(pHandle->nvs, pEnt->key, new_value);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (ESP_OK != err)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

double NVSJSON_GetValueDouble(NVSJSON_SHandle* pHandle, uint16_t entry)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, entry);
    assert(NULL != pEnt && pEnt->type == NVSJSON_ETYPE_Double);
    double dbl = 0;
    size_t len = sizeof(double);
    if (ESP_OK == nvs_get_blob(pHandle->nvs, pEnt->key, (void*)&dbl, &len))
        return dbl;
    return pEnt->uConfig.sDouble.defaultValue;
}

NVSJSON_ESETRET NVSJSON_SetValueDouble(NVSJSON_SHandle* pHandle, uint16_t entry, bool dry_run, double new_value)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, entry);
    assert(NULL != pEnt && pEnt->type == NVSJSON_ETYPE_Double);

    if (NULL != pEnt->uConfig.sDouble.ptrValidator)
    {
        if (!pEnt->uConfig.sDouble.ptrValidator(pEnt, new_value))
            return NVSJSON_ESETRET_ValidatorFailed;
    }
    else
    {
        if (new_value < pEnt->uConfig.sDouble.min || new_value > pEnt->uConfig.sDouble.max)
            return NVSJSON_ESETRET_InvalidRange;
    }

    if (!dry_run)
    {
        const esp_err_t err = nvs_set_blob(pHandle->nvs, pEnt->key, &new_value, sizeof(double));
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (ESP_OK != err)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

void NVSJSON_GetValueString(NVSJSON_SHandle* pHandle, uint16_t entry, char* out_value, size_t* length)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, entry);
    assert(NULL != pEnt && pEnt->type == NVSJSON_ETYPE_String);

    if (ESP_OK != nvs_get_str(pHandle->nvs, pEnt->key, out_value, length))
    {
        if (NULL != pEnt->uConfig.sString.defaultValue)
        {
            *length = strlen(pEnt->uConfig.sString.defaultValue);
            strncpy(out_value, pEnt->uConfig.sString.defaultValue, *length);
        }
    }
}

NVSJSON_ESETRET NVSJSON_SetValueString(NVSJSON_SHandle* pHandle, uint16_t entry, bool dry_run, const char* value)
{
    const NVSJSON_SSettingEntry* pEnt = GetSettingEntry(pHandle, entry);
    assert(NULL != pEnt && pEnt->type == NVSJSON_ETYPE_String);

    if (NULL != pEnt->uConfig.sString.ptrValidator)
    {
        if (!pEnt->uConfig.sString.ptrValidator(pEnt, value))
            return NVSJSON_ESETRET_ValidatorFailed;
    }

    if (!dry_run)
    {
        const esp_err_t err = nvs_set_str(pHandle->nvs, pEnt->key, value);
        ESP_ERROR_CHECK_WITHOUT_ABORT(err);
        if (ESP_OK != err)
            return NVSJSON_ESETRET_CannotSet;
    }
    return NVSJSON_ESETRET_OK;
}

char* NVSJSON_ExportJSON(NVSJSON_SHandle* pHandle)
{
    cJSON* pRoot = cJSON_CreateObject();
    if (NULL == pRoot)
        goto ERROR;

    cJSON* pEntries = cJSON_AddArrayToObject(pRoot, JSON_ENTRIES_NAME);

    for(int i = 0; i < pHandle->config->settingEntryCount; i++)
    {
        uint16_t entry = (uint16_t)i;
        const NVSJSON_SSettingEntry* pEntry = GetSettingEntry(pHandle, entry);

        cJSON* pEntryJSON = cJSON_CreateObject();
        cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_KEY_NAME, cJSON_CreateString(pEntry->key));

        cJSON* pEntryInfoJSON = cJSON_CreateObject();

        // Description and flags apply everywhere
        cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DESC_NAME, cJSON_CreateString(pEntry->desc));
        cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_FLAG_REBOOT_NAME, cJSON_CreateNumber((pEntry->flags & NVSJSON_EFLAGS_NeedsReboot) ? 1 : 0));

        if (NVSJSON_ETYPE_Int32 == pEntry->type)
        {
            if ((pEntry->flags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
                cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(NVSJSON_GetValueInt32(pHandle, entry)));

            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(pEntry->uConfig.sInt32.defaultValue));

            if (NULL == pEntry->uConfig.sInt32.ptrValidator)
            {
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(pEntry->uConfig.sInt32.min));
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(pEntry->uConfig.sInt32.max));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("int32"));
        }
        else if (NVSJSON_ETYPE_Double == pEntry->type)
        {
            if ((pEntry->flags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
                cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateNumber(NVSJSON_GetValueDouble(pHandle, entry)));

            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateNumber(pEntry->uConfig.sDouble.defaultValue));
            if (NULL == pEntry->uConfig.sDouble.ptrValidator)
            {
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MIN_NAME, cJSON_CreateNumber(pEntry->uConfig.sDouble.min));
                cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_MAX_NAME, cJSON_CreateNumber(pEntry->uConfig.sDouble.max));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("double"));
        }
        else if (NVSJSON_ETYPE_String == pEntry->type)
        {
            char value[NVSJSON_GETVALUESTRING_MAXLEN+1] = {0,};
            size_t length = NVSJSON_GETVALUESTRING_MAXLEN;
            if ((pEntry->flags & NVSJSON_EFLAGS_Secret) != NVSJSON_EFLAGS_Secret)
            {
                NVSJSON_GetValueString(pHandle, entry, value, &length);
                cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_VALUE_NAME, cJSON_CreateString(value));
            }
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_DEFAULT_NAME, cJSON_CreateString(pEntry->uConfig.sString.defaultValue));
            cJSON_AddItemToObject(pEntryInfoJSON, JSON_ENTRY_INFO_TYPE_NAME, cJSON_CreateString("string"));
        }

        cJSON_AddItemToObject(pEntryJSON, JSON_ENTRY_INFO_NAME, pEntryInfoJSON);

        cJSON_AddItemToArray(pEntries, pEntryJSON);
    }
    char* pStr = cJSON_PrintUnformatted(pRoot);
    cJSON_Delete(pRoot);
    return pStr;
    ERROR:
    cJSON_Delete(pRoot);
    return NULL;
}

bool NVSJSON_ImportJSON(NVSJSON_SHandle* pHandle, const char* json)
{
    bool ret_ok = true;
    cJSON* pRoot = cJSON_Parse(json);

    cJSON* pEntriesArray = cJSON_GetObjectItem(pRoot, JSON_ENTRIES_NAME);
    if (!cJSON_IsArray(pEntriesArray))
    {
        ESP_LOGE(TAG, "Entries array is not valid");
        goto ERROR;
    }

    for(int pass = 0; pass < 2; pass++)
    {
        const bool is_dry_run = (0 == pass);

        for(int i = 0; i < cJSON_GetArraySize(pEntriesArray); i++)
        {
            cJSON* pEntryJSON = cJSON_GetArrayItem(pEntriesArray, i);

            cJSON* pKeyJSON = cJSON_GetObjectItem(pEntryJSON, JSON_ENTRY_KEY_NAME);
            if (NULL == pKeyJSON || !cJSON_IsString(pKeyJSON))
            {
                ESP_LOGE(TAG, "Cannot find JSON key element");
                goto ERROR;
            }

            cJSON* pValueJSON = cJSON_GetObjectItem(pEntryJSON, JSON_ENTRY_VALUE_NAME);
            if (NULL == pValueJSON)
            {
                // We just ignore changing the setting if the value property is not there.
                // it allows us to handle secret cases.
                ESP_LOGD(TAG, "JSON value is not there, skipping it");
                continue;
            }

            uint16_t entry;
            if (!GetSettingEntryByKey(pHandle, pKeyJSON->valuestring, &entry))
            {
                ESP_LOGE(TAG, "Key: '%s' is not valid", pKeyJSON->valuestring);
                goto ERROR;
            }

            const NVSJSON_SSettingEntry* pSettingEntry = GetSettingEntry(pHandle, entry);

            if (NVSJSON_ETYPE_Int32 == pSettingEntry->type)
            {
                if (!cJSON_IsNumber(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                const int32_t s32 = pValueJSON->valueint;
                NVSJSON_ESETRET set_ret;
                if ((set_ret = NVSJSON_SetValueInt32(pHandle, entry, is_dry_run, s32)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, is_dry_run: %d, ret: %d", pSettingEntry->key, is_dry_run, set_ret);
                    goto ERROR;
                }
            }
            else if (NVSJSON_ETYPE_Double == pSettingEntry->type)
            {
                if (!cJSON_IsNumber(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a number");
                    goto ERROR;
                }
                const double dbl = (float)pValueJSON->valuedouble;
                NVSJSON_ESETRET set_ret;
                if ((set_ret = NVSJSON_SetValueDouble(pHandle, entry, is_dry_run, dbl)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, is_dry_run: %d, ret: %d", pSettingEntry->key, is_dry_run, set_ret);
                    goto ERROR;
                }
            }
            else if (NVSJSON_ETYPE_String == pSettingEntry->type)
            {
                if (!cJSON_IsString(pValueJSON))
                {
                    ESP_LOGE(TAG, "JSON value type is invalid, not a string");
                    goto ERROR;
                }

                const char* str = pValueJSON->valuestring;
                NVSJSON_ESETRET set_ret;
                if ((set_ret = NVSJSON_SetValueString(pHandle, entry, is_dry_run, str)) != NVSJSON_ESETRET_OK)
                {
                    ESP_LOGE(TAG, "Unable to set value for key: %s, is_dry_run: %d, ret: %d", pSettingEntry->key, is_dry_run, set_ret);
                    goto ERROR;
                }
            }
        }
    }

    ret_ok = true;
    ESP_LOGI(TAG, "Import JSON completed");
    goto END;
    ERROR:
    ret_ok = false;
    END:
    cJSON_free(pRoot);
    return ret_ok;
}

static const NVSJSON_SSettingEntry* GetSettingEntry(NVSJSON_SHandle* pHandle, uint16_t entry)
{
    if ((int)entry >= pHandle->config->settingEntryCount)
        return NULL;
    return &pHandle->config->settingEntries[(int)entry];
}

static bool GetSettingEntryByKey(NVSJSON_SHandle* pHandle, const char* key, uint16_t* p_entry)
{
    for(int i = 0; i < pHandle->config->settingEntryCount; i++)
    {
        if (0 == strcmp(pHandle->config->settingEntries[i].key, key))
        {
            *p_entry = (uint16_t)i;
            return true;
        }
    }
    return false;
}
