#include <opencv2/opencv.hpp>
#define NK_INCLUDE_DEFAULT_ALLOCATOR
#define NK_IMPLEMENTATION
#include "../../nuklear.h"
#include "nuklear_cv.h"

typedef struct {
    int event;
    int x;
    int y;
    int flags;
} mouse_event_t;

typedef struct {
    int fontFace;
    int thickness;
} cv_font_t;

static void cv_user_mouse_callback_f(int event, int x, int y, int flags, void* userdata)
{
    mouse_event_t * m = (mouse_event_t*) userdata;
    m->event = event;
    m->x = x;
    m->y = y;
    m->flags = flags;
}

static float nk_user_text_width_f(nk_handle userdata, float h, const char* s, int len){
    cv_font_t* font = (cv_font_t *)userdata.ptr;
    double fontScale = 0.5;
    int baseline=0;
    cv::Size textSize = cv::getTextSize(s, font->fontFace, fontScale, font->thickness, &baseline);
    return textSize.width;
}

void
nk_cv_scissor(cv::Mat& frame, float x, float y, float w, float h)
{
//    XRectangle clip_rect;
//    clip_rect.x = (short)(x-1);
//    clip_rect.y = (short)(y-1);
//    clip_rect.width = (unsigned short)(w+2);
//    clip_rect.height = (unsigned short)(h+2);
//    XSetClipRectangles(surf->dpy, surf->gc, 0, 0, &clip_rect, 1, Unsorted);
}

void
nk_cv_rect(cv::Mat& frame, const struct nk_command_rect * cmd) {
    cv::Point pt1(cmd->x, cmd->y), pt2(cmd->x+cmd->w, cmd->y+cmd->h);
    const cv::Scalar& color = cv::Scalar(cmd->color.b, cmd->color.g, cmd->color.r);
    cv::rectangle(frame, pt1, pt2, color, std::max<int>(cmd->line_thickness, 1), /*lineType*/8, /*shift*/0);
}

void
nk_cv_rect(cv::Mat& frame, const struct nk_command_rect_filled * cmd) {
    cv::Point pt1(cmd->x - 1, cmd->y - 3), pt2(cmd->x+cmd->w+1, cmd->y+cmd->h);
    const cv::Scalar& color = cv::Scalar(cmd->color.b, cmd->color.g, cmd->color.r);
    cv::rectangle(frame, pt1, pt2, color, /*filled*/-1, /*lineType*/8, /*shift*/0);
}

void
nk_cv_text(cv::Mat& frame, const struct nk_command_text * cmd) {
    cv_font_t* font = (cv_font_t *)cmd->font->userdata.ptr;
    int baseline=0;
    std::string text = (const char*)cmd->string;
    baseline += font->thickness;
    
    double fontScale = 0.5;
    cv::Size textSize = cv::getTextSize(text, font->fontFace, fontScale, font->thickness, &baseline);
    cv::Point textOrg(cmd->x, cmd->y + cmd->h/2 + baseline + 1);
    cv::putText(frame, text, textOrg, font->fontFace, fontScale,
            cv::Scalar::all(255), font->thickness, 8);
}

void
nk_cv_fill_triangle(cv::Mat& frame, const struct nk_command_triangle_filled * cmd) {
    std::vector<cv::Point2i> pts;
    pts.push_back(cv::Point2i(cmd->a.x, cmd->a.y));
    pts.push_back(cv::Point2i(cmd->b.x, cmd->b.y));
    pts.push_back(cv::Point2i(cmd->c.x, cmd->c.y));
    cv::fillConvexPoly(frame, pts, cv::Scalar(cmd->color.b, cmd->color.g, cmd->color.r));
}

void
nk_cv_fill_circle(cv::Mat& frame, const struct nk_command_circle_filled * cmd) {
    cv::Point2i center = cv::Point2i(cmd->x + cmd->w/2, cmd->y + cmd->h/2);
    cv::Scalar color = cv::Scalar(cmd->color.b, cmd->color.g, cmd->color.r);
    cv::ellipse(frame, cv::RotatedRect(center, cv::Size(cmd->w + 3, cmd->h + 3), 0), color, -1);
}

void
nk_cv_draw_image(cv::Mat& frame, const struct nk_command_image * cmd) {
    cv::Mat * img = (cv::Mat *)cmd->img.handle.ptr;
    cv::Rect roi(cmd->x, cmd->y, img->cols, std::min<int>(cmd->h, img->rows));
    cv::Mat dst(frame, roi);
    cv::MatIterator_<cv::Vec3b> dst_it = dst.begin<cv::Vec3b>();
    cv::MatConstIterator_<cv::Vec3b> src_it = img->begin<cv::Vec3b>();
    cv::MatConstIterator_<cv::Vec3b> dst_end = dst.end<cv::Vec3b>();
    for( ; dst_it != dst_end; ++src_it, ++dst_it )
    {
        *dst_it = *src_it;
    }
}

