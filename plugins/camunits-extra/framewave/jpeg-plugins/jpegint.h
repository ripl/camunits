/*
 * jpegint.h
 *
 * Copyright (C) 1991-1997, Thomas G. Lane.
 * This file is part of the Independent JPEG Group's software.
 * For conditions of distribution and use, see the accompanying README file.
 *
 * This file provides common declarations for the various JPEG modules.
 * These declarations are considered internal to the JPEG library; most
 * applications using the library shouldn't need to include this file.
 */



#ifdef USE_FW
/* Declarations for Intel Performance Primitives */
//#include "fws.h"
//#include "fwi.h"
#include "fwJPEG.h"
#endif


/* Declarations for both compression & decompression */

typedef enum {      /* Operating modes for buffer controllers */
  JBUF_PASS_THRU,   /* Plain stripwise operation */
  /* Remaining modes require a full-image buffer to have been created */
  JBUF_SAVE_SOURCE, /* Run source subobject only, save output */
  JBUF_CRANK_DEST,  /* Run dest subobject only, using saved data */
  JBUF_SAVE_AND_PASS  /* Run both subobjects, save output */
} J_BUF_MODE;

/* Values of global_state field (jdapi.c has some dependencies on ordering!) */
#define CSTATE_START  100 /* after create_compress */
#define CSTATE_SCANNING 101 /* start_compress done, write_scanlines OK */
#define CSTATE_RAW_OK 102 /* start_compress done, write_raw_data OK */
#define CSTATE_WRCOEFS  103 /* jpegfw_write_coefficients done */
#define DSTATE_START  200 /* after create_decompress */
#define DSTATE_INHEADER 201 /* reading header markers, no SOS yet */
#define DSTATE_READY  202 /* found SOS, ready for start_decompress */
#define DSTATE_PRELOAD  203 /* reading multiscan file in start_decompress*/
#define DSTATE_PRESCAN  204 /* performing dummy pass for 2-pass quant */
#define DSTATE_SCANNING 205 /* start_decompress done, read_scanlines OK */
#define DSTATE_RAW_OK 206 /* start_decompress done, read_raw_data OK */
#define DSTATE_BUFIMAGE 207 /* expecting jpegfw_start_output */
#define DSTATE_BUFPOST  208 /* looking for SOS/EOI in jpegfw_finish_output */
#define DSTATE_RDCOEFS  209 /* reading file in jpegfw_read_coefficients */
#define DSTATE_STOPPING 210 /* looking for EOI in jpegfw_finish_decompress */


/* Declarations for compression modules */

/* Master control module */
struct jpegfw_comp_master {
  JMETHOD(void, prepare_for_pass, (j_compress_ptr cinfo));
  JMETHOD(void, pass_startup, (j_compress_ptr cinfo));
  JMETHOD(void, finish_pass, (j_compress_ptr cinfo));

  /* State variables made visible to other modules */
  boolean call_pass_startup;  /* True if pass_startup must be called */
  boolean is_last_pass;   /* True during last pass */
};

/* Main buffer control (downsampled-data buffer) */
struct jpegfw_c_main_controller {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, process_data, (j_compress_ptr cinfo,
             JSAMPARRAY input_buf, JDIMENSION *in_row_ctr,
             JDIMENSION in_rows_avail));
};

/* Compression preprocessing (downsampling input buffer control) */
struct jpegfw_c_prep_controller {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, pre_process_data, (j_compress_ptr cinfo,
           JSAMPARRAY input_buf,
           JDIMENSION *in_row_ctr,
           JDIMENSION in_rows_avail,
           JSAMPIMAGE output_buf,
           JDIMENSION *out_row_group_ctr,
           JDIMENSION out_row_groups_avail));
};

/* Coefficient buffer control */
struct jpegfw_c_coef_controller {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo, J_BUF_MODE pass_mode));
  JMETHOD(boolean, compress_data, (j_compress_ptr cinfo,
           JSAMPIMAGE input_buf));
};

/* Colorspace conversion */
struct jpegfw_color_converter {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo));
  JMETHOD(void, color_convert, (j_compress_ptr cinfo,
        JSAMPARRAY input_buf, JSAMPIMAGE output_buf,
        JDIMENSION output_row, int num_rows));
};

/* Downsampling */
struct jpegfw_downsampler {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo));
  JMETHOD(void, downsample, (j_compress_ptr cinfo,
           JSAMPIMAGE input_buf, JDIMENSION in_row_index,
           JSAMPIMAGE output_buf,
           JDIMENSION out_row_group_index));

  boolean need_context_rows;  /* TRUE if need rows above & below */
};

