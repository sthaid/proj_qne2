#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <inttypes.h>  //xxx check these
#include <limits.h>

#include <setjmp.h>
#include <jpeglib.h>

#include <png.h>

#include "../../src/sdlx.h"

// jpeg documentationn: 
//   http://www.opensource.apple.com/source/tcl/tcl-87/tcl_ext/tkimg/tkimg/libjpeg/libjpeg.doc
//   https://refspecs.linuxfoundation.org/LSB_4.1.0/LSB-Desktop-generic/LSB-Desktop-generic/libjpegman.html
//   https://github.com/LuaDist/libjpeg/blob/master/example.c

//
// defines
//

#define BYTES_PER_PIXEL 4

//
// typedefs
//

//
// variables
//


//
// prototypes
//

static int read_jpeg_file(char* file_name, unsigned char ** pixels, int * width, int * height);
static int write_jpeg_file(char* file_name, unsigned char * pixels, int width, int height);

int32_t read_png_file(char* file_name, uint8_t ** pixels, int32_t * width, int32_t * height);

// -----------------  MAIN  ----------------------------------------------------------------

int square(int x)
{
    return x * x;
}

void tweaks(unsigned char *pixels_arg, int w, int h)
{
    int xctr, yctr, x, y, dist_squared, limit_squared;
    unsigned int *pixels = (unsigned int *)pixels_arg;

    limit_squared = square(350/2);
    // w must equal h

    xctr = w / 2;
    yctr = h / 2;

    for (y = 0; y < h; y++) {
        for (x = 0; x < w; x++) {
            dist_squared = square(x - xctr) + square(y - yctr);
            if (dist_squared > limit_squared) {
                *pixels = 0;
            }
            pixels++;
        }
    }
}

int main(int argc, char **argv)
{
    int             rc, w, h, len;
    unsigned char  *pixels;
    sdlx_pixels_t  *x;
    sdlx_texture_t *texture;
    FILE           *fp;
    char           *filename = argv[1];

    if (argc != 2) {
        printf("ERROR filename expected\n");
        return 1;
    }

    if (strstr(filename, ".jpeg")) {
        // read jpeg file
        rc = read_jpeg_file(filename, &pixels, &w, &h);
        printf("rc = %d w = %d h = %d\n", rc, w, h);
        if (rc != 0) {
            printf("ERROR: read_jpeg_file %s failed\n", filename);
            return 1;
        }
    } else if (strstr(filename, ".png")) {
        // read png file
        rc = read_png_file(filename, &pixels, &w, &h);
        printf("rc = %d w = %d h = %d\n", rc, w, h);
        if (rc != 0) {
            printf("ERROR: read_png_file %s failed\n", filename);
            return 1;
        }
    } else {
        printf("ERROR not jpeg or png\n");
        return 1;
    }

    unsigned int *p32 = (unsigned int *)pixels;
    for (int i = 0; i < 10; i++) {
        printf("%08x\n", p32[i]);
    }

    // tweaks
    //tweaks(pixels, w, h);

    // xxx
    rc = write_jpeg_file("out.jpeg", pixels, w, h);
    printf("write_jpeg_file ret %d\n", rc);

    // create sdlx_pixels_t
    len = sizeof(sdlx_pixels_t) + w * h * BYTES_PER_PIXEL;
    x = malloc(len);
    memset(x, 0, len);
    x->magic = PIXELS_MAGIC;
    x->struct_len = len;
    x->w = w;
    x->h = h;
    memcpy(x->pixels, pixels, w * h * BYTES_PER_PIXEL);

    // display using sdlx
    sdlx_init(SUBSYS_VIDEO);
    sdlx_display_init(COLOR_BLUE);
    texture = sdlx_create_texture_from_pixels(x);
    sdlx_render_texture(0, 0, sdlx_win_width ,sdlx_win_width, 0, texture);
    sdlx_display_present();
    sleep(10);
    sdlx_quit(SUBSYS_VIDEO);

    // write file
    fp = fopen("compass.pixels", "wb");
    if (fp == NULL) {
        printf("ERROR: failed to open xxx.pixels, %s\n", strerror(errno));
        return 1;
    }
    fwrite(x, len, 1, fp);
    fclose(fp);

    return 0;
}

