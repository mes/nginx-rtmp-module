
/*
 * Copyright (C) Roman Arutyunyan
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include "ngx_rtmp_mpegts.h"

#include "ngx_rtmp_codec_module.h"

static u_char ngx_rtmp_mpegts_header[] = {

        /* https://en.wikipedia.org/wiki/MPEG_transport_stream#Packet */

    /* TS */
    0x47,                                               // Sync byte
    0x40, 0x00,                                         // TEI(1) + PUS(1) + TP(1) + PID(13)
    0x10,                                               // SC(2) + AFF(1) + PF(1) + CC(4)
    0x00,
    /* PSI */
    0x00, 0xb0, 0x0d, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PAT */
    0x00, 0x01, 0xef, 0xff,
    /* CRC */
    0x36, 0x90, 0xe2, 0x3d,
    /* stuffing 167 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,

    /* TS */
    0x47,
    0x4f, 0xff,
    0x10,
    0x00,
    /* PSI */
    0x02, 0xb0, /*0x17*/0x3c, 0x00, 0x01, 0xc1, 0x00, 0x00,
    /* PMT */
    0xe1, 0x00,
    0xf0, /*0x00*/0x11,

    /* Descriptor Loop of the PMT for the Program */
    /* @see https://developer.apple.com/library/ios/documentation/AudioVideo/Conceptual/HTTP_Live_Streaming_Metadata_Spec/2/2.html#//apple_ref/doc/uid/TP40010435-CH2-DontLinkElementID_5 */
    0x25, /* tag (37): metadata pointer */
    0x0f, /* length (15) */
    0xff, 0xff,  /*     metadata_application_format */
    0x49, 0x44, 0x33, 0x20,  /* metadata_format_identifier = 'ID3' */
    0xff, /* metadata_format */

    0x49, 0x44, 0x33, 0x20,  /* metadata_format_identifier = 'ID3' */
    0x00, /* metadata_service_id, metadata_locator_record_flag */
    0x1f, /* reserved */
    0x00, 0x01, /* program_number of the program whose es descriptor loop contains the metadata_descriptor */

    0x1b, 0xe1, 0x00, 0xf0, 0x00, /* h264 */
    0x00, 0x00, 0x00, 0x00, 0x00, /* audio placeholder */

    /* elementary stream carrying the metadata */
    0x15, /* stream_type: packetized metadata */
    0xe1, 0x02, /* reserved, elementary_PID. stream id 2: 0 - video, 1 - audio, 2 - metadata */
    0xf0, 0x0f, /* reserved, ES_info_length */

    /* Descriptor Loop of the PMT for the Elementary Stream */
    0x26, /* descriptor_tag (38) */
    0x0d, /* descriptor_length */
    0xff, 0xff, /* metadata_application_format */
    0x49, 0x44, 0x33, 0x20, /* metadata_application_format_identifier = 'ID3' */
    0xff, /* metadata_format */
    0x49, 0x44, 0x33, 0x20, /* metadata_format_identifier = 'ID3' */
    0x00, /* metadata_service_id, decoder_config_flags */
    0x0f, /*  DSM-CC_flag, reserved */

    0x00, 0x00, 0x00, 0x00, /* crc placeholder */

    /* stuffing 157 - 17 - 20 = 120 bytes */
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

static u_char ngx_rtmp_mpegts_header_mp3[] = {
    0x03, 0xe1, 0x01, 0xf0, 0x00 /* mp3 */
};

static u_char ngx_rtmp_mpegts_header_aac[] = {
    0x0f, 0xe1, 0x01, 0xf0, 0x00 /* aac */
};

/* 700 ms PCR delay */
#define NGX_RTMP_HLS_DELAY  63000


