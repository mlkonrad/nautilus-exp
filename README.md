# nautilus-exp

**nautilus-exp** is an experimental fork of [GNOME Files (Nautilus)](https://gitlab.gnome.org/GNOME/nautilus).
It adds the features described below on top of upstream. Everything else, including the upstream
README further down, applies unchanged.

> Please report problems with the features below in this fork's
> [issue tracker](https://github.com/mlkonrad/nautilus-exp/issues), not to GNOME.

## What's new in this fork

### Command bar

A row of buttons below the header bar for the most common file commands, inspired by the
Windows Explorer command bar. All icons are standard GNOME (Adwaita) symbolic icons.

| Button | What it does |
|---|---|
| **New ▾** | Create a new folder, or a new document from your `~/Templates` folder |
| **Cut**, **Copy**, **Paste** | Clipboard operations on the selection |
| **Rename** | Rename the selected item(s) |
| **Compress** | Compress the selection into an archive (.zip, .tar.xz, .7z, encrypted .zip) |
| **Delete** | Move the selection to the Trash |
| **Sort ▾** | Choose the sort order (A–Z, last modified, size, type…) |
| **View ▾** | Switch between grid and list view, zoom, show hidden files, choose columns |
| **Star** (toggle) | Star or unstar the selection, or the current folder when nothing is selected. Unlike the star column, this also works in grid view |
| **Bookmark** (toggle) | Add or remove the selected folder, or the current folder, from the sidebar bookmarks |
| **Filter** | Search the current folder, with the usual type and date filters |
| **…** | Select All, Invert Selection, Open in Console, Properties |

Buttons are greyed out when they don't apply, for example Cut with nothing selected or Paste with
an empty clipboard. On narrow windows the bar is hidden, and the regular bottom bar is shown instead.

The bar can be turned off in **Preferences → General → Show Command Bar**, or from the command line:

```sh
gsettings set org.gnome.nautilus.preferences show-command-bar false
```

### New app icon

A new app icon (yellow filing cabinet with blue handles), plus a matching variant with blue stripes
for development builds.

## Building this fork

The easiest way to try it is a Flatpak development build, which installs as **Files (Devel)**
(`org.gnome.Nautilus.Devel`) next to your regular Files:

1. Install the tooling and the GNOME nightly SDK:
   ```sh
   flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
   flatpak remote-add --user --if-not-exists gnome-nightly https://nightly.gnome.org/gnome-nightly.flatpakrepo
   flatpak install --user flathub org.flatpak.Builder
   flatpak install --user gnome-nightly org.gnome.Sdk//master org.gnome.Platform//master
   ```
2. Copy `build-aux/flatpak/org.gnome.Nautilus.json` to a folder outside the source tree. In the
   copy, change the first source of the `nautilus` module so it uses your checkout instead of
   upstream's git repository:
   ```json
   { "type": "dir", "path": "/path/to/nautilus-exp" }
   ```
3. Build, install and run it from that folder:
   ```sh
   flatpak run org.flatpak.Builder --user --install --force-clean build org.gnome.Nautilus.json
   flatpak run org.gnome.Nautilus.Devel
   ```

Flatpak apps keep their own settings, so the Devel build starts with empty sidebar bookmarks and
stars. To reuse your bookmarks, copy `~/.config/gtk-3.0/bookmarks` to
`~/.var/app/org.gnome.Nautilus.Devel/config/gtk-3.0/bookmarks`.

---

# nautilus
[![Pipeline status](https://gitlab.gnome.org/GNOME/nautilus/badges/main/pipeline.svg)](https://gitlab.gnome.org/GNOME/nautilus/commits/main)
[![coverage report](https://gitlab.gnome.org/GNOME/nautilus/badges/main/coverage.svg)](https://gitlab.gnome.org/GNOME/nautilus/commits/main) 

This is the project of the [Files](https://apps.gnome.org/Nautilus/) app, a file browser for
GNOME, internally known by its historical name `nautilus`.

## Supported version
Only the latest version of Files as provided upstream is supported. Try out the [Flatpak nightly](https://welcome.gnome.org/en/app/Nautilus/#installing-a-nightly-build) installation before filling issues to ensure the installation is reproducible and doesn't have downstream changes on it. In case you cannot reproduce in the nightly installation, don't hesitate to file an issue in your distribution. This is to ensure the issue is well triaged and reaches the proper people.

## Runtime dependencies
- [Bubblewrap](https://github.com/containers/bubblewrap) installed. Used for security reasons.
- [LocalSearch](https://gitlab.gnome.org/GNOME/localsearch) properly set up and with all features enabled. Used for fast search and metadata extraction, starred files and batch renaming.
- [xdg-user-dirs-gtk](https://gitlab.gnome.org/GNOME/xdg-user-dirs-gtk) installed.  Used to create the default bookmarks and update localization.

## Discourse

For more informal discussion we use [GNOME Discourse](https://discourse.gnome.org/tags/nautilus) in the Applications category with the `nautilus` tag. Feel free to open a topic there.

## Extensions

Documentation for the libnautilus-extension API is available [here](https://gnome.pages.gitlab.gnome.org/nautilus/).  Also, if you are interested in developing a Nautilus extension in Python you should refer to the [nautilus-python](https://gnome.pages.gitlab.gnome.org/nautilus-python/) documentation.

## How to report issues

Report issues to the GNOME [issue tracking system](https://gitlab.gnome.org/GNOME/nautilus/issues).

## Feature requests

Files is a core compoment of the GNOME desktop experience. As such, any changes in behavior or appearance only happen in accordance with the [GNOME design team][design-team].

For major changes, it is best to start a discussion on [discourse] and reach out on the [#gnome-design matrix room][design-room], and only involve the issue tracker once agreement has been reached.

In particular mockups must be approved by the design team to be considered for implementation.

For enhancements that are limited in scope and well-defined, it is acceptable to directly open an issue using the shortcoming template.

[design-team]: https://gitlab.gnome.org/Teams/Design
[discourse]: https://discourse.gnome.org/tag/nautilus
[design-room]: https://matrix.to/#/#gnome-design:gnome.org
