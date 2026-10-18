#!/bin/bash
# Builds the nautilus-exp RPMs for Fedora in a clean Fedora container.
#
# The fork's changes are applied as a single patch, generated from git as the
# difference between the upstream release tag (the spec's Version) and the
# current commit. The packaging directory itself is left out of the patch.
#
# Usage: packaging/fedora/build-rpm.sh
# Environment:
#   WORK            Build directory (default: ~/.cache/nautilus-exp-rpm)
#   FEDORA_VERSION  Fedora release to build for (default: 44)

set -euo pipefail

PKG_DIR=$(cd "$(dirname "$0")" && pwd)
REPO_DIR=$(git -C "$PKG_DIR" rev-parse --show-toplevel)
WORK=${WORK:-$HOME/.cache/nautilus-exp-rpm}
FEDORA_VERSION=${FEDORA_VERSION:-44}

VERSION=$(sed -n 's/^Version:[[:space:]]*//p' "$PKG_DIR/nautilus.spec")
MAJOR=${VERSION%%.*}
TARBALL=nautilus-$VERSION.tar.xz

if ! git -C "$REPO_DIR" rev-parse -q --verify "refs/tags/$VERSION" >/dev/null; then
    echo "Upstream tag $VERSION not found. Fetch it with:" >&2
    echo "  git fetch https://gitlab.gnome.org/GNOME/nautilus.git tag $VERSION" >&2
    exit 1
fi

mkdir -p "$WORK/rpmbuild/SPECS" "$WORK/rpmbuild/SOURCES"
cp "$PKG_DIR/nautilus.spec" "$WORK/rpmbuild/SPECS/"
cp "$PKG_DIR/default-terminal.patch" "$WORK/rpmbuild/SOURCES/"

git -C "$REPO_DIR" diff "$VERSION" HEAD -- . ':(exclude)packaging' \
    > "$WORK/rpmbuild/SOURCES/nautilus-exp.patch"

cd "$WORK/rpmbuild/SOURCES"
if [ ! -f "$TARBALL" ]; then
    curl -sSfLO "https://download.gnome.org/sources/nautilus/$MAJOR/$TARBALL"
fi
echo "$(sed -n "s/^SHA512 ($TARBALL) = //p" "$PKG_DIR/sources")  $TARBALL" | sha512sum -c -

podman run --rm -v "$WORK:/work:Z" "registry.fedoraproject.org/fedora:$FEDORA_VERSION" \
    bash -euo pipefail -c '
        dnf -y -q install rpm-build dnf-plugins-core
        dnf -y -q builddep /work/rpmbuild/SPECS/nautilus.spec
        rpmbuild --define "_topdir /work/rpmbuild" -ba /work/rpmbuild/SPECS/nautilus.spec
    '

echo
echo "RPMs written to $WORK/rpmbuild/RPMS/x86_64/"