static ngx_int_t
ngx_rtmp_mpegts_write_file(ngx_rtmp_mpegts_file_t *file, u_char *in,
    size_t in_size)
{
    u_char   *out;
    size_t    out_size, n;
    ssize_t   rc;

    static u_char  buf[1024];

    if (!file->encrypt) {
        ngx_log_debug1(NGX_LOG_DEBUG_CORE, file->log, 0,
                       "mpegts: write %uz bytes", in_size);

        rc = ngx_write_fd(file->fd, in, in_size);
        if (rc < 0) {
            return NGX_ERROR;
        }

        return NGX_OK;
    }

    /* encrypt */

    ngx_log_debug1(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "mpegts: write %uz encrypted bytes", in_size);

    out = buf;
    out_size = sizeof(buf);

    if (file->size > 0 && file->size + in_size >= 16) {
        ngx_memcpy(file->buf + file->size, in, 16 - file->size);

        in += 16 - file->size;
        in_size -= 16 - file->size;

        AES_cbc_encrypt(file->buf, out, 16, &file->key, file->iv, AES_ENCRYPT);

        out += 16;
        out_size -= 16;

        file->size = 0;
    }

    for ( ;; ) {
        n = in_size & ~0x0f;

        if (n > 0) {
            if (n > out_size) {
                n = out_size;
            }

            AES_cbc_encrypt(in, out, n, &file->key, file->iv, AES_ENCRYPT);

            in += n;
            in_size -= n;

        } else if (out == buf) {
            break;
        }

        rc = ngx_write_fd(file->fd, buf, out - buf + n);
        if (rc < 0) {
            return NGX_ERROR;
        }

        out = buf;
        out_size = sizeof(buf);
    }

    if (in_size) {
        ngx_memcpy(file->buf + file->size, in, in_size);
        file->size += in_size;
    }

    return NGX_OK;
}

// source: https://forum.videolan.org/viewtopic.php?t=10664
static const unsigned int crc_table[256] = {
   0x00000000, 0x04c11db7, 0x09823b6e, 0x0d4326d9, 0x130476dc, 0x17c56b6b,
   0x1a864db2, 0x1e475005, 0x2608edb8, 0x22c9f00f, 0x2f8ad6d6, 0x2b4bcb61,
   0x350c9b64, 0x31cd86d3, 0x3c8ea00a, 0x384fbdbd, 0x4c11db70, 0x48d0c6c7,
   0x4593e01e, 0x4152fda9, 0x5f15adac, 0x5bd4b01b, 0x569796c2, 0x52568b75,
   0x6a1936c8, 0x6ed82b7f, 0x639b0da6, 0x675a1011, 0x791d4014, 0x7ddc5da3,
   0x709f7b7a, 0x745e66cd, 0x9823b6e0, 0x9ce2ab57, 0x91a18d8e, 0x95609039,
   0x8b27c03c, 0x8fe6dd8b, 0x82a5fb52, 0x8664e6e5, 0xbe2b5b58, 0xbaea46ef,
   0xb7a96036, 0xb3687d81, 0xad2f2d84, 0xa9ee3033, 0xa4ad16ea, 0xa06c0b5d,
   0xd4326d90, 0xd0f37027, 0xddb056fe, 0xd9714b49, 0xc7361b4c, 0xc3f706fb,
   0xceb42022, 0xca753d95, 0xf23a8028, 0xf6fb9d9f, 0xfbb8bb46, 0xff79a6f1,
   0xe13ef6f4, 0xe5ffeb43, 0xe8bccd9a, 0xec7dd02d, 0x34867077, 0x30476dc0,
   0x3d044b19, 0x39c556ae, 0x278206ab, 0x23431b1c, 0x2e003dc5, 0x2ac12072,
   0x128e9dcf, 0x164f8078, 0x1b0ca6a1, 0x1fcdbb16, 0x018aeb13, 0x054bf6a4,
   0x0808d07d, 0x0cc9cdca, 0x7897ab07, 0x7c56b6b0, 0x71159069, 0x75d48dde,
   0x6b93dddb, 0x6f52c06c, 0x6211e6b5, 0x66d0fb02, 0x5e9f46bf, 0x5a5e5b08,
   0x571d7dd1, 0x53dc6066, 0x4d9b3063, 0x495a2dd4, 0x44190b0d, 0x40d816ba,
   0xaca5c697, 0xa864db20, 0xa527fdf9, 0xa1e6e04e, 0xbfa1b04b, 0xbb60adfc,
   0xb6238b25, 0xb2e29692, 0x8aad2b2f, 0x8e6c3698, 0x832f1041, 0x87ee0df6,
   0x99a95df3, 0x9d684044, 0x902b669d, 0x94ea7b2a, 0xe0b41de7, 0xe4750050,
   0xe9362689, 0xedf73b3e, 0xf3b06b3b, 0xf771768c, 0xfa325055, 0xfef34de2,
   0xc6bcf05f, 0xc27dede8, 0xcf3ecb31, 0xcbffd686, 0xd5b88683, 0xd1799b34,
   0xdc3abded, 0xd8fba05a, 0x690ce0ee, 0x6dcdfd59, 0x608edb80, 0x644fc637,
   0x7a089632, 0x7ec98b85, 0x738aad5c, 0x774bb0eb, 0x4f040d56, 0x4bc510e1,
   0x46863638, 0x42472b8f, 0x5c007b8a, 0x58c1663d, 0x558240e4, 0x51435d53,
   0x251d3b9e, 0x21dc2629, 0x2c9f00f0, 0x285e1d47, 0x36194d42, 0x32d850f5,
   0x3f9b762c, 0x3b5a6b9b, 0x0315d626, 0x07d4cb91, 0x0a97ed48, 0x0e56f0ff,
   0x1011a0fa, 0x14d0bd4d, 0x19939b94, 0x1d528623, 0xf12f560e, 0xf5ee4bb9,
   0xf8ad6d60, 0xfc6c70d7, 0xe22b20d2, 0xe6ea3d65, 0xeba91bbc, 0xef68060b,
   0xd727bbb6, 0xd3e6a601, 0xdea580d8, 0xda649d6f, 0xc423cd6a, 0xc0e2d0dd,
   0xcda1f604, 0xc960ebb3, 0xbd3e8d7e, 0xb9ff90c9, 0xb4bcb610, 0xb07daba7,
   0xae3afba2, 0xaafbe615, 0xa7b8c0cc, 0xa379dd7b, 0x9b3660c6, 0x9ff77d71,
   0x92b45ba8, 0x9675461f, 0x8832161a, 0x8cf30bad, 0x81b02d74, 0x857130c3,
   0x5d8a9099, 0x594b8d2e, 0x5408abf7, 0x50c9b640, 0x4e8ee645, 0x4a4ffbf2,
   0x470cdd2b, 0x43cdc09c, 0x7b827d21, 0x7f436096, 0x7200464f, 0x76c15bf8,
   0x68860bfd, 0x6c47164a, 0x61043093, 0x65c52d24, 0x119b4be9, 0x155a565e,
   0x18197087, 0x1cd86d30, 0x029f3d35, 0x065e2082, 0x0b1d065b, 0x0fdc1bec,
   0x3793a651, 0x3352bbe6, 0x3e119d3f, 0x3ad08088, 0x2497d08d, 0x2056cd3a,
   0x2d15ebe3, 0x29d4f654, 0xc5a92679, 0xc1683bce, 0xcc2b1d17, 0xc8ea00a0,
   0xd6ad50a5, 0xd26c4d12, 0xdf2f6bcb, 0xdbee767c, 0xe3a1cbc1, 0xe760d676,
   0xea23f0af, 0xeee2ed18, 0xf0a5bd1d, 0xf464a0aa, 0xf9278673, 0xfde69bc4,
   0x89b8fd09, 0x8d79e0be, 0x803ac667, 0x84fbdbd0, 0x9abc8bd5, 0x9e7d9662,
   0x933eb0bb, 0x97ffad0c, 0xafb010b1, 0xab710d06, 0xa6322bdf, 0xa2f33668,
   0xbcb4666d, 0xb8757bda, 0xb5365d03, 0xb1f740b4
};


