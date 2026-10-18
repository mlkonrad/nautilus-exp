/*
 * SPDX-FileCopyrightText: 2026 The GNOME project contributors
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* A row of buttons for the most common file commands, in the spirit of the
 * command bar of other file managers. It doesn't implement any command itself:
 * every button activates an existing "view." or "slot." action, so their
 * sensitivity follows the selection and the clipboard like in the menus.
 * The only exceptions are the bookmark and star toggles, which act on the
 * selection, if any, and on the current folder otherwise.
 */

#include "nautilus-command-bar.h"

#include <glib/gi18n.h>

#include "nautilus-application.h"
#include "nautilus-bookmark-list.h"
#include "nautilus-file.h"
#include "nautilus-tag-manager.h"
#include "nautilus-toolbar-menu-sections.h"
#include "nautilus-view-info.h"
#include "nautilus-window-slot.h"

struct _NautilusCommandBar
{
    AdwBin parent_instance;

    GtkWidget *new_button;
    GtkWidget *sort_button;
    GtkWidget *view_button;
    GtkWidget *bookmark_button;
    GtkWidget *star_button;
    AdwButtonContent *view_button_content;
    GMenu *view_mode_section;

    GSimpleActionGroup *action_group;

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

/* Returns the folder the bookmark toggle acts on: the selected folder, or the
 * current folder when nothing is selected. */
static GFile *
get_bookmark_target (NautilusCommandBar *self)
{
    if (self->window_slot == NULL)
    {
        return NULL;
    }

    GList *selection = nautilus_window_slot_get_selection (self->window_slot);

    if (selection == NULL)
    {
        GFile *location = nautilus_window_slot_get_location (self->window_slot);

        return location != NULL ? g_object_ref (location) : NULL;
    }

    if (selection->next == NULL && nautilus_file_is_directory (selection->data))
    {
        return nautilus_file_get_location (selection->data);
    }

    return NULL;
}

static void
update_bookmark_actions (NautilusCommandBar *self)
{
    NautilusApplication *app = NAUTILUS_APPLICATION (g_application_get_default ());
    NautilusBookmarkList *bookmarks = nautilus_application_get_bookmarks (app);
    g_autoptr (GFile) target = get_bookmark_target (self);
    gboolean can_bookmark = FALSE;
    gboolean bookmarked = FALSE;

    if (target != NULL)
    {
        can_bookmark = nautilus_bookmark_list_can_bookmark (bookmarks, target);
        bookmarked = nautilus_bookmark_list_contains (bookmarks, target);
    }

    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "bookmarked");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), can_bookmark || bookmarked);
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (bookmarked));

    gtk_widget_set_tooltip_text (self->bookmark_button,
                                 bookmarked ? _("Remove from Bookmarks") : _("Add to Bookmarks"));
}

static void
action_bookmarked (GSimpleAction *action,
                   GVariant      *state,
                   gpointer       user_data)
{
    NautilusCommandBar *self = NAUTILUS_COMMAND_BAR (user_data);
    NautilusApplication *app = NAUTILUS_APPLICATION (g_application_get_default ());
    NautilusBookmarkList *bookmarks = nautilus_application_get_bookmarks (app);
    g_autoptr (GFile) target = get_bookmark_target (self);

    if (target == NULL)
    {
        return;
    }

    /* The state is updated by update_bookmark_actions() once the bookmark
     * list changes. */
    if (g_variant_get_boolean (state))
    {
        nautilus_bookmark_list_add (bookmarks, target, -1);
    }
    else
    {
        nautilus_bookmark_list_remove (bookmarks, target);
    }
}

/* Mirrors the rules used to enable the "slot.star-current-directory" and
 * "slot.unstar-current-directory" actions, and the star column for selected
 * items. */
static void
update_star_action (NautilusCommandBar *self)
{
    NautilusTagManager *tag_manager = nautilus_tag_manager_get ();
    GFile *location = NULL;
    GList *selection = NULL;
    gboolean can_star = FALSE;
    gboolean starred = FALSE;

    if (self->window_slot != NULL)
    {
        location = nautilus_window_slot_get_location (self->window_slot);
        selection = nautilus_window_slot_get_selection (self->window_slot);
    }

    if (location != NULL && selection == NULL)
    {
        g_autofree char *uri = g_file_get_uri (location);

        can_star = nautilus_tag_manager_can_star_location (tag_manager, location);
        starred = nautilus_tag_manager_file_is_starred (tag_manager, uri);
    }
    else if (location != NULL)
    {
        gboolean any_starred = FALSE;
        gboolean all_starred = TRUE;

        for (GList *l = selection; l != NULL; l = l->next)
        {
            g_autofree char *uri = nautilus_file_get_activation_uri (l->data);

            if (nautilus_tag_manager_file_is_starred (tag_manager, uri))
            {
                any_starred = TRUE;
            }
            else
            {
                all_starred = FALSE;
            }
        }

        /* A selection mixing starred and non-starred items can't be toggled. */
        can_star = (nautilus_tag_manager_can_star_contents (tag_manager, location) || any_starred) &&
                   (all_starred || !any_starred);
        starred = all_starred;
    }

    GAction *action = g_action_map_lookup_action (G_ACTION_MAP (self->action_group), "starred");
    g_simple_action_set_enabled (G_SIMPLE_ACTION (action), can_star);
    g_simple_action_set_state (G_SIMPLE_ACTION (action), g_variant_new_boolean (can_star && starred));

    gtk_button_set_icon_name (GTK_BUTTON (self->star_button),
                              can_star && starred ? "starred-symbolic" : "non-starred-symbolic");
    gtk_widget_set_tooltip_text (self->star_button,
                                 can_star && starred ? _("Unstar") : _("Star"));
}