// -----------------  READ JPEG FILE  -----------------------------------------------------

static jmp_buf err_jmpbuf;

static void jpeg_decode_error_exit_override(j_common_ptr cinfo);
static void jpeg_decode_output_message_override(j_common_ptr cinfo);

// Args: 
// - file_name: pathname of the jpeg file to be read
// - pixels: the pixels is malloced by read_jpeg_file; the caller should free
//   this memory when done; 4 bytes per pixel; in SDL_PIXELFORMAT_ABGR8888
// - width, height: return the image width and height

static int read_jpeg_file(char* file_name, unsigned char ** pixels, int * width, int * height)
{
    FILE                          * fp = NULL;
    struct jpeg_decompress_struct   cinfo; 
    struct jpeg_error_mgr           err_mgr;
    unsigned char                       * out = NULL;

    // preset returns to caller
    *pixels = NULL;
    *width  = 0;
    *height = 0;

    // open file_name
    fp = fopen(file_name, "rb");
    if (!fp) {
        printf("ERROR: fopen %s\n", file_name);
        return -1;
    }

    // initailze setjmp, for use by the error exit override
    if (setjmp(err_jmpbuf)) {
        goto error_return;
    }

    // error management init:
    // - override the error_exit routine
    // - override the output_message routine
    cinfo.err = jpeg_std_error(&err_mgr);
    cinfo.err->error_exit = jpeg_decode_error_exit_override;
    cinfo.err->output_message = jpeg_decode_output_message_override;

    // initialize the jpeg decompress object,
    // supply fp to the jpeg decoder,
    // read the jpeg header, require_image==true
    jpeg_create_decompress(&cinfo);
    jpeg_stdio_src(&cinfo, fp);
    jpeg_read_header(&cinfo, true);

    // set the desired output pixel format
    cinfo.out_color_space = JCS_RGB;

    //xxx comment,  make this an arg
    cinfo.scale_denom = 8;

    // initialize the decompression, this sets cinfo.output_width and cinfo.output_height
    jpeg_start_decompress(&cinfo);

    // allocate memory for the output, must be after call to jpeg_start_decompress
    out = malloc(cinfo.output_width * cinfo.output_height * BYTES_PER_PIXEL);
    if (out == NULL) {
        printf("ERROR: failed allocate memory for width=%d height=%d bytes_per_pixel=%d\n",
               cinfo.output_width, cinfo.output_height, BYTES_PER_PIXEL);
        goto error_return;
    }

    // loop over scanlines
    unsigned char * outp = out;
    while (cinfo.output_scanline < cinfo.output_height) {
        int i;
        JSAMPLE   row[1000000];
        JSAMPROW  scanline[1] = { row };
        unsigned char * r = row;

        // read one scanline
        jpeg_read_scanlines(&cinfo, scanline, 1);

        // save the row data in the output buffer
        for (i = 0; i < cinfo.output_width; i++) {
            outp[0] = r[0];
            outp[1] = r[1];
            outp[2] = r[2];
            outp[3] = 255;  
            outp+=4;
            r+=3;
        }
    }

    // finish decompress
    jpeg_finish_decompress(&cinfo);

    // success return
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    *pixels = out;
    *width  = cinfo.output_width;
    *height = cinfo.output_height;
    return 0;

    // error return
error_return:
    jpeg_destroy_decompress(&cinfo);
    fclose(fp);
    free(out);
    return -1;
}


// -----------------  WRITE JPEG FILE  -----------------------------------------------------