unsigned int chksum_crc32(const unsigned char *data, int len)
{
    register int i;
    unsigned int crc = 0xffffffff;

    for (i=0; i<len; i++) {
        crc = (crc << 8) ^ crc_table[((crc >> 24) ^ *data++) & 0xff];
    }

    return crc;
}

static ngx_int_t
ngx_rtmp_mpegts_write_header(ngx_rtmp_mpegts_file_t *file, ngx_uint_t *audio_codec_id, ngx_uint_t mpegts_cc)
{

    if (*audio_codec_id == NGX_RTMP_AUDIO_MP3) {
        ngx_memcpy(ngx_rtmp_mpegts_header + 210 + 17, ngx_rtmp_mpegts_header_mp3,
                                        sizeof(ngx_rtmp_mpegts_header_mp3));
    }
    //if (*audio_codec_id == NGX_RTMP_AUDIO_AAC) {
    // default to AAC
    else {
        ngx_memcpy(ngx_rtmp_mpegts_header + 210 + 17, ngx_rtmp_mpegts_header_aac,
                                        sizeof(ngx_rtmp_mpegts_header_aac));
    }

    // Truncate counter to 4 bits here
    mpegts_cc %= 0x0f;
    // And fill headers
    ngx_rtmp_mpegts_header[3] = (ngx_rtmp_mpegts_header[3] & 0xf0) + (u_char)mpegts_cc;
    ngx_rtmp_mpegts_header[191] = (ngx_rtmp_mpegts_header[191] & 0xf0) + (u_char)mpegts_cc;

    // generate and write crc32 of the second TS packet
    // start with byte 5 after TS header; end just before crc field
    unsigned int checksum = chksum_crc32(&ngx_rtmp_mpegts_header[188 + 5], (210 + 17 + 25) - (188 + 5));

    // copy as big endian
    ngx_rtmp_rmemcpy(ngx_rtmp_mpegts_header + 210 + 17 + 25, &checksum, 4);

    ngx_log_debug2(NGX_LOG_DEBUG_CORE, file->log, 0,
                  "checksum start byte: %d; end: %d", ngx_rtmp_mpegts_header[188 + 5], ngx_rtmp_mpegts_header[210 + 17 + 25 - 1]);

    ngx_log_debug4(NGX_LOG_DEBUG_CORE, file->log, 0,
                  "checksum out: %d %d %d %d", ngx_rtmp_mpegts_header[210 + 17 + 25], ngx_rtmp_mpegts_header[210 + 17 + 25 + 1], ngx_rtmp_mpegts_header[210 + 17 + 25 + 2], ngx_rtmp_mpegts_header[210 + 17 + 25 + 3]);

    return ngx_rtmp_mpegts_write_file(file, ngx_rtmp_mpegts_header,
                                      sizeof(ngx_rtmp_mpegts_header));
}


