/*
 *  OpenVPN-GUI -- A Windows GUI for OpenVPN.
 *
 *  Copyright (C) 2004 Mathias Sundman <mathias@nilings.se>
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
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <windows.h>
#include <wincrypt.h>

#include <tchar.h>
#include <shlobj.h>

//#define DEBUG


#include "main.h"
#include "options.h"
#include "openvpn-gui-res.h"
#include "registry.h"
#include "localization.h"
#include "viewlog.h"

extern options_t o;

int
GetRegistryKeys()
{
  TCHAR windows_dir[MAX_PATH];
  TCHAR temp_path[MAX_PATH];
  TCHAR openvpn_path[MAX_PATH];
  HKEY regkey;

  if (!GetWindowsDirectory(windows_dir, _countof(windows_dir))) {
    /* can't get windows dir */
    ShowLocalizedMsg(IDS_ERR_GET_WINDOWS_DIR);
    return(false);
  }

  /* Get path to OpenVPN installation. */
  if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, _T("SOFTWARE\\OpenVPN"), 0, KEY_READ, &regkey)
      != ERROR_SUCCESS) 
    {
      /* registry key not found */
      ShowLocalizedMsg(IDS_ERR_OPEN_REGISTRY);
      return(false);
    }
  if (!GetRegistryValue(regkey, _T(""), openvpn_path, _countof(openvpn_path)))
    {
      /* error reading registry value */
      ShowLocalizedMsg(IDS_ERR_READING_REGISTRY);
      return(false);
    }
  if (openvpn_path[_tcslen(openvpn_path) - 1] != _T('\\'))
    _tcscat(openvpn_path, _T("\\"));


  _sntprintf_0(temp_path, _T("%sconfig"), openvpn_path);
  if (!GetRegKey(_T("config_dir"), o.config_dir, 
      temp_path, _countof(o.config_dir))) return(false);

  if (!GetRegKey(_T("config_ext"), o.ext_string, _T("ovpn"), _countof(o.ext_string))) return(false);

  _sntprintf_0(temp_path, _T("%sbin\\openvpn.exe"), openvpn_path);
  if (!GetRegKey(_T("exe_path"), o.exe_path, 
      temp_path, _countof(o.exe_path))) return(false);

  _sntprintf_0(temp_path, _T("%slog"), openvpn_path);
  if (!GetRegKey(_T("log_dir"), o.log_dir, 
      temp_path, _countof(o.log_dir))) return(false);

  if (!GetRegKey(_T("log_append"), o.append_string, _T("0"), _countof(o.append_string))) return(false);

  if (!GetRegKey(_T("priority"), o.priority_string, 
      _T("NORMAL_PRIORITY_CLASS"), _countof(o.priority_string))) return(false);

  _sntprintf_0(temp_path, _T("%s\\notepad.exe"), windows_dir);
  if (!GetRegKey(_T("log_viewer"), o.log_viewer, 
      temp_path, _countof(o.log_viewer))) return(false);

  _sntprintf_0(temp_path, _T("%s\\notepad.exe"), windows_dir);
  if (!GetRegKey(_T("editor"), o.editor, 
      temp_path, _countof(o.editor))) return(false);

  if (!GetRegKey(_T("allow_edit"), o.allow_edit, _T("1"), _countof(o.allow_edit))) return(false);
  
  if (!GetRegKey(_T("allow_service"), o.allow_service, _T("0"), _countof(o.allow_service))) return(false);

  if (!GetRegKey(_T("allow_password"), o.allow_password, _T("1"), _countof(o.allow_password))) return(false);

  if (!GetRegKey(_T("allow_proxy"), o.allow_proxy, _T("1"), _countof(o.allow_proxy))) return(false);

  if (!GetRegKey(_T("service_only"), o.service_only, _T("0"), _countof(o.service_only))) return(false);

  if (!GetRegKey(_T("show_balloon"), o.show_balloon, _T("1"), _countof(o.show_balloon))) return(false);

  if (!GetRegKey(_T("silent_connection"), o.silent_connection, _T("0"), _countof(o.silent_connection))) return(false);

  if (!GetRegKey(_T("show_script_window"), o.show_script_window, _T("1"), _countof(o.show_script_window))) return(false);

  if (!GetRegKey(_T("disconnect_on_suspend"), o.disconnect_on_suspend, _T("1"), 
      _countof(o.disconnect_on_suspend))) return(false);

  if (!GetRegKey(_T("passphrase_attempts"), o.psw_attempts_string, _T("3"), 
      _countof(o.psw_attempts_string))) return(false);
  o.psw_attempts = _ttoi(o.psw_attempts_string);
  if ((o.psw_attempts < 1) || (o.psw_attempts > 9))
    {
      /* 0 <= passphrase_attempts <= 9 */
      ShowLocalizedMsg(IDS_ERR_PASSPHRASE_ATTEMPTS);
      return(false);
    }

  if (!GetRegKey(_T("connectscript_timeout"), o.connectscript_timeout_string, _T("15"), 
      _countof(o.connectscript_timeout_string))) return(false);
  o.connectscript_timeout = _ttoi(o.connectscript_timeout_string);
  if ((o.connectscript_timeout < 0) || (o.connectscript_timeout > 99))
    {
      /* 0 <= connectscript_timeout <= 99 */
      ShowLocalizedMsg(IDS_ERR_CONN_SCRIPT_TIMEOUT);
      return(false);
    }

  if (!GetRegKey(_T("disconnectscript_timeout"), o.disconnectscript_timeout_string, _T("10"), 
      _countof(o.disconnectscript_timeout_string))) return(false);
  o.disconnectscript_timeout = _ttoi(o.disconnectscript_timeout_string);
  if ((o.disconnectscript_timeout <= 0) || (o.disconnectscript_timeout > 99))
    {
      /* 0 < disconnectscript_timeout <= 99 */
      ShowLocalizedMsg(IDS_ERR_DISCONN_SCRIPT_TIMEOUT);
      return(false);
    }

  if (!GetRegKey(_T("preconnectscript_timeout"), o.preconnectscript_timeout_string, _T("10"), 
      _countof(o.preconnectscript_timeout_string))) return(false);
  o.preconnectscript_timeout = _ttoi(o.preconnectscript_timeout_string);
  if ((o.preconnectscript_timeout <= 0) || (o.preconnectscript_timeout > 99))
    {
      /* 0 < disconnectscript_timeout <= 99 */
      ShowLocalizedMsg(IDS_ERR_PRECONN_SCRIPT_TIMEOUT);
      return(false);
    }

  return(true);
}


