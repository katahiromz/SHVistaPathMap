#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <shlobj.h>
#include <shlwapi.h>
#include <map>
#include <string>
#include <cassert>

std::string WideToAnsi(const std::wstring& str)
{
    CHAR buf[512];
    WideCharToMultiByte(CP_ACP, 0, str.c_str(), -1, buf, 512, NULL, NULL);
    return buf;
}

int load_string(HINSTANCE hMod, UINT id, LPWSTR pszBuf, INT cchBufMax)
{
    HGLOBAL hGlobal;
    HRSRC hRes;
    LPWSTR psz;
    INT index;

    if (HIWORD(id) == 0xffff)
        id = (UINT)(-((INT)id));

    hRes = FindResourceW(hMod, MAKEINTRESOURCEW(LOWORD(id >> 4) + 1), (LPWSTR)RT_STRING);
    if (!hRes)
        return 0;
    hGlobal = LoadResource(hMod, hRes);
    if (!hGlobal)
        return 0;
    psz = (WCHAR *)LockResource(hGlobal);

    index = id & 0xf;
    while (index--)
        psz += *psz + 1;

    if (!pszBuf)
        return *psz;

    if (*psz < cchBufMax)
        cchBufMax = *psz;
    else
        cchBufMax = (cchBufMax - 1);

    if (cchBufMax >= 0)
    {
        memcpy(pszBuf, &psz[1], cchBufMax * sizeof(WCHAR));
        pszBuf[cchBufMax] = 0;
    }

    return cchBufMax;
}

std::wstring get_folder_path(KNOWNFOLDER_DEFINITION& def)
{
    assert(def.pszRelativePath);

    LPWSTR pszPath = NULL;
    SHGetKnownFolderPath(def.fidParent, 0, NULL, &pszPath);

    WCHAR szPath[MAX_PATH];
    wcscpy(szPath, pszPath);
    CoTaskMemFree(pszPath);
    PathAppendW(szPath, def.pszRelativePath);

    return szPath;
}

int main(void)
{
    HRESULT hr;
    hr = CoInitialize(NULL);
    assert(SUCCEEDED(hr));

    IKnownFolderManager *pManager = NULL;
    hr = CoCreateInstance(CLSID_KnownFolderManager, 0, CLSCTX_ALL, IID_PPV_ARGS(&pManager));
    assert(SUCCEEDED(hr));

    UINT count;
    KNOWNFOLDERID *ids = NULL;
    hr = pManager->GetFolderIds(&ids, &count);
    assert(SUCCEEDED(hr));

    for (UINT index = 0; index < count; ++index)
    {
        IKnownFolder *pFolder = NULL;
        hr = pManager->GetFolder(ids[index], &pFolder);
        assert(SUCCEEDED(hr));

        KNOWNFOLDER_DEFINITION def;
        hr = pFolder->GetFolderDefinition(&def);
        assert(SUCCEEDED(hr));

        std::wstring strName1, strName2, strPath;
        if (def.pszLocalizedName)
        {
            WCHAR szExpanded[MAX_PATH];
            ExpandEnvironmentStringsW(def.pszLocalizedName, szExpanded, ARRAYSIZE(szExpanded));
            WCHAR *pch = wcsrchr(szExpanded, L',');
            *pch++ = 0;

            WCHAR szMod[MAX_PATH], szLocalized[MAX_PATH];
            INT nResID = _wtoi(pch);
            if (szExpanded[0] == L'@')
                wcscpy(szMod, &szExpanded[1]);
            else
                wcscpy(szMod, szExpanded);

            assert(def.pszName);
            strName1 = def.pszName;

            if (HMODULE hMod = LoadLibraryExW(szMod, NULL, LOAD_LIBRARY_AS_DATAFILE))
            {
                load_string(hMod, nResID, szLocalized, ARRAYSIZE(szLocalized));
                strName2 = szLocalized;
                FreeLibrary(hMod);
            }

            if (def.pszParsingName)
            {
                strPath = def.pszParsingName;
            }
            else if (def.pszRelativePath)
            {
                strPath = get_folder_path(def);
            }
        }

        if (strName1.empty() && strName2.empty())
            continue;

        std::string name1 = WideToAnsi(strName1);
        std::string name2 = WideToAnsi(strName2);
        std::string path = WideToAnsi(strPath);

        printf("%u: '%s', '%s', '%s'\n", index, name1.c_str(), name2.c_str(), path.c_str());

        FreeKnownFolderDefinitionFields(&def);

        pFolder->Release();
    }

    CoTaskMemFree(ids);
    pManager->Release();
}