static u_char *
ngx_rtmp_mpegts_write_pcr(u_char *p, uint64_t pcr)
{
    *p++ = (u_char) (pcr >> 25);
    *p++ = (u_char) (pcr >> 17);
    *p++ = (u_char) (pcr >> 9);
    *p++ = (u_char) (pcr >> 1);
    *p++ = (u_char) (pcr << 7 | 0x7e);
    *p++ = 0;

    return p;
}


static u_char *
ngx_rtmp_mpegts_write_pts(u_char *p, ngx_uint_t fb, uint64_t pts)
{
    ngx_uint_t val;

    val = fb << 4 | (((pts >> 30) & 0x07) << 1) | 1;
    *p++ = (u_char) val;

    val = (((pts >> 15) & 0x7fff) << 1) | 1;
    *p++ = (u_char) (val >> 8);
    *p++ = (u_char) val;

    val = (((pts) & 0x7fff) << 1) | 1;
    *p++ = (u_char) (val >> 8);
    *p++ = (u_char) val;

    return p;
}


ngx_int_t
ngx_rtmp_mpegts_write_frame(ngx_rtmp_mpegts_file_t *file,
    ngx_rtmp_mpegts_frame_t *f, ngx_buf_t *b)
{
    ngx_uint_t  pes_size, header_size, body_size, in_size, stuff_size, flags;
    u_char      packet[188], *p, *base;
    ngx_int_t   first, rc;

    ngx_log_debug6(NGX_LOG_DEBUG_CORE, file->log, 0,
                   "mpegts: pid=%ui, sid=%ui, pts=%uL, "
                   "dts=%uL, key=%ui, size=%ui",
                   f->pid, f->sid, f->pts, f->dts,
                   (ngx_uint_t) f->key, (size_t) (b->last - b->pos));

    first = 1;

    while (b->pos < b->last) {
        p = packet;

        f->cc++;

        *p++ = 0x47;
        *p++ = (u_char) (f->pid >> 8);

        if (first) {
            p[-1] |= 0x40;
        }

        *p++ = (u_char) f->pid;
        *p++ = 0x10 | (f->cc & 0x0f); /* payload */

        if (first) {

            packet[3] |= 0x20; /* adaptation */

            *p++ = 7;    /* size */
            *p++ = 0x50; /* random access + PCR */

            p = ngx_rtmp_mpegts_write_pcr(p, f->dts - NGX_RTMP_HLS_DELAY);

            /* PES header */

            *p++ = 0x00;
            *p++ = 0x00;
            *p++ = 0x01;
            *p++ = (u_char) f->sid;

            header_size = 5;
            flags = 0x80; /* PTS */

            if (f->dts != f->pts) {
                header_size += 5;
                flags |= 0x40; /* DTS */
            }

            pes_size = (b->last - b->pos) + header_size + 3;
            if (pes_size > 0xffff) {
                pes_size = 0;
            }

            *p++ = (u_char) (pes_size >> 8);
            *p++ = (u_char) pes_size;
            *p++ = 0x80; /* H222 */
            *p++ = (u_char) flags;
            *p++ = (u_char) header_size;

            p = ngx_rtmp_mpegts_write_pts(p, flags >> 6, f->pts +
                                                         NGX_RTMP_HLS_DELAY);

            if (f->dts != f->pts) {
                p = ngx_rtmp_mpegts_write_pts(p, 1, f->dts +
                                                    NGX_RTMP_HLS_DELAY);
            }

            first = 0;
        }

        body_size = (ngx_uint_t) (packet + sizeof(packet) - p);
        in_size = (ngx_uint_t) (b->last - b->pos);

        if (body_size <= in_size) {
            ngx_memcpy(p, b->pos, body_size);
            b->pos += body_size;

        } else {
            stuff_size = (body_size - in_size);

            if (packet[3] & 0x20) {

                /* has adaptation */

                base = &packet[5] + packet[4];
                p = ngx_movemem(base + stuff_size, base, p - base);
                ngx_memset(base, 0xff, stuff_size);
                packet[4] += (u_char) stuff_size;

            } else {

                /* no adaptation */

                packet[3] |= 0x20;
                p = ngx_movemem(&packet[4] + stuff_size, &packet[4],
                                p - &packet[4]);

                packet[4] = (u_char) (stuff_size - 1);
                if (stuff_size >= 2) {
                    packet[5] = 0;
                    ngx_memset(&packet[6], 0xff, stuff_size - 2);
                }
            }

            ngx_memcpy(p, b->pos, in_size);
            b->pos = b->last;
        }

        rc = ngx_rtmp_mpegts_write_file(file, packet, sizeof(packet));
        if (rc != NGX_OK) {
            return rc;
        }
    }

    return NGX_OK;
}


