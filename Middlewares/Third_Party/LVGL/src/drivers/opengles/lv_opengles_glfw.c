/**
 * @file lv_opengles_glfw.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "lv_opengles_glfw.h"
#if LV_USE_GLFW

#include "lv_opengles_window.h"
#include "lv_opengles_driver.h"
#include "lv_opengles_texture.h"
#include "lv_opengles_private.h"
#include "lv_opengles_debug.h"

#include "../../core/lv_refr.h"
#include "../../stdlib/lv_sprintf.h"
#include "../../stdlib/lv_string.h"
#include "../../core/lv_global.h"
#include "../../display/lv_display_private.h"
#include "../../indev/lv_indev.h"
#include "../../lv_init.h"
#include "../../misc/lv_area_private.h"

#include <stdlib.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

struct _lv_opengles_window_t {
    GLFWwindow * window;
    int32_t hor_res;
    int32_t ver_res;
    bool h_flip;
    bool v_flip;
    lv_ll_t textures; /* 当前窗口的图层链表。链表里的每一项要么是普通 OpenGL 纹理，要么是由 LVGL display 驱动出来的纹理/窗口显示层。 */
    lv_point_t mouse_last_point;
    lv_indev_state_t mouse_last_state;
    uint8_t use_indev : 1;
    uint8_t closing : 1;
#if LV_USE_DRAW_OPENGLES
    uint8_t direct_render_invalidated: 1; /* 标记“直出优化”是否失效。为 1 时，下一帧必须走完整的清屏+逐层合成流程，不能直接让 LVGL 直接画满整个窗口。 */
#endif
};

struct _lv_opengles_window_texture_t {
    lv_opengles_window_t * window;
    unsigned int texture_id; /* 为 0 时，表示这一层不是独立 texture，而是直接面向窗口的 display 层。 */
    lv_display_t * disp; /* 非 NULL 表示这一层背后关联着一个 LVGL display，可能是 texture display，也可能是 window display。 */
    uint8_t * fb; /* 非 NULL 时，表示这是软件绘制路径下的 window display framebuffer。 */
    lv_area_t area;
    lv_opa_t opa;
    lv_indev_t * indev;
    lv_point_t indev_last_point;
    lv_indev_state_t indev_last_state;
};

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void window_update_handler(lv_timer_t * t);
static uint32_t lv_glfw_tick_count_callback(void);
static lv_opengles_window_t * lv_glfw_get_lv_window_from_window(GLFWwindow * window);
static void glfw_error_cb(int error, const char * description);
static int lv_glfw_init(void);
static int lv_glew_init(void);
static void lv_glfw_timer_init(void);
static void lv_glfw_window_config(GLFWwindow * window, bool use_mouse_indev);
static void lv_glfw_window_quit(void);
static void window_close_callback(GLFWwindow * window);
static void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods);
static void mouse_button_callback(GLFWwindow * window, int button, int action, int mods);
static void mouse_move_callback(GLFWwindow * window, double xpos, double ypos);
static void proc_mouse(lv_opengles_window_t * window);
static void indev_read_cb(lv_indev_t * indev, lv_indev_data_t * data);
static void framebuffer_size_callback(GLFWwindow * window, int width, int height);
static void window_display_delete_cb(lv_event_t * e);
static void window_display_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map);
#if !LV_USE_DRAW_OPENGLES
    static void ensure_init_window_display_texture(void);
#endif

/**********************
 *  STATIC VARIABLES
 **********************/
static bool glfw_inited;
static bool glew_inited;
static lv_timer_t * update_handler_timer;
static lv_ll_t glfw_window_ll;
#if !LV_USE_DRAW_OPENGLES
    static unsigned int window_display_texture;
#endif

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

