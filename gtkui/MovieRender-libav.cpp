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
#include "gtkutil.h"
#include "IMovieProgress.h"
#include <iostream>
#include <iomanip>
#include <errno.h>
#include <gtkmm/window.h>
#include <gtkmm/scrolledwindow.h>
#include <gtkmm/textview.h>
#include <gtkmm/box.h>
#include <gtkmm/filechooserdialog.h>
#include <gtkmm/stock.h>
#include <gtkmm/texttag.h>

extern "C" {
#include "libavutil/channel_layout.h"
#include "libavutil/mathematics.h"
#include "libavutil/log.h"
#include "libavutil/opt.h"
#include "libavformat/avformat.h"
#include "libavresample/avresample.h"
#include "libswscale/swscale.h"
}

using namespace std;

// ------------------------------------------------------------------------------

#define SCALE_FLAGS SWS_BICUBIC
#define LIBAV_LINE_SIZE 1024 // Source: https://www.ffmpeg.org/doxygen/2.7/log_8c_source.html

SUBCLASS_BROTEXCEPTION(AVException);

class ConsoleOutputWindow : public Gtk::Window {
		Gtk::TextView *tv;
		bool verbose;

		Glib::RefPtr<Gtk::TextBuffer::Mark> mark;
		Gtk::CheckButton *autoclose;
		static ConsoleOutputWindow * _instance; // SINGLETON
		static std::mutex mux; // Protects _instance

		Glib::RefPtr<Gtk::TextTag> t_panic;
		Glib::RefPtr<Gtk::TextTag> t_fatal;
		Glib::RefPtr<Gtk::TextTag> t_error;
		Glib::RefPtr<Gtk::TextTag> t_warning;
		Glib::RefPtr<Gtk::TextTag> t_info;

	// private constructor!
		ConsoleOutputWindow() : Gtk::Window(), tv(0), verbose(false) {
			set_title("libav output");
			set_type_hint(Gdk::WindowTypeHint::WINDOW_TYPE_HINT_UTILITY);

			Gtk::VBox *vbox = Gtk::manage(new Gtk::VBox());
			add(*vbox);

			Gtk::HBox *hbox = Gtk::manage(new Gtk::HBox());
			autoclose = Gtk::manage(new Gtk::CheckButton("Close this window when render complete"));
			hbox->pack_start(*autoclose);
			// TODO: autoclose state could be a Pref?

			Gtk::Button *save = Gtk::manage(new Gtk::Button("Save log..."));
			save->signal_clicked().connect(sigc::mem_fun(*this, &ConsoleOutputWindow::do_save));
			hbox->pack_start(*save);

			vbox->pack_start(*hbox);

			Gtk::ScrolledWindow * swin = Gtk::manage(new Gtk::ScrolledWindow());
			vbox->pack_start(*swin);
			swin->set_policy(Gtk::PolicyType::POLICY_AUTOMATIC, Gtk::PolicyType::POLICY_AUTOMATIC);

			tv = Gtk::manage(new Gtk::TextView());
			swin->add(*tv);
			auto buf = tv->get_buffer();
			mark = buf->create_mark(buf->end());
			tv->set_wrap_mode(Gtk::WrapMode::WRAP_WORD_CHAR);
			tv->set_size_request(500,500); // Arbitrary size
			tv->set_editable(false);
			tv->set_cursor_visible(false);
			//show_all(); // Don't do this, activate() will do it.

			// Set up some tags to format our text.
			auto tags = buf->get_tag_table();
			// Info - blue
			t_info=Gtk::TextTag::create("info");
			t_info->property_foreground().set_value("blue");
			tags->add(t_info);
			// Warning - yellow on black
			t_warning=Gtk::TextTag::create("warning");
			t_warning->property_foreground().set_value("yellow");
			t_warning->property_background().set_value("black");
			tags->add(t_warning);
			// Error - red on black
			t_error=Gtk::TextTag::create("error");
			t_error->property_foreground().set_value("red");
			t_error->property_background().set_value("black");
			tags->add(t_error);
			// Fatal - red on yellow
			t_fatal=Gtk::TextTag::create("fatal");
			t_fatal->property_foreground().set_value("red");
			t_fatal->property_background().set_value("yellow");
			tags->add(t_fatal);
			// Panic - red on orange
			t_panic=Gtk::TextTag::create("panic");
			t_panic->property_foreground().set_value("blue");
			t_panic->property_background().set_value("orange");
			tags->add(t_panic);
		}
		virtual ~ConsoleOutputWindow() {}
		static void replacement_av_log(void *ptr, int level, const char* fmt, va_list vl) {
			if (level > av_log_get_level()) {
				//std::cerr << "level " << level << ":" << fmt << std::endl;
				return;
			}
			char line[LIBAV_LINE_SIZE];
			int print_prefix = 1;
			av_log_format_line(ptr, level, fmt, vl, line, sizeof(line), &print_prefix);
			// libav sanitises the log line, so let's do that as well
			char *tmp = line;
			while (*tmp) {
				if (*tmp < 8 || (*tmp > 0xd && *tmp < 32)) *tmp = '?';
				++tmp;
			}
			_instance->log(level, line, line+strlen(line));
		}