ngx_int_t
ngx_rtmp_mpegts_init_encryption(ngx_rtmp_mpegts_file_t *file,
    u_char *key, size_t key_len, uint64_t iv)
{
    if (AES_set_encrypt_key(key, key_len * 8, &file->key)) {
        return NGX_ERROR;
    }

    ngx_memzero(file->iv, 8);

    file->iv[8]  = (u_char) (iv >> 56);
    file->iv[9]  = (u_char) (iv >> 48);
    file->iv[10] = (u_char) (iv >> 40);
    file->iv[11] = (u_char) (iv >> 32);
    file->iv[12] = (u_char) (iv >> 24);
    file->iv[13] = (u_char) (iv >> 16);
    file->iv[14] = (u_char) (iv >> 8);
    file->iv[15] = (u_char) (iv);

    file->encrypt = 1;

    return NGX_OK;
}


ngx_int_t
ngx_rtmp_mpegts_open_file(ngx_rtmp_mpegts_file_t *file, u_char *path,
    ngx_log_t *log, ngx_uint_t *audio_codec_id, ngx_uint_t mpegts_cc)
{
    file->log = log;

    file->fd = ngx_open_file(path, NGX_FILE_WRONLY, NGX_FILE_TRUNCATE,
                             NGX_FILE_DEFAULT_ACCESS);

    if (file->fd == NGX_INVALID_FILE) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_errno,
                      "hls: error creating fragment file");
        return NGX_ERROR;
    }

    file->size = 0;

    if (ngx_rtmp_mpegts_write_header(file, audio_codec_id, mpegts_cc) != NGX_OK) {
        ngx_log_error(NGX_LOG_ERR, log, ngx_errno,
                      "hls: error writing fragment header");
        ngx_close_file(file->fd);
        return NGX_ERROR;
    }

    return NGX_OK;
}


ngx_int_t
ngx_rtmp_mpegts_close_file(ngx_rtmp_mpegts_file_t *file)
{
    u_char   buf[16];
    ssize_t  rc;

    if (file->encrypt) {
        ngx_memset(file->buf + file->size, 16 - file->size, 16 - file->size);

        AES_cbc_encrypt(file->buf, buf, 16, &file->key, file->iv, AES_ENCRYPT);

        rc = ngx_write_fd(file->fd, buf, 16);
        if (rc < 0) {
            return NGX_ERROR;
        }
    }

    ngx_close_file(file->fd);

    return NGX_OK;
}
