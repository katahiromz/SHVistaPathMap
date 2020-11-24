#ifndef SHVISTAPATHMAP_HPP_
#define SHVISTAPATHMAP_HPP_     2   // Version 2

#ifndef _INC_WINDOWS
    #include <windows.h>
#endif
#include <shlobj.h>
#include <map>
#include <string>
#include <cassert>

class SHVistaPathMap
{
public:
    typedef std::map<std::wstring, std::wstring> to_localized_t;
    typedef std::map<std::wstring, std::wstring> from_localized_t;
    to_localized_t m_to_localized;
    from_localized_t m_from_localized;

    SHVistaPathMap()
    {
    }

    explicit SHVistaPathMap(HWND hwnd)
    {
        init(hwnd);
    }

    void init(HWND hwnd)
    {
        m_to_localized.clear();
        m_from_localized.clear();

        WCHAR szPath[MAX_PATH], szLocalized[MAX_PATH];
        for (INT csidl = 0; csidl < 0x40; ++csidl)
        {
            if (!SHGetSpecialFolderPathW(hwnd, szPath, csidl, FALSE))
                continue;

            WCHAR szMod[MAX_PATH], szExpanded[MAX_PATH];
            INT nResID;
            HRESULT hr = SHGetLocalizedName(szPath, szMod, ARRAYSIZE(szMod), &nResID);
            if (FAILED(hr))
                continue;

            ExpandEnvironmentStringsW(szMod, szExpanded, ARRAYSIZE(szExpanded));

            if (HMODULE hMod = LoadLibraryExW(szExpanded, NULL, LOAD_LIBRARY_AS_DATAFILE))
            {
                if (LoadStringW(hMod, nResID, szLocalized, ARRAYSIZE(szLocalized)))
                {
                    m_to_localized[szPath] = szLocalized;
                    m_from_localized[szLocalized] = szPath;
                }
                FreeLibrary(hMod);
            }
        }
    }

    std::wstring get_localized(const std::wstring& path) const
    {
        assert(m_to_localized.size());  // must be initialized by init
        auto it = m_to_localized.find(path);
        if (it != m_to_localized.end())
            return it->second;
        return path;
    }

    std::wstring get_unlocalized(const std::wstring& localized) const
    {
        assert(m_from_localized.size());  // must be initialized by init
        auto it = m_from_localized.find(localized);
        if (it != m_from_localized.end())
            return it->second;
        return localized;
    }

    std::wstring operator()(const std::wstring& localized) const
    {
        return get_localized(localized);
    }
};

inline void SHVistaPathMap_unittest(void)
{
    HWND hwnd = NULL;
    SHVistaPathMap map(hwnd);

    WCHAR szPath[MAX_PATH];
    INT csidl = CSIDL_PERSONAL;
    SHGetSpecialFolderPathW(hwnd, szPath, csidl, FALSE);

    std::wstring localized = map.get_localized(szPath);
    MessageBoxW(hwnd, localized.c_str(), L"Test #0", MB_ICONINFORMATION);

    std::wstring unlocalized = map.get_unlocalized(localized);
    MessageBoxW(hwnd, unlocalized.c_str(), L"Test #1", MB_ICONINFORMATION);
}

#endif   // ndef SHVISTAPATHMAP_HPP_
