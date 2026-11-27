# OpenBH changes on top of skyjet18/exteplayer3

This tree is [skyjet18/exteplayer3](https://github.com/skyjet18/exteplayer3)
(version 187, commit 31f3edb) with the few additions below. Keep them small
and self-contained so that skyjet updates merge without much work; skyjet's
own documentation is in README.md.

## Build

OE-Alliance builds exteplayer3 with autotools, so `autogen.sh`,
`configure.ac` and `Makefile.am` are kept here (skyjet builds with the
`exteplayer3_git.bb` recipe instead). The sources do not include `config.h`
themselves, so `Makefile.am` passes `-include config.h` to every file.

- `--enable-dreamnextgen` (set by OE-A for the Dreambox One/Two) builds
  `output/dream_video.c` and defines `HAVE_DREAMNEXTGEN`; no ALSA is needed.
- New source files of skyjet must be added to `exteplayer3_SOURCES` in
  `Makefile.am`. Files in `container/` other than `container.c` and
  `container_ffmpeg.c` are included by `container_ffmpeg.c` and are not
  listed.
- skyjet's `ipk/` packages and `.github` workflows are not included; the
  OpenBH build workflow is `.github/workflows/build.yml`.

## Player

- **Live channel count of passthrough audio.** The channels of AC-3,
  E-AC-3 (independent substream 0, as eServiceMP3 counts them) and ADTS AAC
  are read from the frame headers of a track that is not decoded, so a
  change such as 5.1 programme audio to 2.0 in an ad break shows in `c` of
  `a_c` and `a_l`. A change is taken after 3 packets in a row.
  (`audio_live_header_channels()` in `container/container_ffmpeg.c`.)
- **Byte counters for a live bitrate.** The command `BR` answers
  `{"BR":{"v":<bytes>,"a":<bytes>}}`: the compressed bytes of the selected
  video and audio track so far, 32 bit and wrapping, `-1` when there is no
  such track. ServiceApp turns them into the kbit/s of `sVideoBitrate` /
  `sAudioBitrate` for the BitrateInfo converter.
- **AC-4 description.** `d` is `Dolby AC-4` for AC-4 (skyjet: FFmpeg's
  name `ac4`).

ServiceApp reads the description from `d` and falls back to `e` for older
players, which had the description there.
