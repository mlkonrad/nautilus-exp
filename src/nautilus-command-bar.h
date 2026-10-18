/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <adwaita.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define NAUTILUS_TYPE_COMMAND_BAR (nautilus_command_bar_get_type())

G_DECLARE_FINAL_TYPE (NautilusCommandBar, nautilus_command_bar, NAUTILUS, COMMAND_BAR, AdwBin)

G_END_DECLS
