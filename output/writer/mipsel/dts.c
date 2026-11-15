/*
 * linuxdvb output/writer handling.
 *
 * konfetti 2010 based on linuxdvb.c code from libeplayer2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

/* ***************************** */
/* Includes                      */
/* ***************************** */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <linux/dvb/video.h>
#include <linux/dvb/audio.h>
#include <memory.h>
#include <asm/types.h>
#include <pthread.h>
#include <errno.h>

#include "stm_ioctls.h"
#include "bcm_ioctls.h"

#include "debug.h"
#include "common.h"
#include "output.h"
#include "debug.h"
#include "misc.h"
#include "pes.h"
#include "writer.h"

/* ***************************** */
/* Makros/Constants              */
/* ***************************** */
#define PES_AUDIO_PRIVATE_HEADER_SIZE   16                                // consider maximum private header size.
#define PES_AUDIO_HEADER_SIZE           (32 + PES_AUDIO_PRIVATE_HEADER_SIZE)
#define PES_AUDIO_PACKET_SIZE           2028
#define SPDIF_AUDIO_PACKET_SIZE         (1024 * sizeof(unsigned int) * 2) // stereo 32bit samples.

/* ***************************** */
/* Types                         */
/* ***************************** */

/* ***************************** */
/* Varaibles                     */
/* ***************************** */

/* ***************************** */
/* Prototypes                    */
/* ***************************** */

/* ***************************** */
/* MISC Functions                */
/* ***************************** */
static int32_t reset()
{
    return 0;
}

/* --- DTS core frame 4-byte alignment -------------------------------------
 * Several Broadcom and HiSilicon drivers only lock onto DTS core frames whose
 * size is a multiple of 4 bytes.  DVD full-rate DTS uses 2013-byte frames and
 * is then silent (Vu+/GigaBlue) or noisy (SF8008).  Pad such a lone core
 * frame with zeros and raise its FSIZE (and the X96/XCH extension size field,
 * if present); the decoded audio is identical.  A frame that is not a lone
 * core frame (core + DTS-HD EXSS, or already aligned) is left untouched.
 * This mirrors the dvbmediasink fix. */
static size_t dts_core_aligned_size(const uint8_t *in, size_t size)
{
    size_t frame_size, samples, padded;
    if (!in || size < 16 || !(size & 3))
        return 0;
    if (in[0] != 0x7f || in[1] != 0xfe || in[2] != 0x80 || in[3] != 0x01)
        return 0;
    if (in[4] & 0x02) /* CPF: a header CRC would no longer match */
        return 0;
    samples = ((((size_t)in[4] & 0x01) << 6 | in[5] >> 2) + 1) * 32;
    frame_size = (((size_t)in[5] & 0x03) << 12 | (size_t)in[6] << 4 | in[7] >> 4) + 1;
    if (frame_size != size)
        return 0;
    padded = (size + 3) & ~(size_t)3;
    if (padded > 16384 || padded + 8 > samples * 4)
        return 0;
    return padded;
}

static void dts_core_align(uint8_t *out, size_t padded, const uint8_t *in, size_t size)
{
    size_t field = padded - 1, pos;
    memcpy(out, in, size);
    memset(out + size, 0, padded - size);
    out[5] = (uint8_t)((out[5] & 0xfc) | ((field >> 12) & 0x03));
    out[6] = (uint8_t)(field >> 4);
    out[7] = (uint8_t)((out[7] & 0x0f) | ((field & 0x0f) << 4));
    if (in[10] & 0x10) /* EXT_AUDIO: a core extension (X96 / XCH) follows */
    {
        const unsigned int id = in[10] >> 5; /* 0 XCH, 2 X96 */
        for (pos = 16; pos + 8 <= size; pos += 4)
        {
            uint32_t sync = (uint32_t)in[pos] << 24 | (uint32_t)in[pos+1] << 16 |
                            (uint32_t)in[pos+2] << 8 | in[pos+3];
            if (id == 2 && sync == 0x1d95f262u)
            {
                if ((((size_t)in[pos+4] << 4) | (in[pos+5] >> 4)) + 1 != size - pos) continue;
                field = padded - pos - 1;
                if (field > 0xfff) break;
                out[pos+4] = (uint8_t)(field >> 4);
                out[pos+5] = (uint8_t)((out[pos+5] & 0x0f) | ((field & 0x0f) << 4));
                break;
            }
            if (id == 0 && sync == 0x5a5a5a5au)
            {
                if ((((size_t)in[pos+4] << 2) | (in[pos+5] >> 6)) + 1 != size - pos) continue;
                field = padded - pos - 1;
                if (field > 0x3ff) break;
                out[pos+4] = (uint8_t)(field >> 2);
                out[pos+5] = (uint8_t)((out[pos+5] & 0x3f) | ((field & 0x03) << 6));
                break;
            }
        }
    }
}

