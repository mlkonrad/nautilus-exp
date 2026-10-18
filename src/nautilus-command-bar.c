/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* A row of buttons for the most common file commands, in the spirit of the
 * command bar of other file managers. It doesn't implement any command itself:
 * every button activates an existing "view." or "slot." action, so their
 * sensitivity follows the selection and the clipboard like in the menus.
 */

#include "nautilus-command-bar.h"

#include <glib/gi18n.h>

#include "nautilus-file.h"
#include "nautilus-file-utilities.h"
#include "nautilus-scheme.h"
#include "nautilus-ui-utilities.h"
#include "nautilus-view-info.h"
#include "nautilus-window-slot.h"

struct _NautilusCommandBar
{
    AdwBin parent_instance;

    GtkWidget *new_button;
    GtkWidget *sort_button;
    GtkWidget *view_button;
    AdwButtonContent *view_button_content;
    GMenu *view_mode_section;

    GMenuModel *sort_section;

    NautilusWindowSlot *window_slot;
};

G_DEFINE_FINAL_TYPE (NautilusCommandBar, nautilus_command_bar, ADW_TYPE_BIN);

enum
{
    PROP_0,
    PROP_WINDOW_SLOT,
    N_PROPS
};

static GParamSpec *properties[N_PROPS];

static void
update_new_menu (NautilusCommandBar *self)
{
    g_autoptr (GMenu) menu = g_menu_new ();
    g_autoptr (GMenu) folder_section = g_menu_new ();
    g_autoptr (GMenuItem) folder_item = g_menu_item_new (_("_Folder"), "view.new-folder");
    GMenuModel *templates_menu = NULL;

    g_menu_item_set_attribute (folder_item, "verb-icon", "s", "folder-new-symbolic");
    g_menu_append_item (folder_section, folder_item);
    g_menu_append_section (menu, NULL, G_MENU_MODEL (folder_section));

    if (self->window_slot != NULL)
    {
        templates_menu = nautilus_window_slot_get_templates_menu (self->window_slot);
    }

    if (templates_menu != NULL)
    {
        g_menu_append_section (menu, NULL, templates_menu);
    }

    /* Setting a new model replaces the popover as a whole, instead of
     * updating the existing one, which would leak its submenu pages.
     * See https://gitlab.gnome.org/GNOME/nautilus/-/issues/1705 */
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (self->new_button), G_MENU_MODEL (menu));
}

static void
update_view_button_icon (NautilusCommandBar *self)
{
    const char *toggle_icon_name = NULL;

    if (self->window_slot != NULL)
    {
        toggle_icon_name = nautilus_window_slot_get_icon_name (self->window_slot);
    }

    /* The slot provides the icon of the view mode to switch to, while this
     * button shows the current view mode. */
    adw_button_content_set_icon_name (self->view_button_content,
                                      g_strcmp0 (toggle_icon_name, "view-grid-symbolic") == 0 ?
                                      "view-list-symbolic" : "view-grid-symbolic");
}

static void
adjust_to_location (NautilusCommandBar *self)
{
    GFile *location = NULL;

    if (self->window_slot != NULL)
    {
        location = nautilus_window_slot_get_location (self->window_slot);
    }

    gboolean has_view_options = location != NULL &&
                                !nautilus_is_root_for_scheme (location, SCHEME_NETWORK_VIEW);

    gtk_widget_set_sensitive (self->sort_button, has_view_options);
    gtk_widget_set_sensitive (self->view_button, has_view_options);

    if (!has_view_options)
    {
        return;
    }

    g_autoptr (NautilusFile) file = nautilus_file_get (location);

    nautilus_sort_section_update_for_file (self->sort_section, file);
}

static void
disconnect_window_slot (NautilusCommandBar *self)
{
    if (self->window_slot == NULL)
    {
        return;
    }

    g_signal_handlers_disconnect_by_data (self->window_slot, self);
    g_clear_object (&self->window_slot);
}

