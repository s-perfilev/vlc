/*****************************************************************************
 * invert.c : Invert video plugin for vlc
 *****************************************************************************
 * Copyright (C) 2000-2006 VLC authors and VideoLAN
 * $Id$
 *
 * Authors: Samuel Hocevar <sam@zoy.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

/*****************************************************************************
 * Preamble
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_plugin.h>

#include <vlc_filter.h>
#include "filter_picture.h"

/*****************************************************************************
 * Local prototypes
 *****************************************************************************/
static int  Create      ( vlc_object_t * );
static void Destroy     ( vlc_object_t * );

static picture_t *Filter( filter_t *, picture_t * );

/*****************************************************************************
 * Module descriptor
 *****************************************************************************/
#define META_TEXT N_("Metadata file")
#define META_LONGTEXT N_("The path to the file that contains metadata.")

#define FILTER_PREFIX "decrypt-"

vlc_module_begin ()
    set_description( N_("Decrypt video filter") )
    set_shortname( N_("Decryptiong" ))
    set_category( CAT_VIDEO )
    set_subcategory( SUBCAT_VIDEO_VFILTER )
    set_capability( "video filter2", 0 )

    add_string( FILTER_PREFIX "meta", "", META_TEXT, META_LONGTEXT, false )

    add_shortcut( "decrypt" )
    set_callbacks( Create, Destroy )
vlc_module_end ()

static const char *const ppsz_filter_options[] = { "meta", NULL };

/*****************************************************************************
 * Create: allocates Invert video thread output method
 *****************************************************************************
 * This function allocates and initializes a Invert vout method.
 *****************************************************************************/
static int Create( vlc_object_t *p_this )
{
    filter_t *p_filter = (filter_t *)p_this;
    vlc_fourcc_t fourcc = p_filter->fmt_in.video.i_chroma;
    char *path_to_meta;

    /* Only YUVA is supported */
    if( fourcc == VLC_CODEC_YUVP || fourcc == VLC_CODEC_RGBP
     || fourcc == VLC_CODEC_RGBA || fourcc == VLC_CODEC_ARGB )
        return VLC_EGENERIC;

    /* Chroma checking */
    const vlc_chroma_description_t *p_chroma =
        vlc_fourcc_GetChromaDescription( fourcc );
    if( p_chroma == NULL || p_chroma->plane_count == 0
     || p_chroma->pixel_size * 8 != p_chroma->pixel_bits )
        return VLC_EGENERIC;

    p_filter->pf_video_filter = Filter;

    /* Parse configs */
    config_ChainParse( p_filter, FILTER_PREFIX, ppsz_filter_options,
                   p_filter->p_cfg );

    path_to_meta = var_CreateGetNonEmptyStringCommand( p_filter,
                                                       FILTER_PREFIX "meta" );

    printf("%s\n", path_to_meta);

    /* Parse file */

    return VLC_SUCCESS;
}

/*****************************************************************************
 * Destroy: destroy Invert video thread output method
 *****************************************************************************
 * Terminate an output method created by InvertCreateOutputMethod
 *****************************************************************************/
static void Destroy( vlc_object_t *p_this )
{
    (void)p_this;
}

/*****************************************************************************
 * Render: displays previously rendered output
 *****************************************************************************
 * This function send the currently rendered image to Invert image, waits
 * until it is displayed and switch the two rendering buffers, preparing next
 * frame.
 *****************************************************************************/
static picture_t *Filter( filter_t *p_filter, picture_t *p_pic )
{
    picture_t *p_outpic;
    int i_planes;

    if( !p_pic ) return NULL;

    p_outpic = filter_NewPicture( p_filter );
    if( !p_outpic )
    {
        msg_Warn( p_filter, "can't get output picture" );
        picture_Release( p_pic );
        return NULL;
    }

    i_planes = p_pic->i_planes;

    for( int i_index = 0 ; i_index < i_planes ; i_index++ )
    {
        uint8_t *p_in, *p_in_end, *p_line_end, *p_out;

        p_in = p_pic->p[i_index].p_pixels;
        p_in_end = p_in + p_pic->p[i_index].i_visible_lines
                           * p_pic->p[i_index].i_pitch;

        p_out = p_outpic->p[i_index].p_pixels;

        while( p_in < p_in_end )
        {
            uint64_t *p_in64, *p_out64;

            p_line_end = p_in + p_pic->p[i_index].i_visible_pitch - 64;

            p_in64 = (uint64_t*)p_in;
            p_out64 = (uint64_t*)p_out;

            while( p_in64 < (uint64_t *)p_line_end )
            {
                /* Do 64 pixels at a time */
                *p_out64++ = *p_in64++; *p_out64++ = *p_in64++;
                *p_out64++ = *p_in64++; *p_out64++ = *p_in64++;
                *p_out64++ = *p_in64++; *p_out64++ = *p_in64++;
                *p_out64++ = *p_in64++; *p_out64++ = *p_in64++;
            }

            p_in = (uint8_t*)p_in64;
            p_out = (uint8_t*)p_out64;
            p_line_end += 64;

            while( p_in < p_line_end )
            {
                *p_out++ = *p_in++;
            }

            p_in += p_pic->p[i_index].i_pitch
                     - p_pic->p[i_index].i_visible_pitch;
            p_out += p_outpic->p[i_index].i_pitch
                     - p_outpic->p[i_index].i_visible_pitch;
        }
    }

    return CopyInfoAndRelease( p_outpic, p_pic );
}