lv_opengles_window_t * lv_opengles_glfw_window_create_ex(int32_t hor_res, int32_t ver_res, bool use_mouse_indev,
                                                         bool h_flip, bool v_flip,  const char * title)
{
    LV_ASSERT_NULL(title);
    /* 这里必须先把 GLFW 初始化好。
     * 因为后面无论是创建窗口、拿 OpenGL 上下文，还是初始化 shader / buffer / texture，
     * 都要求底层的 native window system 和 GL context 管理先处于可用状态。 */
    if(lv_glfw_init() != 0) {
        LV_LOG_ERROR("Failed to init glfw");
        return NULL;
    }

    lv_opengles_window_t * window = lv_ll_ins_tail(&glfw_window_ll);
    LV_ASSERT_MALLOC(window);
    if(window == NULL) return NULL;
    if(window == NULL) {
        LV_LOG_ERROR("Failed to create glfw window");
        return NULL;
    }
    lv_memzero(window, sizeof(*window));

    /* 如果进程里已经存在窗口，这里把第一个窗口的 context 作为 share context 传给 GLFW。
     * 这样做的目的，是让多个模拟器窗口之间共享 OpenGL 资源，例如 shader program、
     * buffer、texture 等，避免每个窗口都各自维护一份重复的 GPU 资源。 */
    lv_opengles_window_t * existing_window = lv_ll_get_head(&glfw_window_ll);
    window->window = glfwCreateWindow(hor_res, ver_res, title, NULL,
                                      existing_window ? existing_window->window : NULL);
    if(window->window == NULL) {
        LV_LOG_ERROR("glfwCreateWindow fail");
        lv_ll_remove(&glfw_window_ll, window);
        lv_free(window);
        return NULL;
    }

    glfwSetWindowTitle(window->window, title);

    window->h_flip = h_flip;
    window->v_flip = v_flip;
    window->hor_res = hor_res;
    window->ver_res = ver_res;

    lv_ll_init(&window->textures, sizeof(lv_opengles_window_texture_t));
    window->use_indev = use_mouse_indev;
#if LV_USE_DRAW_OPENGLES
    window->direct_render_invalidated = 1;
#endif

    /* 把我们自己的 `lv_opengles_window_t` 挂到 GLFWwindow 上。
     * 后面 GLFW 回调只有原生 window 指针，借助这个 user pointer 才能回到 LVGL 这一层的数据结构。 */
    glfwSetWindowUserPointer(window->window, window);
    lv_glfw_timer_init();
    lv_glfw_window_config(window->window, use_mouse_indev);
    lv_glew_init();
    glfwMakeContextCurrent(window->window);
    /* OpenGL 的全局渲染状态在这里延迟初始化。
     * 注意必须先 `glfwMakeContextCurrent`，否则后面的 GL API 没有当前上下文可用。 */
    lv_opengles_init();

    return window;
}

lv_opengles_window_t * lv_opengles_glfw_window_create(int32_t hor_res, int32_t ver_res, bool use_mouse_indev)
{
    return lv_opengles_glfw_window_create_ex(hor_res, ver_res, use_mouse_indev, false, false, "LVGL Simulator");
}

void lv_opengles_glfw_window_set_title(lv_opengles_window_t * window, const char * new_title)
{
    glfwSetWindowTitle(window->window, new_title);
}

void lv_opengles_window_delete(lv_opengles_window_t * window)
{
    glfwDestroyWindow(window->window);

    lv_opengles_window_texture_t * texture;
    while((texture = lv_ll_get_head(&window->textures))) {
        if(texture->texture_id) {
            lv_opengles_window_texture_remove(texture);
        }
        else {
            lv_display_delete(texture->disp);
        }
    }

    lv_ll_remove(&glfw_window_ll, window);
    lv_free(window);

    if(lv_ll_is_empty(&glfw_window_ll)) {
        lv_glfw_window_quit();
#if !LV_USE_DRAW_OPENGLES
        if(window_display_texture) {
            GL_CALL(glDeleteTextures(1, &window_display_texture));
            window_display_texture = 0;
        }
#endif
    }
}

void * lv_opengles_glfw_window_get_glfw_window(lv_opengles_window_t * window)
{
    return (void *)(window->window);
}

void lv_opengles_glfw_window_set_flip(lv_opengles_window_t * window, bool h_flip, bool v_flip)
{
    window->h_flip = h_flip;
    window->v_flip = v_flip;
}