static int write_jpeg_file(char* file_name, unsigned char * pixels, int width, int height)
{
    FILE                        * fp = NULL;
    struct jpeg_compress_struct   cinfo; 
    struct jpeg_error_mgr         err_mgr;

    // open file_name
    fp = fopen(file_name, "wb");
    if (!fp) {
        printf("ERROR: fopen %s\n", file_name);
        return -1;
    }

    // initailze setjmp, for use by the error exit override
    if (setjmp(err_jmpbuf)) {
        goto error_return;
    }

    // error management init:
    // - override the error_exit routine
    // - override the output_message routine
    cinfo.err = jpeg_std_error(&err_mgr);
    cinfo.err->error_exit = jpeg_decode_error_exit_override;
    cinfo.err->output_message = jpeg_decode_output_message_override;

    // initialize the jpeg compress object,
    // supply fp to the jpeg encoder,
    jpeg_create_compress(&cinfo);
    jpeg_stdio_dest(&cinfo, fp);

    // describe the input image
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;

    // set default compression parameters
    jpeg_set_defaults(&cinfo);

    // initialize the compression
    jpeg_start_compress(&cinfo, TRUE);

    // loop over scanlines
    unsigned char * inp = pixels;
    while (cinfo.next_scanline < cinfo.image_height) {
        int   i;
        JSAMPLE   row[1000000];
        JSAMPROW  scanline[1] = { row };
        unsigned char * r = row;

        // convert scanline pixelformat from 
        // SDL_PIXELFORMAT_ABGR8888 to JCS_RGB
        for (i = 0; i < cinfo.image_width; i++) {
            r[0] = inp[0];
            r[1] = inp[1];
            r[2] = inp[2];
            inp += 4;
            r   += 3;
        }

        // write the JCS_RGB scanline
        jpeg_write_scanlines(&cinfo, scanline, 1);
    }

    // finish compress
    jpeg_finish_compress(&cinfo);

    // success return
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return 0;

    // error return
error_return:
    jpeg_destroy_compress(&cinfo);
    fclose(fp);
    return -1;
}

// -----------------  SUPPORT  -------------------------------------------------------------

static void jpeg_decode_error_exit_override(j_common_ptr cinfo)
{
    (*cinfo->err->output_message)(cinfo);
    longjmp(err_jmpbuf, 1);
}

static void jpeg_decode_output_message_override(j_common_ptr cinfo)
{
    char buffer[JMSG_LENGTH_MAX];

    (*cinfo->err->format_message)(cinfo, buffer);
    if (strncmp(buffer, "Not a JPEG file", 15) != 0) {  //xxx is the if needed
        printf("ERROR: %s\n", buffer);
    } else {
        printf("ERROR: %s\n", buffer);
    }
}





// -----------------  READ PNG FILE  ---------------------------------------------------

#define PNG_COLOR_TYPE_STR(x) \
    ((x) == PNG_COLOR_TYPE_GRAY       ? "PNG_COLOR_TYPE_GRAY" : \
     (x) == PNG_COLOR_TYPE_PALETTE    ? "PNG_COLOR_TYPE_PALETTE" : \
     (x) == PNG_COLOR_TYPE_RGB        ? "PNG_COLOR_TYPE_RGB" : \
     (x) == PNG_COLOR_TYPE_RGB_ALPHA  ? "PNG_COLOR_TYPE_RGB_ALPHA" : \
     (x) == PNG_COLOR_TYPE_GRAY_ALPHA ? "PNG_COLOR_TYPE_GRAY_ALPHA"  \
                                      : "????")



//
// Args:  
// - file_name: pathname of the png file to be read
// - max_image_dim: currently not implemented; the intent is if the 
//   image width or height exceeds max_image_dim then the image size would be
//   scaled down such that the width and height are less or equal to max_image_dim
// - pixels: the pixels_arg is malloced by read_png_file; the caller should free
//   this memory when done; 4 bytes per pixel; in SDL_PIXELFORMAT_ABGR8888
// - width_arg, height_arg: return the image width and height
//
// Notes:          
// - the only png file format currently supported is PNG_COLOR_TYPE_RGB_ALPHA
//

