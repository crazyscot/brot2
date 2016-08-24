/*
    MovieRender-libav.cpp: brot2 movie rendering via libav
    Copyright (C) 2016 Ross Younger

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "MovieRender.h"
#include "MovieMode.h"
#include "Render2.h"
#include "Plot3Plot.h"
#include "Exception.h"
#include "BaseHUD.h"
#include <iostream>
#include <iomanip>
#include <errno.h>

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libavresample/avresample.h"
#include "libswscale/swscale.h"
}

using namespace std;

// ------------------------------------------------------------------------------

#define SCALE_FLAGS SWS_BICUBIC

SUBCLASS_BROTEXCEPTION(AVException);

class LibAV : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class LibAV;

		AVOutputFormat *fmt;
		AVFormatContext *oc;

		AVStream *st;

		/* pts of the next frame that will be generated */
		int64_t next_pts;

		AVFrame *frame;
		AVFrame *tmp_frame;

		float t, tincr, tincr2;

		struct SwsContext *sws_ctx;
		AVAudioResampleContext *avr;

		Plot3::Plot3Plot *plot;
		Plot3::ChunkDivider::Horizontal10px divider;
		Render2::MemoryBuffer *render;
		unsigned char *render_buf;
		std::shared_ptr<const BrotPrefs::Prefs> prefs;

		Private(Movie::RenderJob& _job) :
			RenderInstancePrivate(_job),
			st(0), next_pts(0), frame(0), tmp_frame(0), sws_ctx(0), avr(0),
			plot(0), render(0), render_buf(0), prefs(BrotPrefs::Prefs::getMaster())
		{
		}
		virtual ~Private() {
			avcodec_close(st->codec); // what does the rv mean?
			av_frame_free(&frame);
			av_frame_free(&tmp_frame);
			sws_freeContext(sws_ctx);
			sws_ctx = 0;
			avresample_free(&avr);
			avio_close(oc->pb); // may return error
			avformat_free_context(oc);
			oc = 0;
			delete render;
			delete render_buf;
		}
	};
	public:
		LibAV() : Movie::Renderer("Movie file via LibAV", "*.mov") { }

		static AVFrame* alloc_picture(enum AVPixelFormat pix_fmt, int width, int height) {
			AVFrame *rv = av_frame_alloc();
			if (!rv) return 0;
			rv->format = pix_fmt;
			rv->width = width;
			rv->height = height;
			int ret = av_frame_get_buffer(rv, 32);
			if (ret < 0)
				THROW(AVException, "Could not allocate frame data");
			return rv;
		}

		void render_top(Movie::RenderJob& job, Movie::RenderInstancePrivate **priv) {
			av_register_all();
			Private *mypriv = new Private(job);
			*priv = mypriv;

			mypriv->fmt = av_guess_format(0, job._filename.c_str(), 0);
			if (!mypriv->fmt)
				mypriv->fmt = av_guess_format("mov", 0, 0);
			if (!mypriv->fmt)
				THROW(AVException,"Could not find output format");
			
			// Set up video stream
			mypriv->oc = avformat_alloc_context();
			if (!mypriv->oc)
				THROW(AVException,"Could not alloc format context");
			mypriv->oc->oformat = mypriv->fmt;
			snprintf(mypriv->oc->filename, sizeof(mypriv->oc->filename), "%s", job._filename.c_str());

			AVCodec * codec = avcodec_find_encoder(mypriv->fmt->video_codec);
			if (!codec)
				THROW(AVException,"Could not find codec");
			mypriv->st = avformat_new_stream(mypriv->oc, codec);
			if (!mypriv->st)
				THROW(AVException,"Could not alloc stream");
			// grab https://git.libav.org/?p=libav.git;a=history;f=doc/examples/output.c;h=44a55f564545d66681914aa905cdc069d2233363;hb=HEAD
			// NOTE: For later versions of libav, compare:
			// https://git.libav.org/?p=libav.git;a=commitdiff;h=9897d9f4e074cdc6c7f2409885ddefe300f18dc7
			AVCodecContext *c = mypriv->st->codec;
			c->width = job._movie.width;
			c->height = job._movie.height;
			c->bit_rate = c->width * c->height * job._movie.fps * 3 / 25; // This magic constant (3/25) determined from "HQ" recommendations in table 1 on http://www.lighterra.com/papers/videoencodingh264/
			mypriv->st->time_base = (AVRational){ 1, (int)job._movie.fps };
			c->time_base = mypriv->st->time_base;
			c->gop_size = 12;
			c->pix_fmt = AV_PIX_FMT_YUV420P; // We generate RGB24, so will convert
			if (mypriv->oc->oformat->flags & AVFMT_GLOBALHEADER)
				c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			// Open video codec
			if (avcodec_open2(c, 0, 0) < 0)
				THROW(AVException,"Could not open codec");
			mypriv->frame = alloc_picture(c->pix_fmt, c->width, c->height);
			if (!mypriv->frame)
				THROW(AVException,"Could not alloc picture");
			if (c->pix_fmt != AV_PIX_FMT_RGB24) {
				mypriv->tmp_frame = alloc_picture(AV_PIX_FMT_RGB24, c->width, c->height);
				if (!mypriv->tmp_frame)
					THROW(AVException,"Could not alloc 2nd picture");
			}
			/* Not present in this version of libavcodec...
			int ret = avcodec_parameters_from_context(mypriv->st->codecpar, c);
			if (ret<0)
				THROW(AVException,"Could not copy the stream parameters");
			*/
				
			av_dump_format(mypriv->oc, 0, mypriv->oc->filename, 1);
			if (avio_open(&mypriv->oc->pb, mypriv->oc->filename, AVIO_FLAG_WRITE) < 0)
				THROW(AVException,"Could not open file");
			avformat_write_header(mypriv->oc, 0);

			mypriv->render_buf = new unsigned char[3*c->width*c->height];
			// TODO Get MemoryBuffer to render directly into frame[_tmp].
			mypriv->render = new Render2::MemoryBuffer(mypriv->render_buf, 3*c->width/*rowstride*/, c->width, c->height,
					job._movie.antialias, -1/*local_inf*/,
					Render2::pixpack_format::PACKED_RGB_24, *job._movie.palette);
			// Update local_inf later with mypriv->render->fresh_local_inf() if needed.
		}

		void render_to_frame(Private * mypriv, AVFrame * frame) {
			int ret;
			ret = av_frame_make_writable(frame);
			if (ret < 0) THROW(AVException, "Could not make frame writable");

			ASSERT( frame->linesize[0] >= 3 * (int)mypriv->job._movie.width );
			for (unsigned y=0; y < mypriv->job._movie.height; y++) {
				memcpy(&frame->data[0][y * frame->linesize[0]],
						&mypriv->render_buf[y * mypriv->render->rowstride()],
						mypriv->job._movie.width * mypriv->render->pixelstep());
			}
		}

		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, unsigned n_frames) {
			Private * mypriv = (Private*)(priv);

			mypriv->plot = new Plot3::Plot3Plot(mypriv->job._threads, mypriv->job._reporter,
					*mypriv->job._movie.fractal, mypriv->divider,
					fr.centre, fr.size,
					mypriv->job._rwidth, mypriv->job._rheight);
			mypriv->plot->start();
			mypriv->job._reporter->set_chunks_count(mypriv->plot->chunks_total());
			mypriv->plot->wait();

			mypriv->render->process(mypriv->plot->get_chunks__only_after_completion());
			if (mypriv->job._movie.draw_hud)
				BaseHUD::apply(*mypriv->render, mypriv->prefs, mypriv->plot, false, false);

			// Prepare the video frame
			AVCodecContext *c = mypriv->st->codec;
			// We generate in RGB24 but libav wants YUV420P, so convert
			if (c->pix_fmt != AV_PIX_FMT_RGB24) {
				if (!mypriv->sws_ctx) {
					mypriv->sws_ctx = sws_getContext(c->width, c->height,
							AV_PIX_FMT_RGB24,
							c->width, c->height,
							c->pix_fmt,
							SCALE_FLAGS, 0, 0, 0);
					if (!mypriv->sws_ctx)
						THROW(AVException, "Cannot allocate conversion context");
				}
				render_to_frame(mypriv, mypriv->tmp_frame);
				sws_scale(mypriv->sws_ctx, mypriv->tmp_frame->data, mypriv->tmp_frame->linesize,
						0, c->height, mypriv->frame->data, mypriv->frame->linesize);
			} else {
				render_to_frame(mypriv, mypriv->frame);
			}

			// Write the video frame
			for (unsigned i=0; i<n_frames; i++) {
				mypriv->frame->pts = mypriv->next_pts++;
				AVPacket pkt;
				memset(&pkt, 0, sizeof pkt);
				int got_packet = 0;
				av_init_packet(&pkt);
				int ret = avcodec_encode_video2(c, &pkt, mypriv->frame, &got_packet);
				if (ret<0) THROW(AVException, "Error encoding video frame");
				if (got_packet) {
					av_packet_rescale_ts(&pkt, c->time_base, mypriv->st->time_base);
					pkt.stream_index = mypriv->st->index;
					ret = av_interleaved_write_frame(mypriv->oc, &pkt);
				}
				if (ret<0) THROW(AVException, "Error writing video frame, code "+ret);
			}

			delete mypriv->plot;
			mypriv->plot = 0;

		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);

			AVPacket pkt;
			memset(&pkt, 0, sizeof pkt);
			int got_packet = 1, ret;
			av_init_packet(&pkt);
			AVCodecContext *c = mypriv->st->codec;
			// Need some NULL frames to flush the output codec
			while (got_packet) {
				ret = avcodec_encode_video2(c, &pkt, 0, &got_packet);
				if (ret<0) THROW(AVException, "Error encoding video frame, code "+ret);
				if (got_packet) {
					av_packet_rescale_ts(&pkt, c->time_base, mypriv->st->time_base);
					pkt.stream_index = mypriv->st->index;
					ret = av_interleaved_write_frame(mypriv->oc, &pkt);
					if (ret<0) THROW(AVException, "Error writing video frame, code "+ret);
				}
			}
			ret = av_interleaved_write_frame(mypriv->oc, 0);
			if (ret<0) THROW(AVException, "Error writing video flush, code "+ret);

			ret = av_write_trailer(mypriv->oc);
			if (ret<0) THROW(AVException, "Error writing trailer, code "+ret);
		}
		virtual ~LibAV() {}
};

// ------------------------------------------------------------------------------
// And now some instances to make them live

MOVIERENDER_DECLARE_FACTORY(LibAV, MOVIERENDER_NAME_MOV, "*.mov");
