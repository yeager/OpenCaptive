# Data identity and verification

> Updated for v1.1.82. The scanner persists reusable results for unchanged files and invalidates them when file metadata or content identity changes.

## Principle

OpenCaptive identifies original game data by **SHA-256 of content**, never by
the name of an archive, disk image or member file. Names are unreliable:
different releases, dump tools, repacks and case conventions all vary while
the bytes of a known resource remain stable.

The virtual filesystem (`src/data/data_vfs.c`) accepts directories and ZIP
archives. `vfs_find_sha256()` streams candidate content and returns a matching
buffer only when the requested digest matches. Callers own and free that buffer.

## Read-only Amiga inventory

`amiga_ofs_inventory` is the discovery entry point for an Amiga original
medium. It first locates the ADF by its SHA-256, then emits only each
reconstructed OFS payload's byte length, SHA-256 and container signature. It
does not print or use filesystem member names, so a later decoder or manifest
can be tied to a reproducible content identity rather than to a release's
directory spelling.

```sh
./build/amiga_ofs_inventory /path/to/media <adf-sha256>
```

An inventory is evidence for research, not proof that an unknown payload has a
particular game role. Establish that role independently before adding its hash
to a runtime manifest.

## Captive manifest

The current Captive startup manifest verifies the two boot resources plus all
23 first-person atlas surfaces needed by the active renderer (25 hashes in
total). The CLI verifier, start menu and incremental scanner consume this same
manifest. Examples:

| Purpose | SHA-256 |
| --- | --- |
| Intro animation | `1ec1f90adbcfcb3b99b64a56cf1c669b409b7d3a76bc09cedb056f503bfb1959` |
| Wall texture A | `47ad15b4a593c37880d0306b6a0f51b7a9f20615cf6a188f23716d5b48315524` |
| Object sheet | `21db7daf64cff3b0cae19c3e7eb2057762df9110055e7253175024ecb146fb6b` |
| HUD sheet | `dfca77f0e219962242226f11f9697f580f92e8ad24786296a5b2571b20c2b707` |
| Amiga MapGen recovery module | `bb5a96b9041e98e5b65a36b5645f2bfe0cbecf68c07479d35f7b4f76ed191118` |

The MapGen module also has a pinned unpacked SHA-256,
`b746eb7619a7746eacab2d0a0b2b4b7c42ab34177a22974248d710acf3047ee0`,
and must parse as one 29 304-byte code HUNK. This protects the recovery work
from accepting a same-sized but different payload. It is a verified original
source artifact, not yet a claim of C implementation parity.

Music is loaded the same way. The MIDI system owns each retrieved buffer for
the lifetime of the selected track, preventing a use-after-free during track
changes.

## Liberation CD32 manifest

The verified CD32 data track is:

```text
f807b1385c0996d54ed10afab271a7dd31d2c6dc6a18f13196ad2a79a0af8a80
```

After opening the raw ISO9660 image, the loader verifies these content hashes
inside it:

| Resource role | SHA-256 |
| --- | --- |
| Executable payload | `db61f7e39fd31ac19b82216ea963711728d25518454fae42fd89c5bab52f2215` |
| City generator payload | `e54540c3bf8dfaf569380a135ac039f1438e9efb85cf6d5e3e487e25d4c7c13e` |
| Plot generator payload | `bc9c922801661eb66024d0bcf822c03e38ffea7f3576693e0512692ccf6d6705` |
| Plot text payload | `884d4124fa1ab600a4f7dd889df160779eda8c62e13af1d0280ac9aad681818c` |
| City text payload | `99f7bd75794a7b4f3e94eeef9c61b756da938d862bb83339b140c18d02eb79c5` |
| Dialogue text payload | `e154d250c1acdbed66835bb356a699efdb6f9f8b5e6d586ca07080414610a94c` |

`liberation_data_open()` requires every listed digest. `liberation_data_read()`
then retrieves a resource by its enum-to-hash manifest mapping, rather than by
media filename.

## Data Scanner

The start menu includes an interactive data scanner that lets you verify game
data availability without leaving the application.

### How to use it

Press **D** in the start menu to open the data scanner overlay.

### What it reports

The scanner walks the configured data path and examines every file the VFS can
reach. It reports:

- **ZIP archive count** — how many ZIP containers were found and indexed.
- **Captive files found** (N/25) — the number of verified Captive data files
  out of the 25 required by the manifest.
- **Liberation files found** (N/7) — the number of verified Liberation data
  files out of the 7 required by the manifest.
- **Per-game status** — `[OK]` when all required files are present, `[MISSING]`
  when one or more are absent.

### Verification method

The scanner uses **SHA-256 content hashing**, the same mechanism used at
runtime. It does not rely on filenames, directory structure, or archive names.
A file is considered present only when its content digest matches one of the
digests in the built-in manifest.

### Supported container formats

The scanner looks inside:

- Plain directories
- ZIP archives (including nested ZIPs within ZIPs)
- ADF disk images (Amiga OFS)
- ISO9660 tracks (CD32 data)

### Main menu data status indicators

Even without opening the scanner, the start menu shows data availability at a
glance. Each game card (Captive and Liberation) displays a status indicator:

- **Checkmark** — all required data files for that game are verified and
  present.
- **Cross** — one or more required files are missing or fail verification.

These indicators update when the start menu loads, using the same SHA-256
content-hash check as the full data scanner.

## Adding a digest

1. Obtain media legally and keep it outside the repository.
2. Derive the digest from bytes, record platform/revision/provenance, and test
   a known-good sample.
3. Add the digest to the narrowest manifest that needs it.
4. Add a test or verifier that proves a mismatch is rejected.
5. Do not commit the media, an extracted payload, or a filename-based fallback.