lv_opengles_window_texture_t * lv_opengles_window_add_texture(lv_opengles_window_t * window, unsigned int texture_id,
                                                              int32_t w, int32_t h)
{
    /* 一个 GLFW 窗口内部可以看成一个简单的“图层合成器”。
     * 这里往窗口里挂入一层新的纹理，这个纹理既可以来自 LVGL 自己创建的 display texture，
     * 也可以是外部代码传进来的原生 OpenGL texture。后续刷新时会按链表顺序逐层绘制。 */
    lv_opengles_window_texture_t * texture = lv_ll_ins_tail(&window->textures);
    LV_ASSERT_MALLOC(texture);
    if(texture == NULL) return NULL;
    lv_memzero(texture, sizeof(*texture));
    texture->window = window;
    texture->texture_id = texture_id;
    texture->disp = lv_opengles_texture_get_from_texture_id(texture_id);
    lv_area_set(&texture->area, 0, 0, w - 1, h - 1);
    texture->opa = LV_OPA_COVER;

    if(window->use_indev && texture->disp) {
        /* 只有“背后真有一个 LVGL display”的纹理层，才有资格绑定输入设备。
         * 因为鼠标事件最终是要送进 LVGL 的对象树里的；如果只是一个外部裸 texture，
         * 它并没有对应的 LVGL display，自然也就没有事件分发目标。 */
        lv_indev_t * indev = lv_indev_create();
        if(indev == NULL) {
            lv_ll_remove(&window->textures, texture);
            lv_free(texture);
            return NULL;
        }
        texture->indev = indev;
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, indev_read_cb);
        lv_indev_set_driver_data(indev, texture);
        lv_indev_set_mode(indev, LV_INDEV_MODE_EVENT);
        lv_indev_set_display(indev, texture->disp);
    }

#if LV_USE_DRAW_OPENGLES
    window->direct_render_invalidated = 1;
#endif

    return texture;
}

void lv_opengles_window_texture_remove(lv_opengles_window_texture_t * texture)
{
    if(texture->texture_id == 0) {
        LV_LOG_WARN("window displays should be deleted with `lv_display_delete`");
        return;
    }
    if(texture->indev != NULL) {
        lv_indev_delete(texture->indev);
    }

#if LV_USE_DRAW_OPENGLES
    texture->window->direct_render_invalidated = 1;
#endif

    lv_ll_remove(&texture->window->textures, texture);
    lv_free(texture);
}

lv_display_t * lv_opengles_window_display_create(lv_opengles_window_t * window, int32_t w, int32_t h)
{
    /* 这里创建的是“窗口直连型 display”。
     * 它和 `lv_opengles_texture_create()` 的区别在于：
     * 前者把 LVGL 的结果输出到一个可复用的 GL 纹理；
     * 而这里是把这个 display 作为窗口中的一个显示层来管理，最终直接呈现在 GLFW 窗口中。 */
    lv_display_t * disp = lv_display_create(w, h);
    if(disp == NULL) {
        return NULL;
    }

    lv_opengles_window_texture_t * dsc = lv_ll_ins_tail(&window->textures);
    LV_ASSERT_MALLOC(dsc);
    if(dsc == NULL) {
        lv_display_delete(disp);
        return NULL;
    }
    lv_memzero(dsc, sizeof(*dsc));
    dsc->window = window;
    dsc->disp = disp;
    lv_area_set(&dsc->area, 0, 0, w - 1, h - 1);
    dsc->opa = LV_OPA_COVER;

#if LV_USE_DRAW_OPENGLES
    static size_t LV_ATTRIBUTE_MEM_ALIGN dummy_buf;
    /* 如果启用了 `LV_USE_DRAW_OPENGLES`，LVGL 的绘制会尽量直接走 OpenGL，
     * 不再依赖真正的 CPU framebuffer。
     * 但 LVGL display 框架本身仍然要求“注册过显示缓冲区”，因此这里给一个 dummy buffer，
     * 只是为了满足 display 初始化契约，并不是真的拿它存整帧像素。 */
    lv_display_set_buffers(disp, &dummy_buf, NULL, h * lv_draw_buf_width_to_stride(w, LV_COLOR_FORMAT_ARGB8888),
                           LV_DISPLAY_RENDER_MODE_FULL);
#else
    uint32_t stride = lv_draw_buf_width_to_stride(w, lv_display_get_color_format(disp));
    uint32_t buf_size = stride * h;
    dsc->fb = malloc(buf_size);
    LV_ASSERT_MALLOC(dsc->fb);
    if(dsc->fb == NULL) {
        lv_display_delete(disp);
        lv_ll_remove(&window->textures, dsc);
        lv_free(dsc);
        return NULL;
    }
    lv_display_set_buffers(disp, dsc->fb, NULL, buf_size, LV_DISPLAY_RENDER_MODE_DIRECT);
#endif

    if(window->use_indev) {
        lv_indev_t * indev = lv_indev_create();
        if(indev == NULL) {
            lv_display_delete(disp);
            lv_ll_remove(&window->textures, dsc);
            lv_free(dsc);
            return NULL;
        }
        dsc->indev = indev;
        lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
        lv_indev_set_read_cb(indev, indev_read_cb);
        lv_indev_set_driver_data(indev, dsc);
        lv_indev_set_mode(indev, LV_INDEV_MODE_EVENT);
        lv_indev_set_display(indev, disp);
    }

    lv_display_set_driver_data(disp, dsc);
    /* 这个后端的刷新主循环由 GLFW 这边统一驱动：
     * `window_update_handler()` 会负责轮询事件、触发 LVGL 刷新、执行合成并交换前后缓冲。
     * 所以这里要把 display 自带的默认 refresh timer 去掉，避免同一个 display 被两个时钟源同时驱动。 */
    lv_display_delete_refr_timer(disp);
    lv_display_set_flush_cb(disp, window_display_flush_cb);
    lv_display_add_event_cb(disp, window_display_delete_cb, LV_EVENT_DELETE, disp);

#if LV_USE_DRAW_OPENGLES
    window->direct_render_invalidated = 1;
#endif

    return disp;
}

