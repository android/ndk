/* Copyright (C) 2007-2010 The Android Open Source Project
**
** This software is licensed under the terms of the GNU General Public
** License version 2, as published by the Free Software Foundation, and
** may be copied, distributed, and modified under those terms.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
*/

/*
 * Contains declarations of misc. DWARF utility routines.
 */

#ifndef ELFF_DWARF_UTILS_
#define ELFF_DWARF_UTILS_

#include "dwarf_defs.h"

/* Gets DWARF attribute name string (DW_AT_Xxx) for a given attribute ID.
 * Param:
 *  at - DWARF attribute ID to get name string for.
 * Return:
 *  Attribute name string. Note that this routine returns "DW_AT_Unknown",
 *  if DWARF attribute value passed to this routine has not been recognized.
 */
const char* dwarf_at_name(Dwarf_At at);

/* Gets DWARF form name string (DW_FORM_Xxx) for a given form.
 * Param:
 *  form - DWARF form to get name string for.
 * Return:
 *  Form name string. Note that this routine returns "DW_FORM_Unknown", if
 *  DWARF form value passed to this routine has not been recognized.
 */
const char* dwarf_form_name(Dwarf_Form form);

/* Gets DWARF tag name string (DW_TAG_Xxx) for a given tag.
 * Param:
 *  tag - DWARF tag to get name string for.
 * Return:
 *  Tag name string. Note that this routine returns "DW_TAG_Unknown", if DWARF
 *  tag value passed to this routine has not been recognized.
 */
const char* dwarf_tag_name(Dwarf_Tag tag);

/* Dumps DWARF attribute to stdout.
 * Param:
 *  at - Attribute ID (DW_AT_Xxx)
 *  form - Attribute form (DW_FORM_Xxx)
 *  val - Attribute value.
 */
void dump_attrib(Dwarf_At at, Dwarf_Form form, const Dwarf_Value* val);

/* Dumps DWARF attribute value to stdout.
 * Param:
 *  val - Attribute value.
 */
void dump_value(const Dwarf_Value* val);

#endif  // ELFF_DWARF_UTILS_
