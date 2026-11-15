#ifndef _ffmpeg_profile_compat_h
#define _ffmpeg_profile_compat_h

/*
 * ffmpeg 6.1 added the AV_PROFILE_* names and deprecated FF_PROFILE_*;
 * ffmpeg 7.0 removed FF_PROFILE_* from the public headers.
 * Map the old names onto the new ones so code written with FF_PROFILE_*
 * keeps building, and the #ifdef FF_PROFILE_* checks stay active, on
 * every ffmpeg version.
 */

#include <libavcodec/avcodec.h>

#if !defined(FF_PROFILE_AAC_LOW) && defined(AV_PROFILE_AAC_LOW)
#define FF_PROFILE_AAC_LOW AV_PROFILE_AAC_LOW
#endif
#if !defined(FF_PROFILE_AAC_HE) && defined(AV_PROFILE_AAC_HE)
#define FF_PROFILE_AAC_HE AV_PROFILE_AAC_HE
#endif
#if !defined(FF_PROFILE_AAC_HE_V2) && defined(AV_PROFILE_AAC_HE_V2)
#define FF_PROFILE_AAC_HE_V2 AV_PROFILE_AAC_HE_V2
#endif
#if !defined(FF_PROFILE_AAC_LD) && defined(AV_PROFILE_AAC_LD)
#define FF_PROFILE_AAC_LD AV_PROFILE_AAC_LD
#endif
#if !defined(FF_PROFILE_AAC_ELD) && defined(AV_PROFILE_AAC_ELD)
#define FF_PROFILE_AAC_ELD AV_PROFILE_AAC_ELD
#endif
#if !defined(FF_PROFILE_AAC_USAC) && defined(AV_PROFILE_AAC_USAC)
#define FF_PROFILE_AAC_USAC AV_PROFILE_AAC_USAC
#endif
#if !defined(FF_PROFILE_MPEG2_AAC_LOW) && defined(AV_PROFILE_MPEG2_AAC_LOW)
#define FF_PROFILE_MPEG2_AAC_LOW AV_PROFILE_MPEG2_AAC_LOW
#endif
#if !defined(FF_PROFILE_MPEG2_AAC_HE) && defined(AV_PROFILE_MPEG2_AAC_HE)
#define FF_PROFILE_MPEG2_AAC_HE AV_PROFILE_MPEG2_AAC_HE
#endif
#if !defined(FF_PROFILE_DTS_ES) && defined(AV_PROFILE_DTS_ES)
#define FF_PROFILE_DTS_ES AV_PROFILE_DTS_ES
#endif
#if !defined(FF_PROFILE_DTS_96_24) && defined(AV_PROFILE_DTS_96_24)
#define FF_PROFILE_DTS_96_24 AV_PROFILE_DTS_96_24
#endif
#if !defined(FF_PROFILE_DTS_HD_HRA) && defined(AV_PROFILE_DTS_HD_HRA)
#define FF_PROFILE_DTS_HD_HRA AV_PROFILE_DTS_HD_HRA
#endif
#if !defined(FF_PROFILE_DTS_HD_MA) && defined(AV_PROFILE_DTS_HD_MA)
#define FF_PROFILE_DTS_HD_MA AV_PROFILE_DTS_HD_MA
#endif
#if !defined(FF_PROFILE_DTS_HD_MA_X) && defined(AV_PROFILE_DTS_HD_MA_X)
#define FF_PROFILE_DTS_HD_MA_X AV_PROFILE_DTS_HD_MA_X
#endif
#if !defined(FF_PROFILE_DTS_HD_MA_X_IMAX) && defined(AV_PROFILE_DTS_HD_MA_X_IMAX)
#define FF_PROFILE_DTS_HD_MA_X_IMAX AV_PROFILE_DTS_HD_MA_X_IMAX
#endif
#if !defined(FF_PROFILE_DTS_EXPRESS) && defined(AV_PROFILE_DTS_EXPRESS)
#define FF_PROFILE_DTS_EXPRESS AV_PROFILE_DTS_EXPRESS
#endif
#if !defined(FF_PROFILE_EAC3_DDP_ATMOS) && defined(AV_PROFILE_EAC3_DDP_ATMOS)
#define FF_PROFILE_EAC3_DDP_ATMOS AV_PROFILE_EAC3_DDP_ATMOS
#endif

#endif /* _ffmpeg_profile_compat_h */