int32_t read_png_file(char* file_name,
                   uint8_t ** pixels_arg, int32_t * width_arg, int32_t * height_arg)
{
    FILE        * fp           = NULL;
    png_structp   png_ptr      = NULL;
    png_infop     png_info     = NULL;
    uint8_t     * pixels       = NULL;
    uint8_t    ** row_pointers = NULL;
    uint8_t       hdr[8];
    int32_t       len, width, height, color_type, rowbytes, y, ret;
    int32_t       bit_depth __attribute__((unused));

    // preset returns to caller
    *pixels_arg = NULL;
    *width_arg  = 0;
    *height_arg = 0;

    // open file_name
    fp = fopen(file_name, "rb");
    if (!fp) {
        printf("ERROR: %s: fopen failed, %s\n", file_name, strerror(errno));
        goto error;
    }

    // read and verify header
    len = fread(hdr, 1, sizeof(hdr), fp);
    if (len != sizeof(hdr)) {
        printf("ERROR: %s: hdr read failed, len=%d, %s\n", file_name, len, strerror(errno));
        goto error;
    }
    if (png_sig_cmp(hdr, 0, sizeof(hdr))) {
        printf("ERROR: %s: is not a png file\n", file_name);
        goto error;
    }

    // init
    png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (png_ptr == NULL) {
        printf("ERROR: %s: png_create_read_struct failed\n", file_name);
        goto error;
    }

    png_info = png_create_info_struct(png_ptr);
    if (png_info == NULL) {
        printf("ERROR: %s: png_create_info_struct failed\n", file_name);
        goto error;
    }

    // register error jmpbuf
    if (setjmp(png_jmpbuf(png_ptr))) {
        printf("ERROR: %s: failed\n", file_name);
        goto error;
    }

    // provide the file-pointer to png
    png_init_io(png_ptr, fp);

    // inform png that we've already read the signature
    png_set_sig_bytes(png_ptr, 8);

    // read png info
    png_read_info(png_ptr, png_info);
    width      = png_get_image_width(png_ptr, png_info);
    height     = png_get_image_height(png_ptr, png_info);
    color_type = png_get_color_type(png_ptr, png_info);
    bit_depth  = png_get_bit_depth(png_ptr, png_info);
    printf("INFO: width=%d height=%d color_type=%s bit_depth=%d\n",
          width, height, PNG_COLOR_TYPE_STR(color_type), bit_depth);

    // currently this routine supports only PNG_COLOR_TYPE_RGB_ALPHA
    if (color_type != PNG_COLOR_TYPE_RGB_ALPHA) {
        printf("ERROR: %s: unsupported color_type %s\n", file_name, PNG_COLOR_TYPE_STR(color_type));
        goto error;
    }

    // get the number of bytes in a row, and
    // allocate memory for the pixels
    rowbytes = png_get_rowbytes(png_ptr,png_info);
    printf("INFO: rowbytes=%d\n", rowbytes);
    pixels = malloc(height*rowbytes);
    if (pixels == NULL) {
        printf("ERROR: %s: malloc pixels failed, %dx%d\n", file_name, width, height);
        goto error;
    }

    // allocate and init row_pointers
    row_pointers = malloc(sizeof(void*) * height);
    for (y = 0; y < height; y++) {
        row_pointers[y] = pixels + y * rowbytes;
    }

    // read the image
    png_read_image(png_ptr, row_pointers);

    // success return
    *width_arg  = width;
    *height_arg = height;
    *pixels_arg = pixels;
    ret = 0;
    goto cleanup;

    // error return
error:
    ret = -1;
    goto cleanup;

    // cleanup and return
cleanup:
    if (fp) {
        fclose(fp);
    }
    free(row_pointers);
    if (ret == -1) {
        free(pixels);
    }
    png_destroy_read_struct(&png_ptr, &png_info, NULL);
    return ret;
}