/* Forward DCT (also controls coefficient quantization) */
struct jpegfw_forward_dct {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo));
  /* perhaps this should be an array??? */
  JMETHOD(void, forward_DCT, (j_compress_ptr cinfo,
            jpegfw_component_info * compptr,
            JSAMPARRAY sample_data, JBLOCKROW coef_blocks,
            JDIMENSION start_row, JDIMENSION start_col,
            JDIMENSION num_blocks));
};

/* Entropy encoding */
struct jpegfw_entropy_encoder {
  JMETHOD(void, start_pass, (j_compress_ptr cinfo, boolean gather_statistics));
  JMETHOD(boolean, encode_mcu, (j_compress_ptr cinfo, JBLOCKROW *MCU_data));
  JMETHOD(void, finish_pass, (j_compress_ptr cinfo));
};

/* Marker writing */
struct jpegfw_marker_writer {
  JMETHOD(void, write_file_header, (j_compress_ptr cinfo));
  JMETHOD(void, write_frame_header, (j_compress_ptr cinfo));
  JMETHOD(void, write_scan_header, (j_compress_ptr cinfo));
  JMETHOD(void, write_file_trailer, (j_compress_ptr cinfo));
  JMETHOD(void, write_tables_only, (j_compress_ptr cinfo));
  /* These routines are exported to allow insertion of extra markers */
  /* Probably only COM and APPn markers should be written this way */
  JMETHOD(void, write_marker_header, (j_compress_ptr cinfo, int marker,
              unsigned int datalen));
  JMETHOD(void, write_marker_byte, (j_compress_ptr cinfo, int val));
};


/* Declarations for decompression modules */

/* Master control module */
struct jpegfw_decomp_master {
  JMETHOD(void, prepare_for_output_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, finish_output_pass, (j_decompress_ptr cinfo));

  /* State variables made visible to other modules */
  boolean is_dummy_pass;  /* True during 1st pass for 2-pass quant */
};

/* Input control module */
struct jpegfw_input_controller {
  JMETHOD(int, consume_input, (j_decompress_ptr cinfo));
  JMETHOD(void, reset_input_controller, (j_decompress_ptr cinfo));
  JMETHOD(void, start_input_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, finish_input_pass, (j_decompress_ptr cinfo));

  /* State variables made visible to other modules */
  boolean has_multiple_scans; /* True if file has multiple scans */
  boolean eoi_reached;    /* True when EOI has been consumed */
};

/* Main buffer control (downsampled-data buffer) */
struct jpegfw_d_main_controller {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, process_data, (j_decompress_ptr cinfo,
             JSAMPARRAY output_buf, JDIMENSION *out_row_ctr,
             JDIMENSION out_rows_avail));
};

/* Coefficient buffer control */
struct jpegfw_d_coef_controller {
  JMETHOD(void, start_input_pass, (j_decompress_ptr cinfo));
  JMETHOD(int, consume_data, (j_decompress_ptr cinfo));
  JMETHOD(void, start_output_pass, (j_decompress_ptr cinfo));
  JMETHOD(int, decompress_data, (j_decompress_ptr cinfo,
         JSAMPIMAGE output_buf));
  /* Pointer to array of coefficient virtual arrays, or NULL if none */
  jvirt_barray_ptr *coef_arrays;
};

/* Decompression postprocessing (color quantization buffer control) */
struct jpegfw_d_post_controller {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo, J_BUF_MODE pass_mode));
  JMETHOD(void, post_process_data, (j_decompress_ptr cinfo,
            JSAMPIMAGE input_buf,
            JDIMENSION *in_row_group_ctr,
            JDIMENSION in_row_groups_avail,
            JSAMPARRAY output_buf,
            JDIMENSION *out_row_ctr,
            JDIMENSION out_rows_avail));
};

/* Marker reading & parsing */
struct jpegfw_marker_reader {
  JMETHOD(void, reset_marker_reader, (j_decompress_ptr cinfo));
  /* Read markers until SOS or EOI.
   * Returns same codes as are defined for jpegfw_consume_input:
   * JPEG_SUSPENDED, JPEG_REACHED_SOS, or JPEG_REACHED_EOI.
   */
  JMETHOD(int, read_markers, (j_decompress_ptr cinfo));
  /* Read a restart marker --- exported for use by entropy decoder only */
  jpegfw_marker_parser_method read_restart_marker;

