#!/usr/bin/env bash
#
# Builds the welcome app from the source apps_image/welcome.pin names and
# leaves it where the release picks it up:
#
#   apps_image/<project>.kfw      the app with its app data (tools/mkkfw.py)
#   apps_image/<project>.commit   the commit it was built from, and the kfw's SHA-256
#   storage_image/.kode/appsdata/<project>/
#                                 the kfw's own app.json and icon.k64, which
#                                 the build puts in the FAT image
#
# Every factory pack carries the welcome app installed, with its data folder
# on the FAT as an install on the board would leave it, and the release
# refuses a kfw whose stamp is not the pin. Run it before `idf.py build`: the
# FAT image is made by that build.
#
# The welcome app lives in its own repository and builds with its own SDK pin,
# so like the C5's firmware it is the one image here that `idf.py build`
# cannot make. The pin is one line, "<repository> <app folder> <commit>";
# moving it is a deliberate change, the same as moving the C5's.
#
#   usage: tools/build_welcome.sh
#   exit:  0 on success, 1 if the pin cannot be read, the source is not at the
#          pin, the build fails, or the app carries no appdata/app.json
set -euo pipefail
cd "$(dirname "$0")/.."

pin_file="apps_image/welcome.pin"
read -r repo app_dir pinned < "$pin_file" || true
if [ -z "${repo:-}" ] || [ -z "${app_dir:-}" ] || [ -z "${pinned:-}" ]; then
    echo "$pin_file must hold one line: <repository> <app folder> <commit>" >&2
    exit 1
fi

if [ -z "${IDF_PATH:-}" ]; then
    echo "ESP-IDF is not on this shell. Source its export.sh first." >&2
    exit 1
fi

# The same fallback as build_c5.sh: an EIM install defines idf.py as a shell
# function, which a child script cannot see.
if command -v idf.py >/dev/null 2>&1; then
    idf() { idf.py "$@"; }
else
    idf() { python "$IDF_PATH/tools/idf.py" "$@"; }
fi

# The source is cloned here once; CI clones it before this runs, with the
# deploy key the build's container does not have.
src="build_welcome/src"
build_dir="build_welcome/build"
if [ ! -d "$src/.git" ]; then
    git clone -q "$repo" "$src"
fi
if ! git -C "$src" cat-file -e "${pinned}^{commit}" 2>/dev/null; then
    git -C "$src" fetch -q origin
fi
git -C "$src" -c advice.detachedHead=false checkout -q "$pinned"

# A cold checkout's first configure fetches the SDK's board component and then
# stops on purpose, so the board settings are read on the second; the app's own
# CMakeLists says so. Hence one retry, and only one.
echo "Building the welcome app from $repo @ ${pinned:0:7} ($app_dir) ..."
if ! idf -C "$src/$app_dir" -B "$build_dir" build; then
    idf -C "$src/$app_dir" -B "$build_dir" build
fi

project="$(python3 -c 'import json, sys; print(json.load(open(sys.argv[1]))["project_name"])' \
           "$build_dir/project_description.json")"
image="$build_dir/$project.bin"
if [ ! -f "$image" ]; then
    echo "the build produced no $image" >&2
    exit 1
fi

# The app's data folder, as the apps of kode-apps carry it: app.json and an
# icon. Without it Apps shows the welcome app with no icon, colour or words.
appdata="$src/$app_dir/appdata"
if [ ! -f "$appdata/app.json" ]; then
    echo "the welcome app carries no $app_dir/appdata/app.json at ${pinned:0:7}" >&2
    exit 1
fi

# One container, and the FAT's folder written from that container's own block:
# the two cannot differ. Only this app's folder lives under storage_image/.kode.
kfw="apps_image/$project.kfw"
rm -rf storage_image/.kode
rm -f "apps_image/$project.bin"
python3 tools/mkkfw.py --target app --name "$project" --appdata "$appdata" \
        --appdata-out "storage_image/.kode/appsdata/$project" "$image" "$kfw"

# The stamp: "<commit> <sha256>", -dirty for a tree with local changes, which
# no release accepts.
sha256() { if command -v sha256sum >/dev/null 2>&1; then sha256sum "$1"; else shasum -a 256 "$1"; fi | awk '{ print $1 }'; }
commit="$(git -C "$src" rev-parse HEAD)"
if [ -n "$(git -C "$src" status --porcelain)" ]; then
    commit="${commit}-dirty"
fi
printf '%s %s\n' "$commit" "$(sha256 "$kfw")" > "apps_image/$project.commit"

size="$(wc -c < "$kfw" | tr -d ' ')"
echo
echo "$kfw  <-  ${repo##*/} @ ${pinned:0:7}  ($size bytes), stamped in $project.commit"
echo "storage_image/.kode/appsdata/$project/  <-  its app data, for the FAT image"
