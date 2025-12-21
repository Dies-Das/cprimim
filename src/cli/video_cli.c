#include "video_cli.h"
#include "cprimim.h"
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
#include <stdio.h>
#include <stdlib.h>

// print out the steps and errors
static void logging(const char *fmt, ...);
// decode packets into frames
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame,
                         Args *args);
// save a frame into a .pgm file
static void save_cprimim_frame(unsigned char *buf, int xsize, int ysize, Args *args);
int process_video(Args args)
{
    if (!args.output_path)
    {
        args.output_path = "out.svg";
    }
    AVFormatContext *pFormatContext = avformat_alloc_context();

    if (avformat_open_input(&pFormatContext, *args.input_path, NULL, NULL))
    {
        return EXIT_FAILURE;
    }

    if (avformat_find_stream_info(pFormatContext, NULL) < 0)
    {
        return EXIT_FAILURE;
    }
    AVCodec *pCodec = NULL;
    AVCodecParameters *pCodecParameters = NULL;
    int video_stream_index = -1;
    for (int i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVCodecParameters *pLocalCodecParameters = NULL;
        pLocalCodecParameters = pFormatContext->streams[i]->codecpar;
        printf("AVStream->time_base before open coded %d/%d",
               pFormatContext->streams[i]->time_base.num,
               pFormatContext->streams[i]->time_base.den);
        printf("AVStream->r_frame_rate before open coded %d/%d",
               pFormatContext->streams[i]->r_frame_rate.num,
               pFormatContext->streams[i]->r_frame_rate.den);
        printf("AVStream->start_time %" PRId64, pFormatContext->streams[i]->start_time);
        printf("AVStream->duration %" PRId64, pFormatContext->streams[i]->duration);

        printf("finding the proper decoder (CODEC)");

        AVCodec *pLocalCodec = NULL;

        // finds the registered decoder for a codec ID
        // https://ffmpeg.org/doxygen/trunk/group__lavc__decoding.html#ga19a0ca553277f019dd5b0fec6e1f9dca
        pLocalCodec = avcodec_find_decoder(pLocalCodecParameters->codec_id);

        if (pLocalCodec == NULL)
        {
            printf("ERROR unsupported codec!");
            // In this example if the codec is not found we just skip it
            continue;
        }

        // when the stream is a video we store its index, codec parameters and codec
        if (pLocalCodecParameters->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            if (video_stream_index == -1)
            {
                video_stream_index = i;
                pCodec = pLocalCodec;
                pCodecParameters = pLocalCodecParameters;
            }

            printf("Video Codec: resolution %d x %d", pLocalCodecParameters->width,
                   pLocalCodecParameters->height);
        }

        // print its name, id and bitrate
        printf("\tCodec %s ID %d bit_rate %ld", pLocalCodec->name, pLocalCodec->id,
               pLocalCodecParameters->bit_rate);
    }
    if (video_stream_index == -1)
    {
        logging("File %s does not contain a video stream!", *args.input_path);
        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVCodecContext.html
    AVCodecContext *pCodecContext = avcodec_alloc_context3(pCodec);
    if (!pCodecContext)
    {
        logging("failed to allocated memory for AVCodecContext");
        return -1;
    }

    // Fill the codec context based on the values from the supplied codec parameters
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#gac7b282f51540ca7a99416a3ba6ee0d16
    if (avcodec_parameters_to_context(pCodecContext, pCodecParameters) < 0)
    {
        logging("failed to copy codec params to codec context");
        return -1;
    }

    // Initialize the AVCodecContext to use the given AVCodec.
    // https://ffmpeg.org/doxygen/trunk/group__lavc__core.html#ga11f785a188d7d9df71621001465b0f1d
    if (avcodec_open2(pCodecContext, pCodec, NULL) < 0)
    {
        logging("failed to open codec through avcodec_open2");
        return -1;
    }

    // https://ffmpeg.org/doxygen/trunk/structAVFrame.html
    AVFrame *pFrame = av_frame_alloc();
    if (!pFrame)
    {
        logging("failed to allocate memory for AVFrame");
        return -1;
    }
    // https://ffmpeg.org/doxygen/trunk/structAVPacket.html
    AVPacket *pPacket = av_packet_alloc();
    if (!pPacket)
    {
        logging("failed to allocate memory for AVPacket");
        return -1;
    }
    int response = 0;
    int nr_frame = 0;
    // fill the Packet with data from the Stream
    // https://ffmpeg.org/doxygen/trunk/group__lavf__decoding.html#ga4fdb3084415a82e3810de6ee60e46a61
    while (av_read_frame(pFormatContext, pPacket) >= 0)
    {
        // if it's the video stream
        if (pPacket->stream_index == video_stream_index)
        {
            logging("AVPacket->pts %" PRId64, pPacket->pts);
            response = decode_packet(pPacket, pCodecContext, pFrame, &args);
            if (response < 0)
                break;
            if (nr_frame > 8)
                break;
            // stop it, otherwise we'll be saving hundreds of frames
            nr_frame++;
        }
        // https://ffmpeg.org/doxygen/trunk/group__lavc__packet.html#ga63d5a489b419bd5d45cfd09091cbcbc2
        av_packet_unref(pPacket);
    }

    logging("releasing all the resources");

    avformat_close_input(&pFormatContext);
    av_packet_free(&pPacket);
    av_frame_free(&pFrame);
    avcodec_free_context(&pCodecContext);
    return 0;
}

static void logging(const char *fmt, ...)
{
    va_list args;
    fprintf(stderr, "LOG: ");
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fprintf(stderr, "\n");
}
static int decode_packet(AVPacket *pPacket, AVCodecContext *pCodecContext, AVFrame *pFrame,
                         Args *args)
{
    int response = avcodec_send_packet(pCodecContext, pPacket);

    if (response < 0)
    {
        logging("Error while sending a packet to the decoder: %s", av_err2str(response));
        return response;
    }

    while (response >= 0)
    {
        response = avcodec_receive_frame(pCodecContext, pFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF)
        {
            break;
        }
        else if (response < 0)
        {
            logging("Error while receiving a frame from the decoder: %s", av_err2str(response));
            return response;
        }

        if (response >= 0)
        {
            printf("Color range: %d (1=MPEG/limited, 2=JPEG/full)\n", pFrame->color_range);
            printf("Color space: %d\n", pFrame->colorspace);
            struct SwsContext *sws = NULL;
            sws = sws_getCachedContext(sws, pFrame->width, pFrame->height, pFrame->format,
                                       pFrame->width, pFrame->height, AV_PIX_FMT_RGB24,
                                       SWS_BILINEAR | SWS_FULL_CHR_H_INT | SWS_ACCURATE_RND, NULL,
                                       NULL, NULL);
            const int *coeffs = sws_getCoefficients(SWS_CS_ITU601);
            sws_setColorspaceDetails(sws, coeffs, 0, // src: limited range
                                     coeffs, 1,      // dst: full range
                                     0, 1 << 16, 1 << 16);
            uint8_t *rgb_data[4] = {NULL};
            int rgb_linesize[4] = {0};

            int ret = av_image_alloc(rgb_data, rgb_linesize, pFrame->width, pFrame->height,
                                     AV_PIX_FMT_RGB24, 1);
            if (ret < 0)
            {
                logging("Failed to allocate RGB buffer");
                sws_freeContext(sws);
                return -1;
            }

            sws_scale(sws, (const uint8_t *const *)pFrame->data, pFrame->linesize, 0,
                      pFrame->height, rgb_data, rgb_linesize);
            static int frame_num = 0;
            char fname[64];
            snprintf(fname, sizeof(fname), "debug_%03d.ppm", frame_num++);
            FILE *f = fopen(fname, "wb");
            fprintf(f, "P6\n%d %d\n255\n", pFrame->width, pFrame->height);
            fwrite(rgb_data[0], 1, pFrame->width * pFrame->height * 3, f);
            fclose(f);
            if (pFrame->format != AV_PIX_FMT_YUV420P)
            {
                logging("Warning: the generated file may not be a grayscale image, but could e.g. "
                        "be just the R component if the video format is RGB");
            }
            printf("width: %d, expected stride: %d, actual linesize: %d\n", pFrame->width,
                   pFrame->width * 3, rgb_linesize[0]);
            save_cprimim_frame(rgb_data[0], pFrame->width, pFrame->height, args);

            av_freep(&rgb_data[0]);
            sws_freeContext(sws);
        }
    }
    return 0;
}

static void save_cprimim_frame(unsigned char *buf, int xsize, int ysize, Args *args)
{
    cprimim_Context *ctx = cprimim_create_context(
        (enum cprimim_shape)(*args->method), (cprimim_BackgroundType)*args->background,
        (size_t)(*args->nr_of_shapes), (size_t)(*args->initial_cells), (size_t)(*args->nr_of_tries),
        xsize, ysize, *args->background_shapes, *args->alpha);
    cprimim_set_input(ctx, buf);
    cprimim_approximate(ctx);
    FILE *ptr = fopen(args->output_path, "w");
    cprimim_to_svg(ctx, ptr);
    fclose(ptr);
    cprimim_destroy_context(ctx);
}
