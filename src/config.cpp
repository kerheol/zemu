#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "platform.h"
#include "dirwork.h"
#include "file.h"
#include "config.h"

#ifdef _WIN32
	#include <io.h> // for _access()
#else
	#include <unistd.h> // for access()
#endif // _WIN32

// [rst] what that FOR_INSTALL mean at all?

#ifdef _WIN32
	#ifndef _WIN32_IE
		#define _WIN32_IE 0x400
	#endif

	#include <shlobj.h>
	#include <io.h>
	#include <direct.h> // for _mkdir()
#else
	#include <sys/stat.h>
	#include <sys/types.h>
#endif // _WIN32

#ifdef _WIN32
	#include <shlwapi.h>
#endif

char *CConfig::executableDir = NULL;

CConfig::CConfig() {
	changed = false;
}

CConfig::~CConfig() {
	if (changed && !user_path.empty() && !ini_name.empty()) {
		ini.SaveFile(C_DirWork::Append(user_path, ini_name).c_str());
	}
}

void CConfig::Initialize(const char *app_name) {
	changed = false;
	ini_name = string(app_name) + ".ini";

	EnsurePaths(app_name);

	// 1. try in current folder (allow use custom-configs for folder)
	user_path = ".";
	if (!ini.LoadFile(C_DirWork::Append(user_path, ini_name).c_str())) return;

	if (executableDir) {
		// 2. try in executable dir (useful for debug version && file associations)
		user_path = executableDir;
		if (!ini.LoadFile(C_DirWork::Append(user_path, ini_name).c_str())) return;
	}

	// 3. try in home folder
	user_path = home_path;
	if (!ini.LoadFile(C_DirWork::Append(user_path, ini_name).c_str())) return;

	#ifdef SHARE_PATH
		// 4. try in shared folder
		if (!ini.LoadFile(C_DirWork::Append(SHARE_PATH, ini_name))) return;
	#endif
}

void CConfig::EnsurePaths(const char *app_name) {
	if (!home_path.empty()) {
		return;
	}

	#ifdef _WIN32
		TCHAR buffer[MAX_PATH];

		if (SHGetSpecialFolderPath(0, buffer, CSIDL_APPDATA, true)) {
			home_path = string(buffer);
		}
	#else
		if (getenv("XDG_CONFIG_HOME")) {
			home_path = getenv("XDG_CONFIG_HOME");
		} else if (getenv("HOME")) {
			if (C_File::FileExists(C_DirWork::Append(getenv("HOME"), ".config").c_str())) {
				home_path = C_DirWork::Append(getenv("HOME"), ".config");
			} else {
				home_path = string(getenv("HOME"));
			}
		}
	#endif

	if (home_path.empty()) {
		home_path = ".";
	} else {
		home_path = C_DirWork::Append(home_path, app_name);

		#ifdef _WIN32
			_mkdir(home_path.c_str());
		#else
			mkdir(home_path.c_str(), 0755);
		#endif
	}
}

int CConfig::GetInt(const char *section, const char *key, int default_value) {
	const char *cval = ini.GetValue(section, key, NULL);
	if (cval) {
		return atoi(cval);
	} else {
		SetInt(section, key, default_value);
		return default_value;
	}
}

void CConfig::SetInt(const char *section, const char *key, int value) {
	char buffer[0x100];
	sprintf(buffer, "%d", value);
	ini.SetValue(section, key, buffer);
	changed = true;
}

string CConfig::GetString(const char *section, const char *key, const string &default_value) {
	const char *cval = ini.GetValue(section, key, NULL);
	if (cval) {
		return string(cval);
	} else {
		SetString(section, key, default_value);
		return default_value;
	}
}

void CConfig::SetString(const char *section, const char *key, const string &value) {
	ini.SetValue(section, key, value.c_str());
	changed = true;
}

bool CConfig::GetBool(const char *section, const char *key, bool default_value) {
	const char *cval = ini.GetValue(section, key, NULL);
	if (cval) {
		if(!strcasecmp(cval, "true") || !strcasecmp(cval, "yes")) {
			return true;
		} else {
			return atoi(cval) ? true : false;
		}
	} else {
		SetBool(section, key, default_value);
		return default_value;
	}
}

void CConfig::SetBool(const char *section, const char *key, bool value) {
	ini.SetValue(section, key, value ? "yes" : "no");
	changed = true;
}

size_t CConfig::LoadDataFile(const char *prefix, const char *filename, uint8_t *buffer, size_t size, size_t offset) {
	string path = CConfig::FindDataFile(prefix, filename);

	if (path.empty()) {
		return 0;
	}

	C_File fl;
	fl.Read(path.c_str());

	if (offset) {
		fl.SetFilePointer(offset);
	}

	size_t readed = fl.ReadBlock(buffer, size);
	fl.Close();

	return readed;
}

/*
 * Not used
 *
bool CConfig::SaveDataFile(const char *prefix, const char *filename, const uint8_t *buffer, size_t size) {
	string folder = C_DirWork::Append(user_path, prefix);

	#ifdef _WIN32
		mkdir(folder.c_str());
	#else
		mkdir(folder.c_str(), 0755);
	#endif

	C_File fl;
	fl.Write(C_DirWork::Append(folder, filename).c_str());
	fl.WriteBlock(buffer, size);
	fl.Close();

	return true;
}
*/

string CConfig::FindDataFile(const char *prefix, const char *filename) {
	string path;
	string append_path = C_DirWork::Append(prefix, filename);

	// 1. try in current folder (allow use custom-configs for folder)
	if (C_File::FileExists(append_path.c_str())) return append_path;

	if (executableDir) {
		// 2. try in executable dir (useful for debug version && file associations)
		path = C_DirWork::Append(executableDir, append_path);
		if (C_File::FileExists(path.c_str())) return path;
	}

	// 3. try in home folder
	path = C_DirWork::Append(home_path, append_path);
	if (C_File::FileExists(path.c_str())) return path;

	#ifdef SHARE_PATH
		// 4. try in shared folder
		path = C_DirWork::Append(SHARE_PATH, append_path);
		if (C_File::FileExists(path.c_str())) return path;
	#endif

	return string("");
}
