/**************************************************************************
 *
 * Copyright 2014 Valve Software
 * Copyright 2015 Google Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * Author: Jon Ashburn <jon@lunarg.com>
 * Author: Courtney Goeltzenleuchter <courtney@LunarG.com>
 * Author: Tobin Ehlis <tobin@lunarg.com>
 **************************************************************************/
#include <fstream>
#include <string>
#include <map>
#include <string.h>
#include <vulkan/vk_layer.h>
#include <iostream>
#include "vk_layer_config.h"
#include "vulkan/vk_sdk_platform.h"

#define MAX_CHARS_PER_LINE 4096

class ConfigFile
{
public:
    ConfigFile();
    ~ConfigFile();

    const char *getOption(const std::string &_option);
    void setOption(const std::string &_option, const std::string &_val);

private:
    bool m_fileIsParsed;
    std::map<std::string, std::string> m_valueMap;

    void parseFile(const char *filename);
};

static ConfigFile g_configFileObj;

static VkLayerDbgAction stringToDbgAction(const char *_enum)
{
    // only handles single enum values
    if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_IGNORE"))
        return VK_DBG_LAYER_ACTION_IGNORE;
    else if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_LOG_MSG"))
        return VK_DBG_LAYER_ACTION_LOG_MSG;
#ifdef WIN32
    else if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_DEBUG_OUTPUT"))
        return VK_DBG_LAYER_ACTION_DEBUG_OUTPUT;
#endif
    else if (!strcmp(_enum, "VK_DBG_LAYER_ACTION_BREAK"))
        return VK_DBG_LAYER_ACTION_BREAK;
    return (VkLayerDbgAction) 0;
}

static VkFlags stringToDbgReportFlags(const char *_enum)
{
    // only handles single enum values
    if (!strcmp(_enum, "VK_DEBUG_REPORT_INFO"))
        return VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
    else if (!strcmp(_enum, "VK_DEBUG_REPORT_WARN"))
        return VK_DEBUG_REPORT_WARNING_BIT_EXT;
    else if (!strcmp(_enum, "VK_DEBUG_REPORT_PERF_WARN"))
        return VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
    else if (!strcmp(_enum, "VK_DEBUG_REPORT_ERROR"))
        return VK_DEBUG_REPORT_ERROR_BIT_EXT;
    else if (!strcmp(_enum, "VK_DEBUG_REPORT_DEBUG"))
        return VK_DEBUG_REPORT_DEBUG_BIT_EXT;
    return (VkFlags) 0;
}

static unsigned int convertStringEnumVal(const char *_enum)
{
    unsigned int ret;

    ret = stringToDbgAction(_enum);
    if (ret)
        return ret;

    return stringToDbgReportFlags(_enum);
}

const char *getLayerOption(const char *_option)
{
    return g_configFileObj.getOption(_option);
}

// If option is NULL or stdout, return stdout, otherwise try to open option
//  as a filename. If successful, return file handle, otherwise stdout
FILE* getLayerLogOutput(const char *_option, const char *layerName)
{
    FILE* log_output = NULL;
    if (!_option || !strcmp("stdout", _option))
        log_output = stdout;
    else {
        log_output = fopen(_option, "w");
        if (log_output == NULL) {
            if (_option)
                std::cout << std::endl << layerName << " ERROR: Bad output filename specified: " << _option << ". Writing to STDOUT instead" << std::endl << std::endl;
            log_output = stdout;
        }
    }
    return log_output;
}

VkDebugReportFlagsEXT getLayerOptionFlags(const char *_option, uint32_t optionDefault)
{
    VkDebugReportFlagsEXT flags = optionDefault;
    const char *option = (g_configFileObj.getOption(_option));

    /* parse comma-separated options */
    while (option) {
        const char *p = strchr(option, ',');
        size_t len;

        if (p)
            len = p - option;
        else
            len = strlen(option);

        if (len > 0) {
            if (strncmp(option, "warn", len) == 0) {
                flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
            } else if (strncmp(option, "info", len) == 0) {
                flags |= VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
            } else if (strncmp(option, "perf", len) == 0) {
                flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
            } else if (strncmp(option, "error", len) == 0) {
                flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
            } else if (strncmp(option, "debug", len) == 0) {
                flags |= VK_DEBUG_REPORT_DEBUG_BIT_EXT;
            }
        }

        if (!p)
            break;

        option = p + 1;
    }
    return flags;
}

bool getLayerOptionEnum(const char *_option, uint32_t *optionDefault)
{
    bool res;
    const char *option = (g_configFileObj.getOption(_option));
    if (option != NULL) {
        *optionDefault = convertStringEnumVal(option);
        res = false;
    } else {
        res = true;
    }
    return res;
}

void setLayerOptionEnum(const char *_option, const char *_valEnum)
{
    unsigned int val = convertStringEnumVal(_valEnum);
    char strVal[24];
    snprintf(strVal, 24, "%u", val);
    g_configFileObj.setOption(_option, strVal);
}

void setLayerOption(const char *_option, const char *_val)
{
    g_configFileObj.setOption(_option, _val);
}

ConfigFile::ConfigFile() : m_fileIsParsed(false)
{
}

ConfigFile::~ConfigFile()
{
}

const char *ConfigFile::getOption(const std::string &_option)
{
    std::map<std::string, std::string>::const_iterator it;
    if (!m_fileIsParsed)
    {
        parseFile("vk_layer_settings.txt");
    }

    if ((it = m_valueMap.find(_option)) == m_valueMap.end())
        return NULL;
    else
        return it->second.c_str();
}

void ConfigFile::setOption(const std::string &_option, const std::string &_val)
{
    if (!m_fileIsParsed)
    {
        parseFile("vk_layer_settings.txt");
    }

    m_valueMap[_option] = _val;
}

void ConfigFile::parseFile(const char *filename)
{
    std::ifstream file;
    char buf[MAX_CHARS_PER_LINE];

    m_fileIsParsed = true;
    m_valueMap.clear();

    file.open(filename);
    if (!file.good())
        return;

    // read tokens from the file and form option, value pairs
    file.getline(buf, MAX_CHARS_PER_LINE);
    while (!file.eof())
    {
        char option[512];
        char value[512];

        char *pComment;

        //discard any comments delimited by '#' in the line
        pComment = strchr(buf, '#');
        if (pComment)
            *pComment = '\0';

        if (sscanf(buf, " %511[^\n\t =] = %511[^\n \t]", option, value) == 2)
        {
            std::string optStr(option);
            std::string valStr(value);
            m_valueMap[optStr] = valStr;
        }
        file.getline(buf, MAX_CHARS_PER_LINE);
    }
}

void print_msg_flags(VkFlags msgFlags, char *msg_flags)
{
    bool separator = false;

    msg_flags[0] = 0;
    if (msgFlags & VK_DEBUG_REPORT_DEBUG_BIT_EXT) {
        strcat(msg_flags, "DEBUG");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_INFORMATION_BIT_EXT) {
        if (separator) strcat(msg_flags, ",");
        strcat(msg_flags, "INFO");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_WARNING_BIT_EXT) {
        if (separator) strcat(msg_flags, ",");
        strcat(msg_flags, "WARN");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT) {
        if (separator) strcat(msg_flags, ",");
        strcat(msg_flags, "PERF");
        separator = true;
    }
    if (msgFlags & VK_DEBUG_REPORT_ERROR_BIT_EXT) {
        if (separator) strcat(msg_flags, ",");
        strcat(msg_flags, "ERROR");
    }
}

