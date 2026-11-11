# apps_image

This folder holds the welcome app, the one app a Kode Dot leaves the factory with: an app in a private repository of Kode, the folder `apps/bienvenida`, project `onboarding`. `sdkconfig.defaults` names it in `CONFIG_KODEOS_WELCOME_APP`.

`welcome.pin` is the one file here in git: one line, `<repository> <app folder> <commit>`. `tools/build_welcome.sh` clones that repository into `build_welcome/src`, builds the folder at that commit and writes two files here: `<project>.bin`, the app's image, and its stamp `<project>.commit` ("<commit> <sha256 of the image> <digest of the data folder>", `-dirty` on local changes). It also turns the `appdata/` folder the app carries (its `app.json` and icon) into the folder the board stores, `storage_image/.kode/appsdata/<project>/` with `app.json` and `icon.k64`, so run it before `idf.py build`: the build makes the FAT image. Moving the pin is a deliberate change, like moving the C5's.

An app's metadata travels apart from its image, so the welcome app reaches a board in two pieces. `tools/assemble_release.sh` builds the factory pack's `apps.bin` from the image with `tools/mkapps.py`: the apps region's directory with the app installed in it, the image at the first page of data, and no boot request. `flash.sh` writes it at the region's base. The FAT image carries the data folder, where Apps reads the app's icon, colour and description. kodeOS boots on both and lists the app in Apps. The factory reset that the final station runs on a PASS keeps the app and writes its boot request right before ship mode, so the customer's first power-on runs it.

The release also publishes the two pieces, to put the app back on a board that lost it: `<project>-v<version>.bin` and `<project>-v<version>-appdata.zip`, which holds the folder `<project>/`. Unzip it, then:

```bash
python3 tools/kcli.py -p PORT appdata put <project> <project>/
python3 tools/kcli.py -p PORT install <project>-v<version>.bin --dst app
```

The release refuses an image that is missing, has no stamp, was built from another commit than the pin, is not the stamped one, or carries another project name than `CONFIG_KODEOS_WELCOME_APP`. It also refuses a FAT image whose data folder for the app is missing or is not the stamped one.

**The image and its stamp are not in git.** `.gitignore` excludes `*.bin` and `*.commit` in this folder. A release runs `tools/build_welcome.sh`, which clones the pinned commit over SSH, so it needs a key that reads that repository: only Kode's does. A fork points `welcome.pin` at a welcome app of its own.