lv_opengles_window_texture_t * lv_opengles_window_display_get_window_texture(lv_display_t * window_display)
{
    return lv_display_get_driver_data(window_display);
}

void lv_opengles_window_texture_set_x(lv_opengles_window_texture_t * texture, int32_t x)
{
    lv_area_set_pos(&texture->area, x, texture->area.y1);

#if LV_USE_DRAW_OPENGLES
    texture->window->direct_render_invalidated = 1;
#endif
}

void lv_opengles_window_texture_set_y(lv_opengles_window_texture_t * texture, int32_t y)
{
    lv_area_set_pos(&texture->area, texture->area.x1, y);

#if LV_USE_DRAW_OPENGLES
    texture->window->direct_render_invalidated = 1;
#endif
}

void lv_opengles_window_texture_set_opa(lv_opengles_window_texture_t * texture, lv_opa_t opa)
{
    texture->opa = opa;

#if LV_USE_DRAW_OPENGLES
    texture->window->direct_render_invalidated = 1;
#endif
}

lv_indev_t * lv_opengles_window_texture_get_mouse_indev(lv_opengles_window_texture_t * texture)
{
    return texture->indev;
}

/**********************
 *   STATIC FUNCTIONS
 **********************/

static int lv_glfw_init(void)
{
    if(glfw_inited) {
        return 0;
    }

    glfwSetErrorCallback(glfw_error_cb);

    int ret = glfwInit();
    if(ret == 0) {
        LV_LOG_ERROR("glfwInit fail.");
        return 1;
    }

    lv_ll_init(&glfw_window_ll, sizeof(lv_opengles_window_t));

    glfw_inited = true;
    return 0;
}

static int lv_glew_init(void)
{
    if(glew_inited) {
        return 0;
    }

    GLenum ret = glewInit();
    if(ret != GLEW_OK) {
        LV_LOG_ERROR("glewInit fail: %d.", ret);
        return ret;
    }

    LV_LOG_INFO("GL version: %s", glGetString(GL_VERSION));
    LV_LOG_INFO("GLSL version: %s", glGetString(GL_SHADING_LANGUAGE_VERSION));

    glew_inited = true;

    return 0;
}

static void lv_glfw_timer_init(void)
{
    if(update_handler_timer == NULL) {
        /* 整个模拟器的主循环都挂在这个 timer 上：
         * 1. 轮询 GLFW 原生事件
         * 2. 刷新 LVGL display
         * 3. 把窗口里的各个 texture/display layer 合成到默认 framebuffer
         * 4. `glfwSwapBuffers` 把这一帧真正显示到桌面窗口 */
        update_handler_timer = lv_timer_create(window_update_handler, LV_DEF_REFR_PERIOD, NULL);
        lv_tick_set_cb(lv_glfw_tick_count_callback);
    }
}

