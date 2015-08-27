/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
 *                2010 Heiko Hund <heikoh@users.sf.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program (see the file COPYING included with this
 *  distribution); if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>

#define DEBUG

#include "main.h"
#include "openvpn-gui-res.h"
#include "options.h"
#include "localization.h"

typedef enum
{
    match_false,
    match_file,
    match_dir
} match_t;

extern options_t o;

static match_t
match(const WIN32_FIND_DATA *find, const TCHAR *ext)
{
    size_t ext_len = _tcslen(ext);
    int i;

    if (find->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        return match_dir;

    if (ext_len == 0)
        return match_file;

    i = _tcslen(find->cFileName) - ext_len - 1;

    if (i > 0 && find->cFileName[i] == '.'
    && _tcsicmp(find->cFileName + i + 1, ext) == 0)
        return match_file;

    return match_false;
}


static int
ConfigAlreadyExists(TCHAR *newconfig)
{
    int i;
    for (i = 0; i < o.num_configs; ++i)
    {
        if (_tcsicmp(o.conn[i].config_file, newconfig) == 0)
            return true;
    }
    return false;
}


static void
AddConfigFileToList(int config, TCHAR *filename, TCHAR *config_dir)
{
    connection_t *c = &o.conn[config];
    int i;

    _tcsncpy(c->config_file, filename, _countof(c->config_file) - 1);
    _tcsncpy(c->config_dir, config_dir, _countof(c->config_dir) - 1);
    _tcsncpy(c->config_name, c->config_file, _countof(c->config_name) - 1);
    c->config_name[_tcslen(c->config_name) - _tcslen(o.ext_string) - 1] = _T('\0');
    _sntprintf_0(c->log_path, _T("%s\\%s.log"), o.log_dir, c->config_name);

    c->manage.sk = INVALID_SOCKET;
    c->manage.skaddr.sin_family = AF_INET;
    c->manage.skaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    c->manage.skaddr.sin_port = htons(25340 + config);

    /* Check if connection should be autostarted */
    for (i = 0; i < MAX_CONFIGS && o.auto_connect[i]; ++i)
    {
        if (_tcsicmp(c->config_file, o.auto_connect[i]) == 0)
        {
            c->auto_connect = true;
            break;
        }
    }
}


void
BuildFileList()
{
    WIN32_FIND_DATA find_obj;
    HANDLE find_handle;
    TCHAR find_string[MAX_PATH];
    TCHAR subdir_table[MAX_CONFIG_SUBDIRS][MAX_PATH];
    int subdirs = 0;
    int i;

    /* Reset config counter */
    o.num_configs = 0;

    _sntprintf_0(find_string, _T("%s\\*"), o.config_dir);
    find_handle = FindFirstFile(find_string, &find_obj);
    if (find_handle == INVALID_HANDLE_VALUE)
        return;

    /* Loop over each config file in main config dir */
    do
    {
        if (o.num_configs >= MAX_CONFIGS)
        {
			PrintDebug(L"[BuildFileList] There are %d configs\n",o.num_configs);
			//MessageBox(NULL, _T("Many Configs 01"), _T("Error!"), MB_ICONEXCLAMATION | MB_OK);
           // ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
            break;
        }

        match_t match_type = match(&find_obj, o.ext_string);
        if (match_type == match_file)
        {
            AddConfigFileToList(o.num_configs++, find_obj.cFileName, o.config_dir);
        }
        else if (match_type == match_dir)
        {
            if (_tcsncmp(find_obj.cFileName, _T("."), _tcslen(find_obj.cFileName)) != 0
            &&  _tcsncmp(find_obj.cFileName, _T(".."), _tcslen(find_obj.cFileName)) != 0
            &&  subdirs < MAX_CONFIG_SUBDIRS)
            {
                /* Add dir to dir_table */
                _sntprintf_0(subdir_table[subdirs], _T("%s\\%s"), o.config_dir, find_obj.cFileName);
                subdirs++;
            }
        }
    } while (FindNextFile(find_handle, &find_obj));



    FindClose(find_handle);

    /* Loop over each config file in every subdir */
    for (i = 0; i < subdirs; ++i)
    {
        _sntprintf_0(find_string, _T("%s\\*"), subdir_table[i]);

        find_handle = FindFirstFile (find_string, &find_obj);
        if (find_handle == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (o.num_configs >= MAX_CONFIGS)
            {
				MessageBox(NULL, _T("Many Configs 02"), _T("Error!"), MB_ICONEXCLAMATION | MB_OK);
                ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
                FindClose(find_handle);
                return;
            }

            /* does file have the correct type and extension? */
            if (match(&find_obj, o.ext_string) != match_file)
                continue;

            if (ConfigAlreadyExists(find_obj.cFileName))
            {
                ShowLocalizedMsg(IDS_ERR_CONFIG_EXIST, find_obj.cFileName);
                continue;
            }

            AddConfigFileToList(o.num_configs++, find_obj.cFileName, subdir_table[i]);
        } while (FindNextFile(find_handle, &find_obj));

        FindClose(find_handle);
    }
}



//
// WipeFileList()
//
// Clean out any config files that somehow managed to linger in our
// config directory
// 

void
WipeFileList()
{
    WIN32_FIND_DATA find_obj;
    HANDLE find_handle;
    TCHAR find_string[MAX_PATH];
    TCHAR subdir_table[MAX_CONFIG_SUBDIRS][MAX_PATH];
    int subdirs = 0;
    int i;
	WCHAR outpath[MAX_PATH];

    /* Reset config counter */
    o.num_configs = 0;

    _sntprintf_0(find_string, _T("%s\\*"), o.config_dir);
    find_handle = FindFirstFile(find_string, &find_obj);
    if (find_handle == INVALID_HANDLE_VALUE)
        return;

    /* Loop over each config file in main config dir */
    do
    {
        if (o.num_configs >= MAX_CONFIGS)
        {
			MessageBox(NULL, _T("Many Configs 03"), _T("Error!"), MB_ICONEXCLAMATION | MB_OK);
            ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
            break;
        }

        match_t match_type = match(&find_obj, o.ext_string);
        if (match_type == match_file)
        {
			memset(outpath,0,MAX_PATH * sizeof(WCHAR));
			_sntprintf_0(outpath, _T("%s\\%s"), o.config_dir, find_obj.cFileName);

			DeleteFile(outpath);
            //AddConfigFileToList(o.num_configs++, find_obj.cFileName, o.config_dir);
        }
        else if (match_type == match_dir)
        {
            if (_tcsncmp(find_obj.cFileName, _T("."), _tcslen(find_obj.cFileName)) != 0
            &&  _tcsncmp(find_obj.cFileName, _T(".."), _tcslen(find_obj.cFileName)) != 0
            &&  subdirs < MAX_CONFIG_SUBDIRS)
            {
                /* Add dir to dir_table */
                _sntprintf_0(subdir_table[subdirs], _T("%s\\%s"), o.config_dir, find_obj.cFileName);
                subdirs++;
            }
        }
    } while (FindNextFile(find_handle, &find_obj));

    FindClose(find_handle);

    /* Loop over each config file in every subdir */
    for (i = 0; i < subdirs; ++i)
    {
        _sntprintf_0(find_string, _T("%s\\*"), subdir_table[i]);

        find_handle = FindFirstFile (find_string, &find_obj);
        if (find_handle == INVALID_HANDLE_VALUE)
            continue;

        do
        {
            if (o.num_configs >= MAX_CONFIGS)
            {
				MessageBox(NULL, _T("Many Configs 04"), _T("Error!"), MB_ICONEXCLAMATION | MB_OK);
                ShowLocalizedMsg(IDS_ERR_MANY_CONFIGS, MAX_CONFIGS);
                FindClose(find_handle);
                return;
            }

            /* does file have the correct type and extension? */
            if (match(&find_obj, o.ext_string) != match_file)
                continue;

            if (ConfigAlreadyExists(find_obj.cFileName))
            {
                ShowLocalizedMsg(IDS_ERR_CONFIG_EXIST, find_obj.cFileName);
                continue;
            }

            AddConfigFileToList(o.num_configs++, find_obj.cFileName, subdir_table[i]);
        } while (FindNextFile(find_handle, &find_obj));

        FindClose(find_handle);
    }
}



/*
 * Returns TRUE if option exist in config file.
 */
bool
ConfigFileOptionExist(int config, const char *option)
{
    FILE *fp;
    char line[256];
    TCHAR path[MAX_PATH];

    _tcsncpy(path, o.conn[config].config_dir, _countof(path));
    if (path[_tcslen(path) - 1] != _T('\\'))
        _tcscat(path, _T("\\"));
    _tcsncat(path, o.conn[config].config_file, _countof(path) - _tcslen(path) - 1);

    fp = _tfopen(path, _T("r"));
    if (fp == NULL)
    {
        ShowLocalizedMsg(IDS_ERR_OPEN_CONFIG, path);
        return false;
    }

    while (fgets(line, sizeof(line), fp))
    {
        if (strncmp(line, option, strlen(option)) == 0)
        {
            fclose(fp);
            return true;
        }
    }

    fclose(fp);
    return false;
}

