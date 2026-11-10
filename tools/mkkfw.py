#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Wrap an ESP-IDF image in a Kode firmware container.

The header is 72 bytes, little-endian, and everything in it describes the
*decompressed* image rather than the payload that carries it. That is
deliberate on both counts:

  - the region allocator on the board needs the decompressed length before
    it writes the first byte, and Content-Length over HTTP describes the
    compressed payload instead;
  - the SHA-256 is of the image, because the image is the thing that has to
    be right. A digest of the payload would be satisfied by a correct
    transfer of a corrupt file.

Layout:

    0x00  4   magic "KFW1"
    0x04  2   header version (1)
    0x06  2   header length (72)
    0x08  1   target: 0 os, 1 app, 2 c5
    0x09  1   compression: 0 none, 1 deflate
    0x0A  2   flags: bit 0 = an app-data block follows this header
              (target app only), every other bit reserved, zero
    0x0C  4   payload length, the bytes that follow this header
    0x10  4   image length, after decompression
    0x14  32  SHA-256 of the decompressed image
    0x34  4   CRC32 of bytes 0x00..0x33
    0x38  16  name, NUL-padded; empty unless target is app

With flags bit 0 (`--appdata DIR`, target app only) the app's metadata
travels right after the header, before the payload, uncompressed. It seals
itself, so the image SHA-256 and the header CRC cover what they always did,
and `payload length` still counts the payload alone:

    0x00  4   magic "KAD1"
    0x04  2   json_len, 1..1024
    0x06  2   icon_len, 0 or 12288 (one 64x64 RGB565 + A8 icon.k64)
    0x08  4   CRC32 of the json_len + icon_len bytes that follow (the
              header's flavour, see esp_crc32)
    0x0C  json_len bytes of app.json, exactly as the board will store it
    ...   icon_len bytes of icon.k64

The sizes and offsets are KFW_AD_* in sdk/components/kdrv/apps/include/
kdrv_kfw.h; the board checks the block with the parser the folder's reader
uses, and validate() in kappdata.py is that parser's host twin.

`--appdata-out DIR` also writes that block's app.json and icon.k64 into DIR,
as the board stores them after an install: the data folder a factory pack
carries beside an app it installs itself. parse() reads a container back,
for the tool that builds that pack (tools/mkapps.py).
"""
import argparse
import hashlib
import json
import os
import struct
import sys
import zlib

import kappdata

MAGIC = b"KFW1"
HDR_VERSION = 1
HDR_LEN = 72
NAME_MAX = 16
ESP_IMAGE_MAGIC = 0xE9
TARGETS = {"os": 0, "app": 1, "c5": 2}
FLAG_APPDATA = 0x0001       # flags bit 0: an app-data block follows the header
AD_MAGIC = b"KAD1"          # the block's own magic
AD_HDR_LEN = 12             # magic, json_len, icon_len, crc
AD_JSON_MAX = 1024          # KFW_AD_JSON_MAX: the longest app.json a block carries
AD_ICON_LEN = kappdata.K64_BYTES   # KFW_AD_ICON_LEN: the one icon size, 12288
AD_ICON_NAME = "icon.k64"   # what the icon key reads once an icon is packed
AD_JSON_NAME = "app.json"   # the data folder's other file
HDR_FIXED = "<4sHHBBHII32s"  # the header up to its CRC: 52 bytes
HDR_CRC_OFF = struct.calcsize(HDR_FIXED)
AD_HDR = "<4sHHI"           # the block's header: magic, json_len, icon_len, crc
COMPRESSION = (0, 1)        # none, deflate


def esp_crc32(data: bytes) -> int:
    """The CRC32 the board computes, which is not zlib's default.

    kdrv_kfw.c seals the header with esp_rom_crc32_le(UINT32_MAX, ...). The
    ROM routine inverts the seed it is given, so passing ~0 starts the raw
    CRC at 0 - where zlib's default starts it at ~0. Passing 0xFFFFFFFF as
    zlib's running value is what reproduces it. Getting this wrong produces
    a tool and a test that agree with each other and with nothing on the
    board, which is exactly how it was found.
    """
    return zlib.crc32(data, 0xFFFFFFFF) & 0xFFFFFFFF


def appdata_block(json_bytes: bytes, icon: bytes) -> bytes:
    """Return the app-data block for an app.json and an icon (maybe empty).

    The sizes are checked against what the board accepts, so a block that
    leaves here is one the decoder keeps rather than drops.
    """
    if not 0 < len(json_bytes) <= AD_JSON_MAX:
        raise ValueError(f"app.json: {len(json_bytes)} bytes once written, "
                         f"must be 1..{AD_JSON_MAX}")
    if len(icon) not in (0, AD_ICON_LEN):
        raise ValueError(f"icon: {len(icon)} bytes, must be 0 or {AD_ICON_LEN}")
    body = json_bytes + icon
    head = struct.pack(AD_HDR, AD_MAGIC, len(json_bytes), len(icon),
                       esp_crc32(body))
    if len(head) != AD_HDR_LEN:
        raise AssertionError(f"block header is {len(head)} bytes, "
                             f"must be {AD_HDR_LEN}")
    return head + body


def load_appdata(folder: str) -> tuple:
    """Read `folder` the way `kcli appdata put` does: (app.json bytes, icon).

    Raises ValueError naming the field that the board would refuse. The icon
    comes back empty when the folder has none.
    """
    path = os.path.join(folder, "app.json")
    try:
        with open(path, "rb") as fh:
            obj = kappdata.validate(json.loads(fh.read()))
    except OSError as exc:
        raise ValueError(f"cannot read {path}: {exc}") from exc
    except json.JSONDecodeError as exc:
        raise ValueError(f"{path}: {exc}") from exc

    icon = b""
    icon_path = kappdata.find_icon(folder, obj)
    if icon_path:
        try:
            icon = kappdata.to_k64(icon_path)
        except ImportError as exc:
            raise ValueError(f"icon: {icon_path} needs Pillow "
                             "(python3 -m pip install pillow)") from exc
        except OSError as exc:      # Pillow's "cannot identify image file"
            raise ValueError(f"icon: {icon_path}: {exc}") from exc
        obj["icon"] = AD_ICON_NAME   # the board reads what is packed
    elif "icon" in obj:
        raise ValueError(f"icon: {obj['icon']} is not in {folder}")
    body = json.dumps(obj, separators=(",", ":")).encode("ascii")
    if len(body) > AD_JSON_MAX:
        raise ValueError(f"app.json: {len(body)} bytes once written, "
                         f"longer than {AD_JSON_MAX}")
    return body, icon


def write_appdata(folder: str, appdata: tuple) -> None:
    """Leave in `folder` exactly what the board stores after an install.

    app.json, and icon.k64 when the block carries one. Anything else that was
    in the folder goes: a pack is built from it, and a file of an older build
    would travel to every board.
    """
    json_bytes, icon = appdata
    os.makedirs(folder, exist_ok=True)
    for old in os.listdir(folder):
        os.remove(os.path.join(folder, old))
    with open(os.path.join(folder, AD_JSON_NAME), "wb") as fh:
        fh.write(json_bytes)
    if icon:
        with open(os.path.join(folder, AD_ICON_NAME), "wb") as fh:
            fh.write(icon)


def parse(blob: bytes) -> dict:
    """Read a container back: its target, name, app data and image.

    Checks what the board checks before it trusts one: the header's CRC, the
    app-data block's CRC, the lengths, and the SHA-256 of the image once it is
    out of its payload. Raises ValueError naming what is wrong.

    Returns {"target", "name", "appdata", "image"}; `appdata` is
    (json_bytes, icon_bytes) or None.
    """
    if len(blob) < HDR_LEN or blob[:4] != MAGIC:
        raise ValueError("not a kfw container")
    (_, version, hdr_len, target, compression, flags, payload_len, image_len,
     digest) = struct.unpack_from(HDR_FIXED, blob, 0)
    (crc,) = struct.unpack_from("<I", blob, HDR_CRC_OFF)
    if crc != esp_crc32(blob[:HDR_CRC_OFF]):
        raise ValueError("the header's CRC does not match")
    if version != HDR_VERSION or hdr_len != HDR_LEN:
        raise ValueError(f"header version {version}, length {hdr_len}: "
                         f"this tool reads version {HDR_VERSION}, {HDR_LEN} bytes")
    if target not in TARGETS.values():
        raise ValueError(f"unknown target {target}")
    if compression not in COMPRESSION:
        raise ValueError(f"unknown compression {compression}")
    if flags & ~FLAG_APPDATA:
        raise ValueError(f"reserved flags are set: 0x{flags:04x}")
    name = blob[HDR_CRC_OFF + 4:HDR_LEN].split(b"\x00", 1)[0].decode("ascii", "replace")

    at = HDR_LEN
    appdata = None
    if flags & FLAG_APPDATA:
        if target != TARGETS["app"]:
            raise ValueError("only an app container carries app data")
        if len(blob) < at + AD_HDR_LEN:
            raise ValueError("the container ends inside its app-data block")
        ad_magic, json_len, icon_len, ad_crc = struct.unpack_from(AD_HDR, blob, at)
        if ad_magic != AD_MAGIC:
            raise ValueError("the app-data block's magic is wrong")
        if not 0 < json_len <= AD_JSON_MAX or icon_len not in (0, AD_ICON_LEN):
            raise ValueError(f"app-data block: json {json_len} bytes, icon {icon_len}")
        body = blob[at + AD_HDR_LEN:at + AD_HDR_LEN + json_len + icon_len]
        if len(body) != json_len + icon_len:
            raise ValueError("the container ends inside its app-data block")
        if ad_crc != esp_crc32(body):
            raise ValueError("the app-data block's CRC does not match")
        appdata = (body[:json_len], body[json_len:])
        at += AD_HDR_LEN + len(body)

    if len(blob) - at != payload_len:
        raise ValueError(f"the payload is {len(blob) - at} bytes, the header says "
                         f"{payload_len}")
    image = blob[at:]
    if compression == 1:
        try:
            image = zlib.decompress(image)
        except zlib.error as exc:
            raise ValueError(f"the payload does not inflate: {exc}") from exc
    if len(image) != image_len:
        raise ValueError(f"the image is {len(image)} bytes, the header says {image_len}")
    if hashlib.sha256(image).digest() != digest:
        raise ValueError("the image's SHA-256 does not match the header")
    return {"target": target, "name": name, "appdata": appdata, "image": image}


def build(image: bytes, target: int, name: str, compress: bool,
          appdata: tuple = None) -> bytes:
    """Return the container for `image`. Pure, so the test can trust it.

    `appdata` is (json_bytes, icon_bytes_or_empty) and needs target app.
    """
    if appdata is not None and target != TARGETS["app"]:
        raise ValueError("only an app container carries app data")
    flags = 0
    block = b""
    if appdata is not None:
        flags = FLAG_APPDATA
        block = appdata_block(*appdata)
    payload = zlib.compress(image, 9) if compress else image
    head = struct.pack(
        HDR_FIXED,
        MAGIC,
        HDR_VERSION,
        HDR_LEN,
        target,
        1 if compress else 0,
        flags,
        len(payload),
        len(image),
        hashlib.sha256(image).digest(),
    )
    head += struct.pack("<I", esp_crc32(head))
    head += name.encode().ljust(NAME_MAX, b"\x00")
    if len(head) != HDR_LEN:
        raise AssertionError(f"header is {len(head)} bytes, must be {HDR_LEN}")
    return head + block + payload


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--target", required=True, choices=sorted(TARGETS),
                    help="where the board is meant to write this")
    ap.add_argument("--name", default="",
                    help="app name, for --target app only")
    ap.add_argument("--no-compress", action="store_true",
                    help="store the image as-is instead of deflating it")
    ap.add_argument("--appdata", metavar="DIR",
                    help="pack DIR's app.json and icon into the container "
                         "(--target app only)")
    ap.add_argument("--appdata-out", metavar="DIR",
                    help="also leave in DIR the app.json and icon.k64 the "
                         "container carries, as the board stores them "
                         "(needs --appdata)")
    ap.add_argument("input", help="the .bin to wrap")
    ap.add_argument("output", help="the .kfw to write")
    a = ap.parse_args()

    if len(a.name) >= NAME_MAX:
        print(f"a name must be under {NAME_MAX} bytes, "
              f"'{a.name}' is {len(a.name)}", file=sys.stderr)
        return 1
    if a.name and a.target != "app":
        print("only an app image carries a name", file=sys.stderr)
        return 1

    if a.appdata is not None and a.target != "app":
        print("only an app container carries --appdata", file=sys.stderr)
        return 1
    if a.appdata_out is not None and a.appdata is None:
        print("--appdata-out writes what --appdata packs: give both", file=sys.stderr)
        return 1

    appdata = None
    if a.appdata is not None:
        try:
            appdata = load_appdata(a.appdata)
        except ValueError as exc:
            print(exc, file=sys.stderr)
            return 1

    try:
        with open(a.input, "rb") as fh:
            image = fh.read()
    except OSError as exc:
        print(f"cannot read {a.input}: {exc}", file=sys.stderr)
        return 1

    if not image:
        print(f"{a.input} is empty", file=sys.stderr)
        return 1
    if image[0] != ESP_IMAGE_MAGIC:
        print(f"{a.input} starts with 0x{image[0]:02x}, not the ESP image "
              f"magic 0x{ESP_IMAGE_MAGIC:02x} - this is not a firmware image",
              file=sys.stderr)
        return 1

    try:
        blob = build(image, TARGETS[a.target], a.name, not a.no_compress,
                     appdata)
    except ValueError as exc:
        print(exc, file=sys.stderr)
        return 1
    try:
        with open(a.output, "wb") as fh:
            fh.write(blob)
    except OSError as exc:
        print(f"cannot write {a.output}: {exc}", file=sys.stderr)
        return 1
    if a.appdata_out is not None:
        try:
            write_appdata(a.appdata_out, appdata)
        except OSError as exc:
            print(f"cannot write {a.appdata_out}: {exc}", file=sys.stderr)
            return 1

    block_len = 0
    extra = ""
    if appdata is not None:
        block_len = AD_HDR_LEN + len(appdata[0]) + len(appdata[1])
        extra = (f", app data {len(appdata[0])} bytes of app.json + "
                 f"{len(appdata[1])} bytes of icon")
    ratio = (len(blob) - HDR_LEN - block_len) * 100 // len(image)
    print(f"{a.output}: {len(image)} bytes of image -> {len(blob)} bytes "
          f"({ratio}% of the original, target={a.target}{extra})")
    return 0


if __name__ == "__main__":
    sys.exit(main())