static void lv_glfw_window_config(GLFWwindow * window, bool use_mouse_indev)
{
    glfwMakeContextCurrent(window);

    glfwSwapInterval(1);

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if(use_mouse_indev) {
        glfwSetMouseButtonCallback(window, mouse_button_callback);
        glfwSetCursorPosCallback(window, mouse_move_callback);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwSetWindowCloseCallback(window, window_close_callback);
}

static void lv_glfw_window_quit(void)
{
    lv_timer_delete(update_handler_timer);
    update_handler_timer = NULL;

    glfwTerminate();
    glfw_inited = false;

    lv_deinit();

    exit(0);
}

static void window_update_handler(lv_timer_t * t)
{
    LV_UNUSED(t);

    lv_opengles_window_t * window;

    /* 先把平台层事件取回来，再进入本帧的 LVGL 刷新和输入分发。
     * 这样本帧里读到的鼠标/键盘状态才是最新的。 */
    glfwPollEvents();

    /* 删除已经收到关闭请求的窗口。这里先标记、再在主循环里统一销毁，
     * 可以避免在 GLFW 回调上下文里直接改动窗口链表。 */
    window = lv_ll_get_head(&glfw_window_ll);
    while(window) {
        lv_opengles_window_t * window_to_delete = window->closing ? window : NULL;
        window = lv_ll_get_next(&glfw_window_ll, window);
        if(window_to_delete) {
            glfwSetWindowShouldClose(window_to_delete->window, GLFW_TRUE);
            lv_opengles_window_delete(window_to_delete);
        }
    }

    /* 每个 GLFW 窗口都要单独渲染一遍。
     * 因为每个窗口各自拥有自己的默认 framebuffer，所以进入每个窗口前都要切换当前 GL context。 */
    LV_LL_READ(&glfw_window_ll, window) {
        glfwMakeContextCurrent(window->window);
        lv_opengles_viewport(0, 0, window->hor_res, window->ver_res);

#if LV_USE_DRAW_OPENGLES
        lv_opengles_window_texture_t * textures_head;
        bool window_display_direct_render =
            !window->direct_render_invalidated
            && (textures_head = lv_ll_get_head(&window->textures))
            && textures_head->texture_id == 0 /* 这一层是 window display */
            && lv_ll_get_next(&window->textures, textures_head) == NULL /* 并且它是窗口里唯一的一层 */
            && textures_head->opa == LV_OPA_COVER
            && textures_head->area.x1 == 0
            && textures_head->area.y1 == 0
            && textures_head->area.x2 == window->hor_res - 1
            && textures_head->area.y2 == window->ver_res - 1
            ;
        window->direct_render_invalidated = 0;
        if(!window_display_direct_render) {
            /* 普通路径下，要先清空窗口，再按顺序重画所有图层。
             * 如果命中了 direct render 快路径，就说明当前窗口只有一个全屏、不透明、位置完全覆盖的 window display，
             * 而且这一帧没有触发让直出失效的条件，那么就不需要先 clear 再做逐层合成。 */
            lv_opengles_render_clear();
        }
#else
        lv_opengles_render_clear();
#endif

        /* 开始按链表顺序合成这个窗口中的每一层内容。
         * 每一层可能是：
         * 1. 一个“窗口 display”（texture_id == 0，表示内容直接面向这个窗口）
         * 2. 一个普通的 texture layer（可能由 LVGL 输出，也可能是外部纹理） */
        lv_opengles_window_texture_t * texture;
        LV_LL_READ(&window->textures, texture) {
            if(texture->texture_id == 0) { /* 当前层是 window display，而不是独立的 texture layer */
#if LV_USE_DRAW_OPENGLES
                lv_display_set_render_mode(texture->disp,
                                           window_display_direct_render ? LV_DISPLAY_RENDER_MODE_DIRECT : LV_DISPLAY_RENDER_MODE_FULL);
#endif

                /* LVGL 内部很多刷新辅助函数默认是基于“当前 default display”工作的。
                 * 所以这里在刷新这个子 display 前，先临时把 default display 切到它，
                 * 刷新结束后再恢复原来的 default display。 */
                lv_display_t * default_save = lv_display_get_default();
                lv_display_set_default(texture->disp);
                lv_display_refr_timer(NULL);
                lv_display_set_default(default_save);

#if !LV_USE_DRAW_OPENGLES
                /* 软件绘制路径下，LVGL 先把像素画到 CPU 内存 `texture->fb` 里。
                 * 为了复用同一套 OpenGL 合成逻辑，这里把这块 CPU framebuffer 上传到一个临时 GL 纹理，
                 * 然后再把这个临时纹理当作普通图层绘制进窗口。 */
                ensure_init_window_display_texture();

                GL_CALL(glBindTexture(GL_TEXTURE_2D, window_display_texture));

                /* 重新指定这张临时纹理的尺寸和像素格式，把 CPU framebuffer 上传进去。
                 * 颜色深度支持：8(L8)、16(RGB565)、24(RGB888)、32(XRGB8888)。 */
#if LV_COLOR_DEPTH == 8
                GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, lv_area_get_width(&texture->area), lv_area_get_height(&texture->area), 0,
                                     GL_RED, GL_UNSIGNED_BYTE, texture->fb));
#elif LV_COLOR_DEPTH == 16
                GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB565, lv_area_get_width(&texture->area), lv_area_get_height(&texture->area),
                                     0, GL_RGB, GL_UNSIGNED_SHORT_5_6_5,
                                     texture->fb));
#elif LV_COLOR_DEPTH == 24
                GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, lv_area_get_width(&texture->area), lv_area_get_height(&texture->area), 0,
                                     GL_BGR, GL_UNSIGNED_BYTE, texture->fb));