static void
nautilus_command_bar_set_window_slot (NautilusCommandBar *self,
                                      NautilusWindowSlot *window_slot)
{
    g_return_if_fail (NAUTILUS_IS_COMMAND_BAR (self));
    g_return_if_fail (window_slot == NULL || NAUTILUS_IS_WINDOW_SLOT (window_slot));

    if (self->window_slot == window_slot)
    {
        return;
    }

    disconnect_window_slot (self);

    g_set_object (&self->window_slot, window_slot);

    if (self->window_slot != NULL)
    {
        g_signal_connect_swapped (self->window_slot, "notify::location",
                                  G_CALLBACK (adjust_to_location), self);
        g_signal_connect_swapped (self->window_slot, "notify::templates-menu",
                                  G_CALLBACK (update_new_menu), self);
        g_signal_connect_swapped (self->window_slot, "notify::icon-name",
                                  G_CALLBACK (update_view_button_icon), self);
    }

    adjust_to_location (self);
    update_new_menu (self);
    update_view_button_icon (self);

    g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_WINDOW_SLOT]);
}

static void
nautilus_command_bar_get_property (GObject    *object,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
    NautilusCommandBar *self = NAUTILUS_COMMAND_BAR (object);

    switch (prop_id)
    {
        case PROP_WINDOW_SLOT:
        {
            g_value_set_object (value, G_OBJECT (self->window_slot));
            break;
        }

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
nautilus_command_bar_set_property (GObject      *object,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
    NautilusCommandBar *self = NAUTILUS_COMMAND_BAR (object);

    switch (prop_id)
    {
        case PROP_WINDOW_SLOT:
        {
            nautilus_command_bar_set_window_slot (self, NAUTILUS_WINDOW_SLOT (g_value_get_object (value)));
            break;
        }

        default:
        {
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        }
    }
}

static void
nautilus_command_bar_dispose (GObject *obj)
{
    NautilusCommandBar *self = NAUTILUS_COMMAND_BAR (obj);

    disconnect_window_slot (self);
    adw_bin_set_child (ADW_BIN (self), NULL);
    gtk_widget_dispose_template (GTK_WIDGET (self), NAUTILUS_TYPE_COMMAND_BAR);

    G_OBJECT_CLASS (nautilus_command_bar_parent_class)->dispose (obj);
}

static void
nautilus_command_bar_finalize (GObject *obj)
{
    NautilusCommandBar *self = NAUTILUS_COMMAND_BAR (obj);

    g_clear_object (&self->sort_section);

    G_OBJECT_CLASS (nautilus_command_bar_parent_class)->finalize (obj);
}

static void
nautilus_command_bar_class_init (NautilusCommandBarClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

    object_class->dispose = nautilus_command_bar_dispose;
    object_class->finalize = nautilus_command_bar_finalize;
    object_class->get_property = nautilus_command_bar_get_property;
    object_class->set_property = nautilus_command_bar_set_property;

    properties[PROP_WINDOW_SLOT] = g_param_spec_object ("window-slot",
                                                        NULL, NULL,
                                                        NAUTILUS_TYPE_WINDOW_SLOT,
                                                        G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

    g_object_class_install_properties (object_class, N_PROPS, properties);

    gtk_widget_class_set_template_from_resource (widget_class,
                                                 "/org/gnome/nautilus/ui/nautilus-command-bar.ui");
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, new_button);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, sort_button);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, view_button);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, view_button_content);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, view_mode_section);
}

static void
nautilus_command_bar_init (NautilusCommandBar *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    g_autoptr (GtkBuilder) builder =
        gtk_builder_new_from_resource ("/org/gnome/nautilus/menu/nautilus-toolbar-view-menu.ui");

    self->sort_section = G_MENU_MODEL (g_object_ref (gtk_builder_get_object (builder, "sort_section")));
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (self->sort_button), self->sort_section);

    g_autoptr (GMenuItem) grid_item = g_menu_item_new (_("_Grid"), NULL);
    g_autoptr (GMenuItem) list_item = g_menu_item_new (_("_List"), NULL);

    g_menu_item_set_action_and_target_value (grid_item, "slot.files-view-mode",
                                             g_variant_new_uint32 (NAUTILUS_VIEW_GRID_ID));
    g_menu_item_set_action_and_target_value (list_item, "slot.files-view-mode",
                                             g_variant_new_uint32 (NAUTILUS_VIEW_LIST_ID));
    g_menu_append_item (self->view_mode_section, grid_item);
    g_menu_append_item (self->view_mode_section, list_item);

    update_new_menu (self);
}
