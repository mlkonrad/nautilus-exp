# Fedora packaging for nautilus-exp

Builds nautilus-exp as native Fedora RPMs that replace the system `nautilus` and
`nautilus-extensions` packages.

The spec is [Fedora's own](https://src.fedoraproject.org/rpms/nautilus) (f44 branch), with these changes:

- `Release` is `1%{?dist}.exp1`. That sorts just above Fedora's matching release, and the `exp1`
  suffix marks the package as this fork.
- The fork's changes are applied as `nautilus-exp.patch`, after Fedora's `default-terminal.patch`.
- A changelog entry for the fork is added on top of Fedora's changelog.

## Building

Requires `git`, `curl` and `podman`. Nothing is installed on the host: the build runs in a clean
Fedora container.

```sh
packaging/fedora/build-rpm.sh
```

The script:

1. Generates `nautilus-exp.patch` with `git diff <Version> HEAD`, leaving this directory out.
   It needs the upstream release tag (for example `50.4`) in your local repository.
2. Downloads the release tarball from download.gnome.org and checks it against `sources`.
3. Builds the RPMs in a `fedora:44` container. Override the Fedora release with `FEDORA_VERSION`
   and the build directory (default `~/.cache/nautilus-exp-rpm`) with `WORK`.

## Installing

```sh
sudo dnf install ~/.cache/nautilus-exp-rpm/rpmbuild/RPMS/x86_64/nautilus-{,extensions-}50.4-1.fc44.exp1.x86_64.rpm
nautilus -q
```

To keep Fedora's Nautilus updates from replacing it:

```sh
sudo dnf versionlock add nautilus nautilus-extensions
```

Remove the lock (`sudo dnf versionlock delete nautilus nautilus-extensions`) before upgrading to a
new Fedora release.

To go back to Fedora's Nautilus:

```sh
sudo dnf distro-sync nautilus nautilus-extensions
```

## Updating to a new Nautilus release

1. Rebase this branch onto the new upstream tag, for example `git rebase --onto 50.5 50.4`.
2. Update `Version`, `sources` and, if Fedora changed them, the spec and `default-terminal.patch`,
   using Fedora's [dist-git](https://src.fedoraproject.org/rpms/nautilus).
3. Add a changelog entry and run `build-rpm.sh`.

A new GNOME major version (for example Nautilus 51 in Fedora 45) needs the fork's changes ported to
a new `stable-51` branch first.