static int32_t writeData(void* _call)
{
    WriterAVCallData_t* call = (WriterAVCallData_t*) _call;

    uint8_t PesHeader[PES_AUDIO_HEADER_SIZE];

    dts_printf(10, "\n");

    if (call == NULL)
    {
        dts_err("call data is NULL...\n");
        return 0;
    }

    dts_printf(10, "AudioPts %lld\n", call->Pts);

    if ((call->data == NULL) || (call->len <= 0))
    {
        dts_err("parsing NULL Data. ignoring...\n");
        return 0;
    }

    if (call->fd < 0)
    {
        dts_err("file pointer < 0. ignoring ...\n");
        return 0;
    }

    uint8_t *Data = call->data;
    int32_t Size = call->len;

#ifdef CHECK_FOR_DTS_HD
    int32_t pos = 0;
    while ((pos + 4) <= Size)
    {
        // check for DTS-HD
        if (!strcmp((char*)(Data + pos), "\x64\x58\x20\x25"))
        {
            Size = pos;
            break;
        }
        ++pos;
    }
#endif

// #define DO_BYTESWAP
#ifdef DO_BYTESWAP
    /* 16-bit byte swap all data before injecting it */
    for (i=0; i< Size; i+=2)
    {
        uint8_t Tmp = Data[i];
        Data[i] = Data[i+1];
        Data[i+1] = Tmp;
    }
#endif

    /* pad a lone DTS core frame to a 4-byte boundary (see above) */
    static uint8_t dtsPadBuf[16384 + 8];
    size_t dtsPadded = dts_core_aligned_size(Data, (size_t)Size);
    if (dtsPadded && dtsPadded <= sizeof(dtsPadBuf))
    {
        dts_core_align(dtsPadBuf, dtsPadded, Data, (size_t)Size);
        Data = dtsPadBuf;
        Size = (int32_t)dtsPadded;
    }

    struct iovec iov[2];
    iov[0].iov_base = PesHeader;
    iov[0].iov_len = InsertPesHeader (PesHeader, Size, MPEG_AUDIO_PES_START_CODE, call->Pts, 0);
    iov[1].iov_base = Data;
    iov[1].iov_len = Size;

    int32_t len = call->WriteV(call->fd, iov, 2);
    dts_printf(10, "< len %d\n", len);
    return len;
}

/* ***************************** */
/* Writer  Definition            */
/* ***************************** */

static WriterCaps_t caps = {
    "dts",
    eAudio,
    "A_DTS",
    AUDIO_ENCODING_DTS,
    AUDIOTYPE_DTS,
    -1
};

struct Writer_s WriterAudioDTS = {
    &reset,
    &writeData,
    NULL,
    &caps
};

/* DTS-HD MA / DTS:X / DTS-HD HRA / DTS Express: the whole frame (core + DTS-HD
 * EXSS) is passed through, and the driver is put in its DTS-HD bypass mode
 * (AUDIOTYPE_DTS_HD) by LinuxDvbMapBypassMode, which falls back to plain DTS
 * where the driver has no DTS-HD mode. Same writeData (padding only touches
 * lone core frames, so a core+EXSS frame is written untouched). */
static WriterCaps_t caps_dts_hd = {
    "dts-hd",
    eAudio,
    "A_DTS_HD",
    AUDIO_ENCODING_DTS,
    AUDIOTYPE_DTS_HD,
    -1
};

struct Writer_s WriterAudioDTSHD = {
    &reset,
    &writeData,
    NULL,
    &caps_dts_hd
};