int GetRegKey(const TCHAR name[], TCHAR *data, const TCHAR default_data[], DWORD len)
{
  LONG status;
  DWORD type;
  HKEY openvpn_key;
  HKEY openvpn_key_write;
  DWORD dwDispos;
  TCHAR expanded_string[MAX_PATH];
  DWORD size = len * sizeof(*data);
  DWORD max_len = len - 1;

  /* If option is already set via cmd-line, return */
  if (data[0] != 0) 
    {
      // Expand environment variables inside the string.
      ExpandEnvironmentStrings(data, expanded_string, _countof(expanded_string));
      _tcsncpy(data, expanded_string, max_len);
      return(true);
    }

  status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                       _T("SOFTWARE\\AxionVPN"),
                       0,
                       KEY_READ,
                       &openvpn_key);

  if (status != ERROR_SUCCESS)
    {
      if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        _T("Software\\AxionVPN"),
                        0,
                        _T(""),
                        REG_OPTION_NON_VOLATILE,
                        KEY_READ | KEY_WRITE,
                        NULL,
                        &openvpn_key,
                        &dwDispos) != ERROR_SUCCESS)
        {
          /* error creating registry key */
          ShowLocalizedMsg(IDS_ERR_CREATE_REG_KEY);
          return(false);
        }  
    }


  /* get a registry string */
  status = RegQueryValueEx(openvpn_key, name, NULL, &type, (byte *) data, &size);
  if (status != ERROR_SUCCESS || type != REG_SZ)
    {
      /* key did not exist - set default value */
      status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
			  _T("SOFTWARE\\AxionVPN"),
			  0,
			  KEY_READ | KEY_WRITE,
			  &openvpn_key_write);

      if (status != ERROR_SUCCESS) {
         /* can't open registry for writing */
         ShowLocalizedMsg(IDS_ERR_OPEN_WRITE_REG);
         return(false);
      }    
      if(!SetRegistryValue(openvpn_key_write, name, default_data))
        {
          /* cant read / set reg-key */ 
          return(false);
        }
      _tcsncpy(data, default_data, max_len);
      RegCloseKey(openvpn_key_write);

    }

  RegCloseKey(openvpn_key);

  // Expand environment variables inside the string.
  ExpandEnvironmentStrings(data, expanded_string, _countof(expanded_string));
  _tcsncpy(data, expanded_string, max_len);

  return(true);
}

LONG GetRegistryValue(HKEY regkey, const TCHAR *name, TCHAR *data, DWORD len)
{
  LONG status;
  DWORD type;
  DWORD data_len;

  data_len = len * sizeof(*data);

  /* get a registry string */
  status = RegQueryValueEx(regkey, name, NULL, &type, (byte *) data, &data_len);
  if (status != ERROR_SUCCESS || type != REG_SZ)
    return(0);

  return(data_len / sizeof(*data));

}

LONG
GetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD *data)
{
  DWORD type;
  DWORD size = sizeof(*data);
  LONG status = RegQueryValueEx(regkey, name, NULL, &type, (PBYTE) data, &size);
  return (type == REG_DWORD ? status : ERROR_FILE_NOT_FOUND);
}