static void
action_starred (GSimpleAction *action,
                GVariant      *state,
                gpointer       user_data)
{
    NautilusCommandBar *self = NAUTILUS_COMMAND_BAR (user_data);
    gboolean star = g_variant_get_boolean (state);

    if (self->window_slot == NULL)
    {
        return;
    }

    /* The state is updated by update_star_action() once the tag manager
     * reports the change. */
    if (nautilus_window_slot_get_selection (self->window_slot) == NULL)
    {
        gtk_widget_activate_action (GTK_WIDGET (self),
                                    star ? "slot.star-current-directory" : "slot.unstar-current-directory",
                                    NULL);
    }
    else if (star)
    {
        /* Not using "view.star" and "view.unstar", because the latter is only
         * enabled in the Starred view. Undo is handled by the tag manager. */
        nautilus_tag_manager_star_files (nautilus_tag_manager_get (),
                                         G_OBJECT (self),
                                         nautilus_window_slot_get_selection (self->window_slot),
                                         nautilus_tag_manager_announce_starred_cb,
                                         self,
                                         NULL);
    }
    else
    {
        nautilus_tag_manager_unstar_files (nautilus_tag_manager_get (),
                                           G_OBJECT (self),
                                           nautilus_window_slot_get_selection (self->window_slot),
                                           nautilus_tag_manager_announce_unstarred_cb,
                                           self,
                                           NULL);
    }
}

static const GActionEntry command_bar_entries[] =
{
    { .name = "bookmarked", .state = "false", .change_state = action_bookmarked },
    { .name = "starred", .state = "false", .change_state = action_starred },
};

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
    update_bookmark_actions (self);
    update_star_action (self);
}

/* The sort section is provided by the view through the slot, like for
 * NautilusViewControls. There is none when the view has no view options. */
static void
update_view_options (NautilusCommandBar *self)
{
    NautilusToolbarMenuSections *sections = NULL;

    if (self->window_slot != NULL)
    {
        sections = nautilus_window_slot_get_toolbar_menu_sections (self->window_slot);
    }

    gtk_widget_set_sensitive (self->sort_button, sections != NULL);
    gtk_widget_set_sensitive (self->view_button, sections != NULL);
    gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (self->sort_button),
                                    sections != NULL ? sections->sort_section : NULL);
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
        g_signal_connect_swapped (self->window_slot, "notify::toolbar-menu-sections",
                                  G_CALLBACK (update_view_options), self);
        g_signal_connect_swapped (self->window_slot, "notify::templates-menu",
                                  G_CALLBACK (update_new_menu), self);
        g_signal_connect_swapped (self->window_slot, "notify::icon-name",
                                  G_CALLBACK (update_view_button_icon), self);
        g_signal_connect_swapped (self->window_slot, "notify::selection",
                                  G_CALLBACK (update_bookmark_actions), self);
        g_signal_connect_swapped (self->window_slot, "notify::selection",
                                  G_CALLBACK (update_star_action), self);
    }

    adjust_to_location (self);
    update_view_options (self);
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

    g_clear_object (&self->action_group);

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
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, bookmark_button);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, star_button);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, view_button_content);
    gtk_widget_class_bind_template_child (widget_class, NautilusCommandBar, view_mode_section);
}

static void
nautilus_command_bar_init (NautilusCommandBar *self)
{
    gtk_widget_init_template (GTK_WIDGET (self));

    self->action_group = g_simple_action_group_new ();
    g_action_map_add_action_entries (G_ACTION_MAP (self->action_group),
                                     command_bar_entries,
                                     G_N_ELEMENTS (command_bar_entries),
                                     self);
    gtk_widget_insert_action_group (GTK_WIDGET (self),
                                    "command-bar",
                                    G_ACTION_GROUP (self->action_group));

    NautilusApplication *app = NAUTILUS_APPLICATION (g_application_get_default ());
    g_signal_connect_object (nautilus_application_get_bookmarks (app), "changed",
                             G_CALLBACK (update_bookmark_actions), self,
                             G_CONNECT_SWAPPED);
    g_signal_connect_object (nautilus_tag_manager_get (), "starred-changed",
                             G_CALLBACK (update_star_action), self,
                             G_CONNECT_SWAPPED);

    g_autoptr (GMenuItem) grid_item = g_menu_item_new (_("_Grid"), NULL);
    g_autoptr (GMenuItem) list_item = g_menu_item_new (_("_List"), NULL);

    g_menu_item_set_action_and_target_value (grid_item, "slot.files-view-mode",
                                             g_variant_new_uint32 (NAUTILUS_VIEW_GRID_ID));
    g_menu_item_set_action_and_target_value (list_item, "slot.files-view-mode",
                                             g_variant_new_uint32 (NAUTILUS_VIEW_LIST_ID));
    g_menu_append_item (self->view_mode_section, grid_item);
    g_menu_append_item (self->view_mode_section, list_item);

    update_new_menu (self);
    update_bookmark_actions (self);
    update_star_action (self);
}
