"""Package private staff entry separately from the normal Unix launcher."""
import hashlib, pathlib, sys, zipfile
platform = sys.argv[1]
assert platform in {"linux-x64", "macos-x64", "macos-arm64"}
dist = pathlib.Path("dist")
if platform == "linux-x64":
    body = '#!/bin/sh\nset -eu\nbase=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\nexport POKEREPORT_STAFF_TOOLS=1\nexec "$base/PokeReport-Launcher-0.2.4-linux-x64.AppImage" "$@"\n'
    filename = "PokeReport-Staff.sh"
else:
    body = '#!/bin/sh\nset -eu\nbase=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)\napp="$base/LunaEternal.app"\nif [ ! -d "$app" ]; then app="/Applications/LunaEternal.app"; fi\nexport POKEREPORT_STAFF_TOOLS=1\nexec "$app/Contents/MacOS/lunaeternal" "$@"\n'
    filename = "PokeReport-Staff.command"
with zipfile.ZipFile(dist / f"PokeReport-Staff-0.2.4-{platform}.zip", "w", zipfile.ZIP_DEFLATED) as archive:
    entry = zipfile.ZipInfo(filename)
    entry.create_system = 3
    entry.external_attr = 0o100755 << 16
    archive.writestr(entry, body)
    archive.writestr("LEEME.txt", "Acceso Staff privado. Extrae este acceso junto al launcher normal. En Mac también puede usar /Applications/LunaEternal.app. El rol de servidor se asigna por separado.\n")
lines = []
for path in sorted(dist.iterdir()):
    if path.is_file() and path.name != "SHA256SUMS":
        lines.append(hashlib.sha256(path.read_bytes()).hexdigest() + "  " + path.name)
(dist / "SHA256SUMS").write_text("\n".join(lines) + "\n", encoding="utf-8")
