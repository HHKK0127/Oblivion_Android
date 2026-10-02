# Bundled Game Data (APK-internal)

Place your original Oblivion game data files here to bundle them into the APK:

- `Oblivion.esm`
- `Oblivion - Meshes.bsa`
- other `.bsa` / `.esm` files you want embedded

At app startup (with "APK Bundled Data" selected in the debug panel),
these files are copied from `assets/data/` into the app's private
`filesDir/data` directory, where the native engine loads them.

Notes:

- `.bsa` / `.esm` files here are excluded from git (BYO-data model).
  This README keeps the folder present in the repository.
- Large archives (several GB) are better kept out of the APK: use the
  "Steam Data (Copied)" source and the SAF folder picker instead.
- See `docs/STEAM_DATA_TRANSFER.md` for the full transfer guide.