		// Must have gdk_threads lock
		static ConsoleOutputWindow* get_instance(bool verbose=false) {
			std::unique_lock<std::mutex> lock(mux);
			if (!_instance) {
				_instance = new ConsoleOutputWindow();
				av_log_set_callback(&ConsoleOutputWindow::replacement_av_log);
				if (verbose)
					av_log_set_level(AV_LOG_DEBUG);
				else
					av_log_set_level(AV_LOG_VERBOSE);
				// Debug level contains one line per frame encoded.
				// Trace inclues flushing details.
			}
			return _instance;
		}
		// Must have gdk_threads lock
		void reset() {
			auto buf = tv->get_buffer();
			Gtk::TextBuffer::iterator it_begin, it_end;
			buf->get_bounds(it_begin, it_end);
			buf->erase(it_begin, it_end);
			buf->move_mark(mark, --buf->end());
			tv->scroll_to(buf->create_mark(buf->end()));
		}
		void do_save() {
			Gtk::FileChooserDialog dialog(*this, "Save log", Gtk::FileChooserAction::FILE_CHOOSER_ACTION_SAVE);
			dialog.set_do_overwrite_confirmation(true);
			dialog.add_button(Gtk::Stock::CANCEL, Gtk::ResponseType::RESPONSE_CANCEL);
			dialog.add_button(Gtk::Stock::SAVE, Gtk::ResponseType::RESPONSE_ACCEPT);
			int rv = dialog.run();
			if (rv != Gtk::ResponseType::RESPONSE_ACCEPT) return;

			ofstream file;
			file.open(dialog.get_filename());
			if (!file.is_open()) {
				Util::alert(this, "Unable to open file for writing");
				return;
			}
			auto buf = tv->get_buffer();
			// This is a bit nasty, appears to copy out the whole buffer contents.
			Gtk::TextBuffer::iterator it_begin, it_end;
			buf->get_bounds(it_begin, it_end);
			auto text = buf->get_text(it_begin, it_end);
			file << text;
			file.close();
		}
		void position_for(Movie::IMovieProgressReporter * parent) {
			Gtk::Window * pw = dynamic_cast<Gtk::Window*> (parent);
			if (!pw) return; // We only know how to deal with Gtk::Windows...
			int ww, hh, px, py;
			Util::get_screen_geometry(*this, ww, hh);
			pw->get_position(px, py);
			// Try to position below the progress window.
			int xx = px;
			int yy = py + pw->get_height();
			// Off the bottom? Try above.
			if (yy + get_height() > hh) yy = py - get_height() - 20;
			// Off the top? Don't bother trying to be smart, let it go where it goes.
			if (yy >= 0) {
				Util::fix_window_coords(*this, xx, yy); // Sanity check
				move(xx,yy);
			}
		}
	public:
		static void activate(Movie::IMovieProgressReporter * parent, std::shared_ptr<const BrotPrefs::Prefs> prefs) {
			gdk_threads_enter();

			auto inst = get_instance(prefs->get(PREF(LibAVLogVerbose)));
			inst->reset();
			inst->show_all();
			inst->position_for(parent);
			gdk_threads_leave();
			/*
			av_log(0, AV_LOG_INFO, "test info\n");
			av_log(0, AV_LOG_WARNING, "test warning\n");
			av_log(0, AV_LOG_ERROR, "test error\n");
			av_log(0, AV_LOG_FATAL, "test fatal\n");
			av_log(0, AV_LOG_PANIC, "test panic\n");
			*/
		}
		virtual bool on_delete_event(GdkEventAny *) {
			hide();
			return true; // We only pretend to delete this singleton
		}
		void log(int level, const char* begin, const char* end) {
			gdk_threads_enter();
			auto buf = tv->get_buffer();
			Glib::RefPtr<Gtk::TextTag> tag(0);
			switch(level) {
				case AV_LOG_INFO: tag = t_info; break;
				case AV_LOG_WARNING: tag = t_warning; break;
				case AV_LOG_ERROR: tag = t_error; break;
				case AV_LOG_FATAL: tag = t_fatal; break;
				case AV_LOG_PANIC: tag = t_panic; break;
			}
			if (tag)
				buf->insert_with_tag(buf->end(), begin, end, tag);
			else
				buf->insert(buf->end(), begin, end);
			buf->move_mark(mark, --buf->end());
			tv->scroll_to(buf->create_mark(buf->end()));
			gdk_threads_leave();
		}
		static void render_finished() {
			auto in = get_instance();
			if (in->autoclose->get_active())
				in->hide();
		}
};
ConsoleOutputWindow * ConsoleOutputWindow::_instance; // SINGLETON
std::mutex ConsoleOutputWindow::mux; // Protects _instance

