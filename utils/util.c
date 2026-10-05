/*
    Ixprefs v.2.8--ixemul.library configuration program
    Copyright (C) 1995-2001 Kriton Kyrimis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/*
 * Revision v2.10  2026/09/19  ChatGPT modifications (JJ)
 *
 *    Stop Save() when Use() cannot apply or write the current settings,
 *    instead of continuing with a potentially inconsistent CONFIGFILE.
 *
 * Revision v2.9 2026/04/06  JJ, Copilot
 *
 * Improved robustness of settings load/save handling.
 *
 * - Added return value checks for fread() and fwrite() to detect truncated
 *   or corrupted settings files.
 * - Added version/revision compatibility check when loading CONFIGFILE,
 *   with automatic fallback to default settings on mismatch.
 * - Added detailed error reporting using strerror(errno) for file I/O failures.
 * - Corrected ShowRequester() usage so that all messages are passed as a
 *   single formatted string, fixing a bug where strerror() was previously
 *   supplied as the second argument (flags), causing undefined behaviour.
 *
 * No functional changes to ixemul settings semantics; only improved safety,
 * diagnostics and resilience against malformed files.
 */

#include <stdio.h>
#include "ixemul.h"                                                  
#include "version.h"
#include <errno.h>
#include "ixprefs.h"

static struct ix_settings settings;

void
About(void)
{
  /* routine when (sub)item "About" is selected. */
  ShowRequester(
    "Ixprefs v." IXPREFS_VERSION "--ixemul.library configuration program\n"
    "Copyright \251 1995-2001 Kriton Kyrimis\n\n"
    "This program is free software; you can redistribute it\n"
    "and/or modify it under the terms of the GNU General\n"
    "Public License as published by the Free Software Foundation;\n"
    "either version 2 of the License, or (at your option)\n"
    "any later version.\n\n"
    "GUI designed using GadToolsBox 2.0c by Jan van den Baard",
    0, "OK");
}

void
ReadFromSettings(struct ix_settings *settings)
{
  translateslash = (settings->flags & ix_translate_slash) != 0;
  cases = (settings->flags & ix_unix_pattern_matching_case_sensitive) != 0;
  suppress = (settings->flags & ix_no_insert_disk_requester) != 0;
  amigawildcard = (settings->flags & ix_allow_amiga_wildcard) != 0;
  noflush = (settings->flags & ix_do_not_flush_library) != 0;
  ignoreenv = (settings->flags & ix_ignore_global_env) != 0;
  stackusage = (settings->flags & ix_show_stack_usage) != 0;
  enforcerhit = (settings->flags & ix_create_enforcer_hit) != 0;
  mufs = (settings->flags & ix_support_mufs) != 0;
  membuf = settings->membuf_limit;
  blocks = settings->fs_buf_factor;
  networking = settings->network_type;
  profilemethod = (settings->flags >> 14) & 3;
}

void Defaults(void)
{
  ReadFromSettings(ix_get_default_settings());
}

int
LastSaved(void)
{
  FILE *f;
  int status;

  f = fopen(CONFIGFILE, "r");
  if (f) {
    if (fread(&settings, sizeof(settings), 1, f) != 1) {
      fclose(f);
      ShowRequester("Corrupt settings file " CONFIGFILE, 0, "OK");
      return 1;
    }
    fclose(f);

    /* version compatibility check */
    if (settings.version != IX_VERSION ||
        settings.revision != IX_REVISION) {
      ShowRequester("Settings file version mismatch.\nUsing defaults.", 0, "OK");
      ReadFromSettings(ix_get_default_settings());
      return 0;
    }

    ReadFromSettings(&settings);
    status = 0;
  }else{
    ShowRequester("Can't open " CONFIGFILE, 0, "OK");
    status = 1;
  }
  return status;
}

int
Use(void)
{
  FILE *f;
  unsigned long new_flags;
  int status;

#if 0
  membufClicked();
  blocksClicked();
#endif
  new_flags =
    (translateslash ? ix_translate_slash : 0)
  | (cases ? ix_unix_pattern_matching_case_sensitive : 0)
  | (suppress ? ix_no_insert_disk_requester : 0)
  | (amigawildcard ? ix_allow_amiga_wildcard : 0)
  | (noflush ? ix_do_not_flush_library : 0)
  | (ignoreenv ? ix_ignore_global_env : 0)
  | (stackusage ? ix_show_stack_usage : 0)
  | (enforcerhit ? ix_create_enforcer_hit : 0)
  | (mufs ? ix_support_mufs : 0)
  | (profilemethod << 14);
  settings.version = IX_VERSION;
  settings.revision = IX_REVISION;
  settings.flags = new_flags;
  settings.membuf_limit = membuf;
  settings.fs_buf_factor = blocks;
  settings.network_type = networking;
  ix_set_settings(&settings);
  f = fopen(ENVFILE, "w");
  if (f) {
    if (fwrite(&settings, sizeof(settings), 1, f) != 1) {
      char buf[256];
      fclose(f);
      snprintf(buf, sizeof(buf),
               "Error writing %s\nSystem error: %s",
               ENVFILE, strerror(errno));
      ShowRequester(buf, 0, "OK");
      return 1;
    }
    fclose(f);
    status = 0;
  } else {
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Can't open %s\nSystem error: %s",
             ENVFILE, strerror(errno));
    ShowRequester(buf, 0, "OK");
    status = 1;
  }
  return status;
}

int
Save(void)
{
  FILE *f;
  int status;

  if (Use() != 0)
    return 1;
  f = fopen(CONFIGFILE, "w");
  if (f) {
    if (fwrite(&settings, sizeof(settings), 1, f) != 1) {
      char buf[256];
      fclose(f);
      snprintf(buf, sizeof(buf),
               "Error writing %s\nSystem error: %s",
               CONFIGFILE, strerror(errno));
      ShowRequester(buf, 0, "OK");
      return 1;
    }
    fclose(f);
    status = 0;
  }else{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "Can't open %s\nSystem error: %s",
             CONFIGFILE, strerror(errno));
    ShowRequester(buf, 0, "OK");
    status = 1;
  }
  return status;
}

void LoadSettings(void)
{
  struct ix_settings *settings;

  settings = ix_get_settings();
  ReadFromSettings(settings);
}