  /* State of marker reader --- nominally internal, but applications
   * supplying COM or APPn handlers might like to know the state.
   */
  boolean saw_SOI;    /* found SOI? */
  boolean saw_SOF;    /* found SOF? */
  int next_restart_num;   /* next restart number expected (0-7) */
  unsigned int discarded_bytes; /* # of bytes skfwed looking for a marker */
};

/* Entropy decoding */
struct jpegfw_entropy_decoder {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo));
  JMETHOD(boolean, decode_mcu, (j_decompress_ptr cinfo,
        JBLOCKROW *MCU_data));

  /* This is here to share code between baseline and progressive decoders; */
  /* other modules probably should not use it */
  boolean insufficient_data;  /* set TRUE after emitting warning */
};

/* Inverse DCT (also performs dequantization) */
typedef JMETHOD(void, inverse_DCT_method_ptr,
    (j_decompress_ptr cinfo, jpegfw_component_info * compptr,
     JCOEFPTR coef_block,
     JSAMPARRAY output_buf, JDIMENSION output_col));

struct jpegfw_inverse_dct {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo));
  /* It is useful to allow each component to have a separate IDCT method. */
  inverse_DCT_method_ptr inverse_DCT[MAX_COMPONENTS];
};

/* Upsampling (note that upsampler must also call color converter) */
struct jpegfw_upsampler {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, upsample, (j_decompress_ptr cinfo,
         JSAMPIMAGE input_buf,
         JDIMENSION *in_row_group_ctr,
         JDIMENSION in_row_groups_avail,
         JSAMPARRAY output_buf,
         JDIMENSION *out_row_ctr,
         JDIMENSION out_rows_avail));

  boolean need_context_rows;  /* TRUE if need rows above & below */
};

/* Colorspace conversion */
struct jpegfw_color_deconverter {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, color_convert, (j_decompress_ptr cinfo,
        JSAMPIMAGE input_buf, JDIMENSION input_row,
        JSAMPARRAY output_buf, int num_rows));
};

/* Color quantization or color precision reduction */
struct jpegfw_color_quantizer {
  JMETHOD(void, start_pass, (j_decompress_ptr cinfo, boolean is_pre_scan));
  JMETHOD(void, color_quantize, (j_decompress_ptr cinfo,
         JSAMPARRAY input_buf, JSAMPARRAY output_buf,
         int num_rows));
  JMETHOD(void, finish_pass, (j_decompress_ptr cinfo));
  JMETHOD(void, new_color_map, (j_decompress_ptr cinfo));
};


/* Miscellaneous useful macros */

#undef MAX
#define MAX(a,b)  ((a) > (b) ? (a) : (b))
#undef MIN
#define MIN(a,b)  ((a) < (b) ? (a) : (b))


/* We assume that right shift corresponds to signed division by 2 with
 * rounding towards minus infinity.  This is correct for typical "arithmetic
 * shift" instructions that shift in copies of the sign bit.  But some
 * C compilers implement >> with an unsigned shift.  For these machines you
 * must define RIGHT_SHIFT_IS_UNSIGNED.
 * RIGHT_SHIFT provides a proper signed right shift of an INT32 quantity.
 * It is only applied with constant shift counts.  SHIFT_TEMPS must be
 * included in the variables of any routine using RIGHT_SHIFT.
 */

#ifdef RIGHT_SHIFT_IS_UNSIGNED
#define SHIFT_TEMPS INT32 shift_temp;
#define RIGHT_SHIFT(x,shft)  \
  ((shift_temp = (x)) < 0 ? \
   (shift_temp >> (shft)) | ((~((INT32) 0)) << (32-(shft))) : \
   (shift_temp >> (shft)))
#else
#define SHIFT_TEMPS
#define RIGHT_SHIFT(x,shft) ((x) >> (shft))
#endif


/* Short forms of external names for systems with brain-damaged linkers. */