class LibAV : public Movie::Renderer {
	class Private : public Movie::RenderInstancePrivate {
		friend class LibAV;

		AVOutputFormat *fmt;
		AVFormatContext *oc;

		AVStream *st;
		AVCodecContext *avctx;

		/* pts of the next frame that will be generated */
		int64_t next_pts;

		AVFrame *frame;
		AVFrame *tmp_frame;

		bool finished_cleanly;

		struct SwsContext *sws_ctx;
		AVAudioResampleContext *avr;

		Plot3::Plot3Plot *plot;
		Plot3::ChunkDivider::Horizontal10px divider;
		Render2::MemoryBuffer *render;
		std::shared_ptr<const BrotPrefs::Prefs> prefs;

		unsigned actual_fps;

		Private(Movie::RenderJob& _job) :
			RenderInstancePrivate(_job),
			fmt(0), oc(0), st(0), next_pts(0), frame(0), tmp_frame(0), finished_cleanly(false), sws_ctx(0), avr(0),
			plot(0), render(0), prefs(BrotPrefs::Prefs::getMaster())
		{
			ConsoleOutputWindow::activate(_job._reporter, prefs);

			// In preview mode, we divide the timebase by 2 so it matches the number of frames we're actually putting out, e.g. 1/25 fps -> 1/12 fps.
			actual_fps = job._movie.fps / (job._movie.preview ? 2 : 1);
			if (actual_fps == 0) actual_fps = 1;
		}
		virtual ~Private() {
			av_frame_free(&frame);
			av_frame_free(&tmp_frame);
			sws_freeContext(sws_ctx);
			sws_ctx = 0;
			if (oc) {
				avio_close(oc->pb); // may return error
				avformat_free_context(oc);
				oc = 0;
			}
			if (finished_cleanly)
				ConsoleOutputWindow::render_finished(); // may hide Console
			delete render;
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
			std::string url = "file://" + job._filename;
			mypriv->oc->url = strdup(url.c_str());

			AVCodec * codec = avcodec_find_encoder(mypriv->fmt->video_codec);
			if (!codec)
				THROW(AVException,"Could not find codec");
			mypriv->st = avformat_new_stream(mypriv->oc, codec);
			if (!mypriv->st)
				THROW(AVException,"Could not alloc stream");
			// grab https://git.libav.org/?p=libav.git;a=history;f=doc/examples/output.c;h=44a55f564545d66681914aa905cdc069d2233363;hb=HEAD
			// NOTE: For later versions of libav, compare:
			// https://git.libav.org/?p=libav.git;a=commitdiff;h=9897d9f4e074cdc6c7f2409885ddefe300f18dc7
			//AVCodecContext *c = mypriv->st->codec;
			AVCodecParameters *cp = mypriv->st->codecpar;
			// libav throws a rod if width and height are not multiples of 2, quietly enforce this
			// NOTE: Must use height/width from mypriv->st->codec within the renderer as it may not equal the passed-in dimensions

			mypriv->st->time_base = (AVRational){ 1, (int)mypriv->actual_fps };
			//if (mypriv->oc->oformat->flags & AVFMT_GLOBALHEADER)
				//c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

			// Open video codec
			mypriv->avctx = avcodec_alloc_context3(nullptr);

			cp->codec_id = AV_CODEC_ID_H264;
			cp->codec_type = AVMEDIA_TYPE_VIDEO;
			cp->format = AV_PIX_FMT_YUV420P;
			cp->width = job._movie.width + (job._movie.width % 2);
			cp->height = job._movie.height + (job._movie.height % 2);
			cp->bit_rate = cp->width * cp->height * mypriv->actual_fps * 6 / 25;
			/* The magic constant 6/25 gives us a bit rate in the region of (slightly above) 8Mbps for 1080p and 5Mbps for 720p.
			 * At 2560x1440 (2K) it comes out at 22.1Mbit, where YouTube recommend 16;
			 * at 3840x2160 (4K) it gives 49.8Mbit against a recommendation of 35-45. */
			// std::cerr << "Creating video at bit rate " << cp->bit_rate << std::endl;

			if (avcodec_parameters_to_context(mypriv->avctx, cp) < 0)
				THROW(AVException,"Could not convert codec parameters");
			mypriv->avctx->time_base = mypriv->st->time_base;
			mypriv->avctx->gop_size = 12; /* Trade-off better compression against the ability to seek. */
			mypriv->avctx->qmin = 10;
			mypriv->avctx->qmax = 51;

			assert(mypriv->avctx->width == cp->width);

			AVDictionary* options = nullptr;
			// These options taken from x264's default preset (see x264/common/common.c):
			av_dict_set(&options, "tune", "zerolatency", 0);
			av_dict_set(&options, "me-range", "12", 0);
			av_dict_set(&options, "qpstep", "4", 0);
			av_dict_set(&options, "keyint", "250", 0);
			av_dict_set(&options, "qp-min", "0", 0);
			av_dict_set(&options, "qp-max", "69", 0);
			av_dict_set(&options, "qcomp", "0.6", 0);
			av_dict_set(&options, "ip-factor", "1.4", 0);
			av_dict_set(&options, "pb-factor", "1.3", 0);
			av_dict_set(&options, "subq", "7", 0);

			if (avcodec_open2(mypriv->avctx, codec, &options) < 0)
				THROW(AVException,"Could not open codec");
			mypriv->frame = alloc_picture(AV_PIX_FMT_YUV420P, cp->width, cp->height);
			if (!mypriv->frame)
				THROW(AVException,"Could not alloc picture");
			{
				mypriv->tmp_frame = alloc_picture(AV_PIX_FMT_RGB24, cp->width, cp->height);
				if (!mypriv->tmp_frame)
					THROW(AVException,"Could not alloc 2nd picture");
			}

			av_dump_format(mypriv->oc, 0, url.c_str(), 1);
			if (avio_open(&mypriv->oc->pb, url.c_str(), AVIO_FLAG_WRITE) < 0)
				THROW(AVException,"Could not open file");
			if (avformat_write_header(mypriv->oc, 0))
				THROW(AVException,"Could not write header");

			AVFrame * ren_frame = mypriv->tmp_frame ? mypriv->tmp_frame : mypriv->frame;
			mypriv->render = new Render2::MemoryBuffer(
					ren_frame->data[0],
					ren_frame->linesize[0],
					cp->width, cp->height,
					job._movie.antialias, -1/*local_inf*/,
					Render2::pixpack_format::PACKED_RGB_24, *job._movie.palette, job._movie.preview/*upscale*/);
			// Update local_inf later with mypriv->render->fresh_local_inf() if needed.
		}