#elif LV_COLOR_DEPTH == 32
                GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, lv_area_get_width(&texture->area), lv_area_get_height(&texture->area),
                                     0, GL_BGRA, GL_UNSIGNED_BYTE, texture->fb));
#else
#error("Unsupported color format")
#endif

                GL_CALL(glGenerateMipmap(GL_TEXTURE_2D));

                GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));

                lv_opengles_render_texture(window_display_texture, &texture->area, texture->opa, window->hor_res, window->ver_res,
                                           &texture->area, window->h_flip, window->v_flip);
#endif
            }
            else {
                /* 如果当前这个纹理层背后其实对应着一个 LVGL display，
                 * 那么在拿它去合成之前，必须先刷新那个 display，
                 * 否则本帧采样到的还是上一帧甚至更早的纹理内容。 */
                if(texture->disp != NULL) {
#if LV_USE_DRAW_OPENGLES
                    lv_display_t * default_save = lv_display_get_default();
                    lv_display_set_default(texture->disp);
                    lv_display_refr_timer(NULL);
                    lv_display_set_default(default_save);
#else
                    lv_refr_now(texture->disp);
#endif
                }

#if LV_USE_DRAW_OPENGLES
                lv_opengles_render_texture(texture->texture_id, &texture->area, texture->opa, window->hor_res, window->ver_res,
                                           &texture->area, window->h_flip, texture->disp == NULL ? window->v_flip : !window->v_flip);
#else
                lv_opengles_render_texture(texture->texture_id, &texture->area, texture->opa, window->hor_res, window->ver_res,
                                           &texture->area, window->h_flip, window->v_flip);
#endif
            }
        }

        /* 到这里，这个窗口对应的默认 framebuffer 已经包含完整的一帧内容了。
         * `glfwSwapBuffers` 会把后台缓冲切到前台，让桌面窗口真正显示这帧。 */
        glfwSwapBuffers(window->window);
    }
}

static void glfw_error_cb(int error, const char * description)
{
    LV_LOG_ERROR("GLFW Error %d: %s", error, description);
}

static lv_opengles_window_t * lv_glfw_get_lv_window_from_window(GLFWwindow * window)
{
    return glfwGetWindowUserPointer(window);
}

static void window_close_callback(GLFWwindow * window)
{
    lv_opengles_window_t * lv_window = lv_glfw_get_lv_window_from_window(window);
    lv_window->closing = 1;
}

static void key_callback(GLFWwindow * window, int key, int scancode, int action, int mods)
{
    LV_UNUSED(scancode);
    LV_UNUSED(mods);
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        lv_opengles_window_t * lv_window = lv_glfw_get_lv_window_from_window(window);
        lv_window->closing = 1;
    }
}