int main(int argc, char** argv) {

    const char* WINNAME = argv[0];

    struct nk_context ctx;
    cv_font_t cv_font; {
        cv_font.fontFace = cv::FONT_HERSHEY_SIMPLEX;
        cv_font.thickness = 1;
    }
    struct nk_user_font font; {
        font.userdata.ptr = (void*)&cv_font;
        font.height = 8;
        font.width = nk_user_text_width_f;
    }
    nk_init_default(&ctx, &font);
    cv::Mat frame = cv::Mat(600, 800, CV_8UC3);

    cv::VideoCapture cap(0);

    cv::namedWindow(WINNAME, cv::WINDOW_AUTOSIZE);
    mouse_event_t mouse{0};
    cv::setMouseCallback(WINNAME, cv_user_mouse_callback_f, &mouse);
    bool nk_cv_ok = true;
    while (nk_cv_ok) {
        // [Input]
        nk_input_begin(&ctx);
        char ch = cv::waitKey(10);
        if (0x20 <= ch || ch <= 0x7e) {
            nk_input_char(&ctx, ch);
        } else {
            switch(ch){
            case '\xff':
                break;
            default:
                fprintf(stderr, "Unhandled key event: %x\n", ch);
                break;
            }
        }
        if (-1 == cv::getWindowProperty(WINNAME, 1)){
            break;
        }
        struct nk_vec2 wheel_val; 
        switch(mouse.event){
        case cv::EVENT_MOUSEMOVE:
            nk_input_motion(&ctx, mouse.x, mouse.y);
            break;
        case cv::EVENT_LBUTTONDOWN:
            nk_input_button(&ctx, NK_BUTTON_LEFT, mouse.x, mouse.y, 1);
            break;
        case cv::EVENT_LBUTTONUP:
            nk_input_button(&ctx, NK_BUTTON_LEFT, mouse.x, mouse.y, 0);
            break;
        case cv::EVENT_RBUTTONDOWN:
            nk_input_button(&ctx, NK_BUTTON_RIGHT, mouse.x, mouse.y, 1);
            break;
        case cv::EVENT_RBUTTONUP:
            nk_input_button(&ctx, NK_BUTTON_RIGHT, mouse.x, mouse.y, 0);
            break;
        case cv::EVENT_LBUTTONDBLCLK:
            nk_input_button(&ctx, NK_BUTTON_DOUBLE, mouse.x, mouse.y, 1);
            break;
        case cv::EVENT_MOUSEWHEEL:
            wheel_val.x = 0.f;
            wheel_val.y = cv::getMouseWheelDelta(mouse.flags) > 0 ? 0.1f : -0.1f;
            nk_input_scroll(&ctx, wheel_val);
            break;
        case cv::EVENT_MOUSEHWHEEL:
            wheel_val.x = cv::getMouseWheelDelta(mouse.flags) > 0 ? 0.1f : -0.1f;
            wheel_val.y = 0.f;
            nk_input_scroll(&ctx, wheel_val);
            break;
        default:
            break;
        }
        nk_input_end(&ctx);

        // [GUI]
        if (nk_begin(&ctx, "Demo", nk_rect(50, 50, 200, 200),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            enum {EASY, HARD};
            static int op = EASY;
            static int property = 20;
            static int checkbox_val = 1;

            nk_layout_row_dynamic(&ctx, 30, 1);
            if (nk_button_label(&ctx, "button"))
                fprintf(stdout, "button pressed\n");
            nk_layout_row_dynamic(&ctx, 30, 2);
            if (nk_option_label(&ctx, "easy", op == EASY)) op = EASY;
            if (nk_option_label(&ctx, "hard", op == HARD)) op = HARD;
            nk_layout_row_dynamic(&ctx, 25, 1);
            nk_property_int(&ctx, "Compression:", 0, &property, 100, 10, 1);
            nk_layout_row_dynamic(&ctx, 25, 1);
            nk_checkbox_label(&ctx, "blablabla", &checkbox_val);
        }
        nk_end(&ctx);

        if (nk_begin(&ctx, "Preview", nk_rect(270, 50, 400, 400),
            NK_WINDOW_BORDER|NK_WINDOW_MOVABLE|NK_WINDOW_SCALABLE|
            NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE|NK_WINDOW_TITLE))
        {
            static cv::Mat video;
            static cv::Mat resized;
            static struct nk_image img = nk_image_ptr((void*)&resized);
            struct nk_panel* layout = ctx.current->layout;
            {
                cap.read(video);
                float fx = (float)(layout->bounds.w) / video.cols;
                float fy = (float)(layout->bounds.h) / video.rows;
                fx = std::min<float>(fx, fy);
                cv::resize(video, resized, cv::Size(), fx, fx);
                img.w = resized.cols;
                img.h = resized.rows;
            }
            nk_layout_row_dynamic(&ctx, layout->bounds.h, 1);
            nk_image(&ctx, img);
        }
        nk_end(&ctx);

        // [Draw]
        frame = cv::Scalar(49, 52, 49);
        const struct nk_command *cmd = 0;
        nk_foreach(cmd, &ctx) {
            switch (cmd->type) {
            case NK_COMMAND_NOP: break;
            case NK_COMMAND_SCISSOR: break;
            ///case NK_COMMAND_SCISSOR: {
            ///    const struct nk_command_scissor *s =(const struct nk_command_scissor*)cmd;
            ///    nk_cv_scissor(frame, s->x, s->y, s->w, s->h);
            ///} break;
            ///case NK_COMMAND_LINE: {
            ///    const struct nk_command_line *l = (const struct nk_command_line *)cmd;
            ///    nk_xsurf_stroke_line(surf, l->begin.x, l->begin.y, l->end.x,
            ///        l->end.y, l->line_thickness, l->color);
            ///} break;
            case NK_COMMAND_RECT: {
                const struct nk_command_rect *r = (const struct nk_command_rect *)cmd;
                nk_cv_rect(frame, r);
            } break;
            case NK_COMMAND_RECT_FILLED: {
                const struct nk_command_rect_filled *r = (const struct nk_command_rect_filled *)cmd;
                nk_cv_rect(frame, r);
            } break;
            ///case NK_COMMAND_CIRCLE: {
            ///    const struct nk_command_circle *c = (const struct nk_command_circle *)cmd;
            ///    nk_xsurf_stroke_circle(surf, c->x, c->y, c->w, c->h, c->line_thickness, c->color);
            ///} break;
            case NK_COMMAND_CIRCLE_FILLED: {
                const struct nk_command_circle_filled *c = (const struct nk_command_circle_filled *)cmd;
                nk_cv_fill_circle(frame, c);
            } break;
            ///case NK_COMMAND_TRIANGLE: {
            ///    const struct nk_command_triangle*t = (const struct nk_command_triangle*)cmd;
            ///    nk_xsurf_stroke_triangle(surf, t->a.x, t->a.y, t->b.x, t->b.y,
            ///        t->c.x, t->c.y, t->line_thickness, t->color);
            ///} break;
            case NK_COMMAND_TRIANGLE_FILLED: {
                const struct nk_command_triangle_filled *t = (const struct nk_command_triangle_filled *)cmd;
                nk_cv_fill_triangle(frame, t);
            } break;
            ///case NK_COMMAND_POLYGON: {
            ///    const struct nk_command_polygon *p =(const struct nk_command_polygon*)cmd;
            ///    nk_xsurf_stroke_polygon(surf, p->points, p->point_count, p->line_thickness,p->color);
            ///} break;
            ///case NK_COMMAND_POLYGON_FILLED: {
            ///    const struct nk_command_polygon_filled *p = (const struct nk_command_polygon_filled *)cmd;
            ///    nk_xsurf_fill_polygon(surf, p->points, p->point_count, p->color);
            ///} break;
            ///case NK_COMMAND_POLYLINE: {
            ///    const struct nk_command_polyline *p = (const struct nk_command_polyline *)cmd;
            ///    nk_xsurf_stroke_polyline(surf, p->points, p->point_count, p->line_thickness, p->color);
            ///} break;
            case NK_COMMAND_TEXT: {
                const struct nk_command_text *t = (const struct nk_command_text*)cmd;
                nk_cv_text(frame, t);
            } break;
            ///case NK_COMMAND_CURVE: {
            ///    const struct nk_command_curve *q = (const struct nk_command_curve *)cmd;
            ///    nk_xsurf_stroke_curve(surf, q->begin, q->ctrl[0], q->ctrl[1],
            ///        q->end, 22, q->line_thickness, q->color);
            ///} break;
            case NK_COMMAND_IMAGE: {
                const struct nk_command_image *i = (const struct nk_command_image *)cmd;
                nk_cv_draw_image(frame, i);
            } break;
            case NK_COMMAND_RECT_MULTI_COLOR:
            case NK_COMMAND_ARC:
            case NK_COMMAND_ARC_FILLED:
            case NK_COMMAND_CUSTOM:
            default:
                fprintf(stderr, "Unhandled command %d\n", cmd->type);
                //nk_cv_ok = false;
                break;
            }
        }
        cv::imshow(WINNAME, frame);
        nk_clear(&ctx);
    }
    nk_free(&ctx);

    return 0;
}