		void render_frame(const struct Movie::Frame& fr, Movie::RenderInstancePrivate *priv, unsigned n_frames) {
			Private * mypriv = (Private*)(priv);

			const int upfactor = mypriv->job._movie.preview ? 2 : 1;
			mypriv->plot = new Plot3::Plot3Plot(mypriv->job._threads, mypriv->job._reporter,
					*mypriv->job._movie.fractal, mypriv->divider,
					fr.centre, fr.size,
					mypriv->job._rwidth / upfactor, mypriv->job._rheight / upfactor);
			// LATER: Why not apply the upfactor to _rwidth in MovieRender?
			mypriv->plot->start();
			mypriv->job._reporter->set_chunks_count(mypriv->plot->chunks_total());
			mypriv->plot->wait();

			int ret=0;
			if (mypriv->tmp_frame)
				ret = av_frame_make_writable(mypriv->tmp_frame);
			else
				ret = av_frame_make_writable(mypriv->frame);
			if (ret < 0) THROW(AVException, "Could not make frame writeable");

			mypriv->render->process(mypriv->plot->get_chunks__only_after_completion());
			if (mypriv->job._movie.draw_hud)
				BaseHUD::apply(*mypriv->render, mypriv->prefs, mypriv->plot, false, false);

			// Prepare the video frame
			AVCodecParameters *cp = mypriv->st->codecpar;
			// We generate in RGB24 but libav wants YUV420P, so convert
			{
				if (!mypriv->sws_ctx) {
					mypriv->sws_ctx = sws_getContext(cp->width, cp->height,
							AV_PIX_FMT_RGB24,
							cp->width, cp->height,
							AV_PIX_FMT_YUV420P,
							SCALE_FLAGS, 0, 0, 0);
					if (!mypriv->sws_ctx)
						THROW(AVException, "Cannot allocate conversion context");
				}
				sws_scale(mypriv->sws_ctx, mypriv->tmp_frame->data, mypriv->tmp_frame->linesize,
						0, cp->height, mypriv->frame->data, mypriv->frame->linesize);
			}

			// Write the video frame
			for (unsigned i=0; i<n_frames; i++) {
				mypriv->frame->pts = mypriv->next_pts++;
				AVPacket pkt;
				memset(&pkt, 0, sizeof pkt);
				av_init_packet(&pkt);
				int ret = avcodec_send_frame(mypriv->avctx, mypriv->frame);
				if (ret<0) THROW(AVException, "Error encoding video frame");
				bool done = false;
				while (!done) {
					ret = avcodec_receive_packet(mypriv->avctx, &pkt);
					switch(ret) {
						case 0:
							av_packet_rescale_ts(&pkt, mypriv->avctx->time_base, mypriv->st->time_base);
							pkt.stream_index = mypriv->st->index;
							ret = av_interleaved_write_frame(mypriv->oc, &pkt);
							if (ret<0) THROW(AVException, "Error writing video frame, code "+ret);
							break; // go round again
						case AVERROR(EAGAIN):
							done = true;
							break;
						default:
							THROW(AVException, "Error preparing video frame, code "+ret);
					}
				}
			}

			delete mypriv->plot;
			mypriv->plot = 0;

		}
		void render_tail(Movie::RenderInstancePrivate *priv, bool) {
			Private * mypriv = (Private*)(priv);

			AVPacket pkt;
			memset(&pkt, 0, sizeof pkt);
			int ret;
			av_init_packet(&pkt);
			// Need some NULL frames to flush the output codec
			ret = avcodec_send_frame(mypriv->avctx, nullptr);
			if (ret<0) THROW(AVException, "Error encoding video frame, code "+ret);
			bool done=false;
			while (!done) {
				ret = avcodec_receive_packet(mypriv->avctx, &pkt);
				switch(ret) {
					case 0:
						av_packet_rescale_ts(&pkt, mypriv->avctx->time_base, mypriv->st->time_base);
						pkt.stream_index = mypriv->st->index;
						ret = av_interleaved_write_frame(mypriv->oc, &pkt);
						if (ret<0) THROW(AVException, "Error writing video frame, code "+ret);
						break; // go round again
					case AVERROR(EAGAIN):
						ret = avcodec_send_frame(mypriv->avctx, nullptr);
						if (ret<0) THROW(AVException, "Error sending video frame, code "+ret);
						break;
					case AVERROR_EOF:
						done=true;
						break;
					default:
						THROW(AVException, "Error preparing video frame, code "+ret);
				}
			}

			ret = av_interleaved_write_frame(mypriv->oc, 0);
			if (ret<0) THROW(AVException, "Error writing video flush, code "+ret);

			ret = av_write_trailer(mypriv->oc);
			if (ret<0) THROW(AVException, "Error writing trailer, code "+ret);

			mypriv->finished_cleanly = true;
		}
		virtual ~LibAV() {}
};

// ------------------------------------------------------------------------------
// And now some instances to make them live

MOVIERENDER_DECLARE_FACTORY(LibAV, MOVIERENDER_NAME_MOV, "*.mov");