#ifdef NEED_SHORT_EXTERNAL_NAMES
#define jinitfw_compress_master jICompress
#define jinitfw_c_master_control  jICMaster
#define jinitfw_c_main_controller jICMainC
#define jinitfw_c_prep_controller jICPrepC
#define jinitfw_c_coef_controller jICCoefC
#define jinitfw_color_converter jICColor
#define jinitfw_downsampler jIDownsampler
#define jinitfw_forward_dct jIFDCT
#define jinitfw_huff_encoder  jIHEncoder
#define jinitfw_phuff_encoder jIPHEncoder
#define jinitfw_marker_writer jIMWriter
#define jinitfw_master_decompress jIDMaster
#define jinitfw_d_main_controller jIDMainC
#define jinitfw_d_coef_controller jIDCoefC
#define jinitfw_d_post_controller jIDPostC
#define jinitfw_input_controller  jIInCtlr
#define jinitfw_marker_reader jIMReader
#define jinitfw_huff_decoder  jIHDecoder
#define jinitfw_phuff_decoder jIPHDecoder
#define jinitfw_inverse_dct jIIDCT
#define jinitfw_upsampler   jIUpsampler
#define jinitfw_color_deconverter jIDColor
#define jinitfw_1pass_quantizer jI1Quant
#define jinitfw_2pass_quantizer jI2Quant
#define jinitfw_merged_upsampler  jIMUpsampler
#define jinitfw_memory_mgr  jIMemMgr
#define jdiv_round_up   jDivRound
#define jround_up   jRound
#define jcopy_sample_rows jCopySamples
#define jcopy_block_row   jCopyBlocks
#define jzero_far   jZeroFar
#define jpegfw_zigzag_order jZIGTable
#define jpegfw_natural_order  jZAGTable
#endif /* NEED_SHORT_EXTERNAL_NAMES */


/* Compression module initialization routines */
EXTERN(void) jinitfw_compress_master JPP((j_compress_ptr cinfo));
EXTERN(void) jinitfw_c_master_control JPP((j_compress_ptr cinfo,
           boolean transcode_only));
EXTERN(void) jinitfw_c_main_controller JPP((j_compress_ptr cinfo,
            boolean need_full_buffer));
EXTERN(void) jinitfw_c_prep_controller JPP((j_compress_ptr cinfo,
            boolean need_full_buffer));
EXTERN(void) jinitfw_c_coef_controller JPP((j_compress_ptr cinfo,
            boolean need_full_buffer));
EXTERN(void) jinitfw_color_converter JPP((j_compress_ptr cinfo));
EXTERN(void) jinitfw_downsampler JPP((j_compress_ptr cinfo));
EXTERN(void) jinitfw_forward_dct JPP((j_compress_ptr cinfo));
EXTERN(void) jinitfw_huff_encoder JPP((j_compress_ptr cinfo));
EXTERN(void) jinitfw_phuff_encoder JPP((j_compress_ptr cinfo));
EXTERN(void) jinitfw_marker_writer JPP((j_compress_ptr cinfo));
/* Decompression module initialization routines */
EXTERN(void) jinitfw_master_decompress JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_d_main_controller JPP((j_decompress_ptr cinfo,
            boolean need_full_buffer));
EXTERN(void) jinitfw_d_coef_controller JPP((j_decompress_ptr cinfo,
            boolean need_full_buffer));
EXTERN(void) jinitfw_d_post_controller JPP((j_decompress_ptr cinfo,
            boolean need_full_buffer));
EXTERN(void) jinitfw_input_controller JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_marker_reader JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_huff_decoder JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_phuff_decoder JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_inverse_dct JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_upsampler JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_color_deconverter JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_1pass_quantizer JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_2pass_quantizer JPP((j_decompress_ptr cinfo));
EXTERN(void) jinitfw_merged_upsampler JPP((j_decompress_ptr cinfo));
/* Memory manager initialization */
EXTERN(void) jinitfw_memory_mgr JPP((j_common_ptr cinfo));

/* Utility routines in jutils.c */
EXTERN(long) jdiv_round_up JPP((long a, long b));
EXTERN(long) jround_up JPP((long a, long b));
EXTERN(void) jcopy_sample_rows JPP((JSAMPARRAY input_array, int source_row,
            JSAMPARRAY output_array, int dest_row,
            int num_rows, JDIMENSION num_cols));
EXTERN(void) jcopy_block_row JPP((JBLOCKROW input_row, JBLOCKROW output_row,
          JDIMENSION num_blocks));
EXTERN(void) jzero_far JPP((void FAR * target, size_t bytestozero));
/* Constant tables in jutils.c */
#if 0       /* This table is not actually needed in v6a */
extern const int jpegfw_zigzag_order[]; /* natural coef order to zigzag order */
#endif
extern const int jpegfw_natural_order[]; /* zigzag coef order to natural order */

/* Suppress undefined-structure complaints if necessary. */

#ifdef INCOMPLETE_TYPES_BROKEN
#ifndef AM_MEMORY_MANAGER /* only jmemmgr.c defines these */
struct jvirt_sarray_control { long dummy; };
struct jvirt_barray_control { long dummy; };
#endif
#endif /* INCOMPLETE_TYPES_BROKEN */