static void mouse_button_callback(GLFWwindow * window, int button, int action, int mods)
{
    LV_UNUSED(mods);
    if(button == GLFW_MOUSE_BUTTON_LEFT) {
        lv_opengles_window_t * lv_window = lv_glfw_get_lv_window_from_window(window);
        lv_window->mouse_last_state = action == GLFW_PRESS ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
        proc_mouse(lv_window);
    }
}

static void mouse_move_callback(GLFWwindow * window, double xpos, double ypos)
{
    lv_opengles_window_t * lv_window = lv_glfw_get_lv_window_from_window(window);
    lv_window->mouse_last_point.x = (int32_t)xpos;
    lv_window->mouse_last_point.y = (int32_t)ypos;
    proc_mouse(lv_window);
}

static void proc_mouse(lv_opengles_window_t * window)
{
    /* 鼠标命中测试从最上层往下层做。
     * 也就是说，只有最上面那个包含鼠标点位的 LVGL 显示层会收到这次输入。 */
    lv_opengles_window_texture_t * texture;
    LV_LL_READ_BACK(&window->textures, texture) {
        if(lv_area_is_point_on(&texture->area, &window->mouse_last_point, 0)) {
            /* GLFW 给出来的是“相对于整个窗口左上角”的坐标。
             * 但 LVGL indev 读回调需要的是“相对于当前目标 texture/display 左上角”的局部坐标。
             * 所以这里要先减去图层偏移，并结合窗口的水平/垂直翻转选项做一次坐标换算。 */
            if(window->h_flip) {
                texture->indev_last_point.x = texture->area.x2 - window->mouse_last_point.x;
            }
            else {
                texture->indev_last_point.x  = window->mouse_last_point.x - texture->area.x1;
            }
            if(window->v_flip) {
                texture->indev_last_point.y = (texture->area.y2 - window->mouse_last_point.y);
            }
            else {
                texture->indev_last_point.y = (window->mouse_last_point.y - texture->area.y1);
            }
            texture->indev_last_state = window->mouse_last_state;
            lv_indev_read(texture->indev);
            break;
        }
    }
}

static void indev_read_cb(lv_indev_t * indev, lv_indev_data_t * data)
{
    lv_opengles_window_texture_t * texture = lv_indev_get_driver_data(indev);
    data->point = texture->indev_last_point;
    data->state = texture->indev_last_state;
}

static void framebuffer_size_callback(GLFWwindow * window, int width, int height)
{
    lv_opengles_window_t * lv_window = lv_glfw_get_lv_window_from_window(window);
    lv_window->hor_res = width;
    lv_window->ver_res = height;
}

static uint32_t lv_glfw_tick_count_callback(void)
{
    double tick = glfwGetTime() * 1000.0;
    return (uint32_t)tick;
}

static void window_display_delete_cb(lv_event_t * e)
{
    lv_display_t * disp = lv_event_get_target(e);
    lv_opengles_window_texture_t * dsc = lv_display_get_driver_data(disp);
    free(dsc->fb);
    lv_opengles_window_texture_remove(dsc);
}

static void window_display_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map)
{
    LV_UNUSED(area);
    LV_UNUSED(px_map);
    /* 真正把内容显示到窗口上的动作，不在 flush 回调里做，而是在 `window_update_handler()` 统一处理。
     * 这里的 flush 只承担“告诉 LVGL：这一轮 flush 已经结束，可以继续后续渲染流程”的同步作用。 */
    lv_display_flush_ready(disp);
}

#if !LV_USE_DRAW_OPENGLES
static void ensure_init_window_display_texture(void)
{
    if(window_display_texture) {
        return;
    }

    /* 软件绘制模式下，窗口 display 的 CPU framebuffer 每一帧都会重新上传。
     * 因此这里只维护一个可复用的临时纹理对象就够了，不需要为每个 window display 单独长期持有一张纹理。 */
    GL_CALL(glGenTextures(1, &window_display_texture));
    GL_CALL(glBindTexture(GL_TEXTURE_2D, window_display_texture));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
    GL_CALL(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));

    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 20));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST));
    GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));

    GL_CALL(glBindTexture(GL_TEXTURE_2D, 0));
}
#endif

#endif /*LV_USE_GLFW*/