int SetRegistryValue(HKEY regkey, const TCHAR *name, const TCHAR *data)
{
	//PrintDebug(_T("[SaveCreds] Called with: %S and %S"), name, data);


  /* set a registry string */
  DWORD size = (_tcslen(data) + 1) * sizeof(*data);
  if(RegSetValueEx(regkey, name, 0, REG_SZ, (PBYTE) data, size) != ERROR_SUCCESS)
    {
      /* Error writing registry value */
      ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
      return(0);
    }

  return(1);

}

int
SetRegistryValueNumeric(HKEY regkey, const TCHAR *name, DWORD data)
{
  LONG status = RegSetValueEx(regkey, name, 0, REG_DWORD, (PBYTE) &data, sizeof(data));
  if (status == ERROR_SUCCESS)
    return 1;

  ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
  return 0;
}


//
// int SetRegistryValueBin(HKEY regkey, const WCHAR *name, const void *data,DWORD size)
//
// Given a buffer of size "size", use MS's data protection function and write it to the 
// registry
//
int SetRegistryValueBin(HKEY regkey, const WCHAR *name, const void *data, DWORD size)
{
	//PrintDebug(L"[SetRegistryValueBin] Called with %S of size %d",data,size);

	DATA_BLOB DataIn;
	DATA_BLOB DataOut;
	DATA_BLOB DataVerify;
    DWORD cbDataInput = size;


   DataIn.pbData = data;    
   DataIn.cbData = cbDataInput;
   LPWSTR pDescrOut = NULL;


if(CryptProtectData(
     &DataIn,
     NULL, // A description string. 
     NULL,                               // Optional entropy
                                         // not used.
     NULL,                               // Reserved.
     NULL,								 // No PromptStruct.
     0,
     &DataOut))
{
     printf("The encryption phase worked. \n");
}
else
{
    printf("Encryption error!");
}


  /* set a registry binary value */
  if(RegSetValueEx(regkey, name, 0, REG_BINARY, (PBYTE) DataOut.pbData, DataOut.cbData) != ERROR_SUCCESS)
    {
      /* Error writing registry value */
      ShowLocalizedMsg(IDS_ERR_WRITE_REGVALUE, GUI_REGKEY_HKCU, name);
      return(0);
    }

  return(1);

}



//
// LONG GetRegistryValueBin(HKEY regkey, const WCHAR *name, void *data, DWORD len)
//
// Retrieve an encrypted registry value
//


LONG GetRegistryValueBin(HKEY regkey, const WCHAR *name, void *data, DWORD len)
{
  LONG status;
  DWORD type;
  DWORD dwBytesNeeded = 0;
  DWORD dwRetVal = 0;

  DATA_BLOB DataIn;
  DATA_BLOB DataOut;

  void *tmpData = NULL;

	
  //PrintDebug(L"[GetRegistryValueBin] Called");


  //See how many bytes we need
  status = RegQueryValueEx(regkey, name, NULL, &type, NULL, &dwBytesNeeded);
  if (status != ERROR_SUCCESS || dwBytesNeeded == 0){
     //PrintDebug(L"[GetRegistryValueBin] Query failed or needed no bytes");
	 goto exit;
  }

  //Allocate on the heap the Bytes we need into a temporarary heap space
  //PrintDebug(L"[GetRegistryValueBin] Size needed is %d",dwBytesNeeded);
  tmpData = HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,dwBytesNeeded);
  if(tmpData == NULL){
	//PrintDebug(L"[GetRegistryValueBin] Failed to allocate memory");
	goto exit;
  }


  //Now really query for our Data.
  status = RegQueryValueEx(regkey, name, NULL, &type, (byte *) tmpData, &dwBytesNeeded);
    if (status != ERROR_SUCCESS || dwBytesNeeded == 0){
     //PrintDebug(L"[GetRegistryValueBin] Second query failed");
	 goto exit;
  }

  DataIn.pbData = tmpData; 
  DataIn.cbData = dwBytesNeeded;

 

  if (CryptUnprotectData(
        &DataIn,
        NULL,
        NULL,                 // Optional entropy
        NULL,                 // Reserved
        NULL,				  // No PromptStruct
        0,
        &DataOut))
{
     
	 memset(data,0,len);
     memcpy(data,DataOut.pbData,DataOut.cbData);
	 dwRetVal = DataOut.cbData;
	 //PrintDebug(L"[GetRegistryValueBin] Decrypted data len is: %d\n",DataOut.cbData);
	 //PrintDebug(L"[GetRegistryValueBin] The decrypted data is: %S\n", data);
 
}
else
{
    //PrintDebug("Decryption error!");
}




exit:
  if(tmpData){
	  HeapFree(GetProcessHeap(),0,tmpData);
  }

  return dwRetVal;

}
