#include "script-startup-internal.hpp"
//    script-startup.cpp - EAthena script frontend, engine, and library.
//
//    Copyright © ????-2004 Athena Dev Teams
//    Copyright © 2004-2011 The Mana World Development Team
//    Copyright © 2011 Chuck Miller
//    Copyright © 2011-2014 Ben Longbons <b.r.longbons@gmail.com>
//    Copyright © 2013 wushin
//
//    This file is part of The Mana World (Athena server)
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 3 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include "../strings/zstring.hpp"

#include "../generic/db.hpp"
#include "../generic/intern-pool.hpp"

#include "../io/cxxstdio.hpp"
#include "../io/extract.hpp"
#include "../io/read.hpp"
#include "../io/lock.hpp"

#include "../net/timer.hpp"

#include "globals.hpp"
#include "map.hpp"
#include "map_conf.hpp"
#include "script-parse-internal.hpp"
#include "script-persist.hpp"

#include "../poison.hpp"


namespace tmwa
{
namespace map
{
constexpr std::chrono::milliseconds MAPREG_AUTOSAVE_INTERVAL = 10_s;

bool read_constdb(ZString filename)
{
    io::ReadFile in(filename);
    if (!in.is_open())
    {
        PRINTF("can't read %s\n"_fmt, filename);
        return false;
    }

    bool rv = true;
    AString line_;
    while (in.getline(line_))
    {
        // is_comment only works for whole-line comments
        // that could change once the Z dependency is dropped ...
        LString comment = "//"_s;
        XString line = line_.xislice_h(std::search(line_.begin(), line_.end(), comment.begin(), comment.end())).rstrip();
        if (!line)
            continue;
        // "%m[A-Za-z0-9_] %i %i"

        // TODO promote either qsplit() or asplit()
        auto _it = std::find(line.begin(), line.end(), ' ');
        auto name = line.xislice_h(_it);
        auto _rest = line.xislice_t(_it);
        while (_rest.startswith(' '))
            _rest = _rest.xslice_t(1);
        auto _it2 = std::find(_rest.begin(), _rest.end(), ' ');
        auto val_ = _rest.xislice_h(_it2);
        auto type_ = _rest.xislice_t(_it2);
        while (type_.startswith(' '))
            type_ = type_.xslice_t(1);
        // yes, the above actually DTRT even for underlength input

        int val;
        int type = 0;
        // Note for future archeaologists: this code is indented correctly
        if (std::find_if_not(name.begin(), name.end(),
                    [](char c)
                    {
                        return ('0' <= c && c <= '9')
                            || ('A' <= c && c <= 'Z')
                            || ('a' <= c && c <= 'z')
                            || (c == '_');
                    }) != name.end()
                || !extract(val_, &val)
                || (!extract(type_, &type) && type_))
        {
            PRINTF("Bad const line: %s\n"_fmt, line_);
            rv = false;
            continue;
        }
        P<str_data_t> n = add_strp(name);
        n->type = type ? StringCode::PARAM : StringCode::INT;
        n->val = val;
    }
    return rv;
}

/*==========================================
 * マップ変数の変更
 *------------------------------------------
 */
void mapreg_setreg(SIR reg, int val)
{
    mapreg_db.put(reg, val);

    mapreg_dirty = 1;
}

/*==========================================
 * 文字列型マップ変数の変更
 *------------------------------------------
 */
void mapreg_setregstr(SIR reg, XString str)
{
    if (!str)
        mapregstr_db.erase(reg);
    else
        mapregstr_db.insert(reg, str);

    mapreg_dirty = 1;
}

/*==========================================
 * 永続的マップ変数の読み込み
 *------------------------------------------
 */
static
void script_load_mapreg(void)
{
    io::ReadFile in(map_conf.mapreg_txt);

    if (!in.is_open())
        return;

    AString line;
    while (in.getline(line))
    {
        XString buf1, buf2;
        int index = 0;
        if (extract(line,
                    record<'\t'>(
                        record<','>(&buf1, &index),
                        &buf2))
            || extract(line,
                    record<'\t'>(
                        record<','>(&buf1),
                        &buf2)))
        {
            int s = variable_names.intern(buf1);
            SIR key = SIR::from(s, index);
            if (buf1.back() == '$')
            {
                mapregstr_db.insert(key, buf2);
            }
            else
            {
                int v;
                if (!extract(buf2, &v))
                    goto borken;
                mapreg_db.put(key, v);
            }
        }
        else
        {
        borken:
            PRINTF("%s: %s broken data !\n"_fmt, map_conf.mapreg_txt, AString(buf1));
            continue;
        }
    }
    mapreg_dirty = 0;
}

/*==========================================
 * 永続的マップ変数の書き込み
 *------------------------------------------
 */
static
void script_save_mapreg_intsub(SIR key, int data, io::WriteFile& fp)
{
    int num = key.base(), i = key.index();
    ZString name = variable_names.outtern(num);
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%d\n"_fmt, name, data);
        else
            FPRINTF(fp, "%s,%d\t%d\n"_fmt, name, i, data);
    }
}

static
void script_save_mapreg_strsub(SIR key, ZString data, io::WriteFile& fp)
{
    int num = key.base(), i = key.index();
    ZString name = variable_names.outtern(num);
    if (name[1] != '@')
    {
        if (i == 0)
            FPRINTF(fp, "%s\t%s\n"_fmt, name, data);
        else
            FPRINTF(fp, "%s,%d\t%s\n"_fmt, name, i, data);
    }
}

static
void script_save_mapreg(void)
{
    io::WriteLock fp(map_conf.mapreg_txt);
    if (!fp.is_open())
        return;
    for (auto& pair : mapreg_db)
        script_save_mapreg_intsub(pair.first, pair.second, fp);
    for (auto& pair : mapregstr_db)
        script_save_mapreg_strsub(pair.first, pair.second, fp);
    mapreg_dirty = 0;
}

static
void script_autosave_mapreg(TimerData *, tick_t)
{
    if (mapreg_dirty)
        script_save_mapreg();
}

void do_final_script(void)
{
    if (mapreg_dirty >= 0)
        script_save_mapreg();

    mapreg_db.clear();
    mapregstr_db.clear();
    scriptlabel_db.clear();
    userfunc_db.clear();

    str_datam.clear();
}

/*==========================================
 * 初期化
 *------------------------------------------
 */
void do_init_script(void)
{
    script_load_mapreg();

    Timer(gettick() + MAPREG_AUTOSAVE_INTERVAL,
            script_autosave_mapreg,
            MAPREG_AUTOSAVE_INTERVAL
    ).detach();
}
} // namespace map
} // namespace tmwa
