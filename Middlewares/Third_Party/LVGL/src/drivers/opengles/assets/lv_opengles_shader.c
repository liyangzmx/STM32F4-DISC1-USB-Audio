#include "lv_opengles_shader.h"

#if LV_USE_OPENGLES

#include "../opengl_shader/lv_opengl_shader_internal.h"
#include "../../../misc/lv_types.h"

/* 这是给 GLSL ES 1.00 版本使用的 include 片段表。
 * 它和 `src_includes_v300es` 的职责相同，都是提供给 shader manager 做伪 `#include`
 * 展开用的源码片段，只不过这里的内容要兼容旧版 GLSL ES 语法。
 *
 * 这些片段目前主要承载“可选颜色调整逻辑”，让主 fragment shader 可以通过宏控制，
 * 按需拼进 HSV、亮度、对比度这类后处理函数。 */
static const lv_opengl_shader_t src_includes_v100[] = {{
        "hsv_adjust.glsl", R"(
        
        uniform float u_Hue;
        uniform float u_Saturation;
        uniform float u_Value;

        // 把 RGB 颜色转换到 HSV 空间，便于分别调节色相/饱和度/明度
        vec3 rgb2hsv(vec3 c) {
            vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
            vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
            vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
            float d = q.x - min(q.w, q.y);
            float e = 1.0e-10;
            return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
        }

        // 把调节后的 HSV 再转回 RGB，供最终输出
        vec3 hsv2rgb(vec3 c) {
            vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
            return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }

        vec3 adjustHSV(vec3 color){
            vec3 hsv = rgb2hsv(color);
            hsv.x = fract(hsv.x + u_Hue);
            hsv.y = clamp(hsv.y * u_Saturation, 0.0, 1.0);
            hsv.z = clamp(hsv.z * u_Value, 0.0, 1.0);
            return hsv2rgb(hsv);
        }
    )"
    }, {
        "brightness_adjust.glsl", R"(
        uniform float u_Brightness; // 亮度偏移量，范围大致是 [ -1.0 .. +1.0 ]，0.0 表示不调整

        vec3 adjustBrightness(vec3 color){
            return clamp(color + vec3(u_Brightness), 0.0, 1.0);
        }

    )"
    }, {
        "contrast_adjust.glsl", R"(
        uniform float u_Contrast; // 0.0 表示压到中灰，1.0 表示不变，大于 1.0 会增强对比度

        vec3 adjustContrast(vec3 color){
            // 先平移到以 0 为中心的区间，缩放后再平移回 [0, 1]
            return clamp(((color - 0.5) * u_Contrast) + 0.5, 0.0, 1.0);
        }
    )"
    },
};

/* GLSL ES 1.00 版本的顶点 shader。
 * 作用很纯粹：读取 CPU 侧传进来的 quad 顶点位置和纹理坐标，
 * 用 `u_VertexTransform` 把顶点从“标准 quad 空间”变换到目标位置，
 * 再把纹理坐标透传给 fragment shader。 */
static const char * src_vertex_shader_v100 = R"(
    precision mediump float;
    
    attribute vec4 position;
    attribute vec2 texCoord;
    
    varying vec2 v_TexCoord;
    
    uniform mat3 u_VertexTransform;
    
    void main()
    {
        gl_Position = vec4((u_VertexTransform * vec3(position.xy, 1.0)).xy, position.zw);
        v_TexCoord = texCoord;
    }
)";

/* GLSL ES 1.00 版本的片元 shader。
 * 这一版负责：
 * 1. 根据 `u_IsFill` 决定当前 draw 是纯色填充还是纹理采样
 * 2. 根据颜色深度决定输出灰度还是 RGBA
 * 3. 把 LVGL 传入的整体不透明度 `u_Opa` 乘到结果上
 * 4. 在启用宏时，执行 HSV 等可选颜色调整 */
static const char *src_fragment_shader_v100 = R"(
    precision lowp float;
    
    varying vec2 v_TexCoord;
    
    uniform sampler2D u_Texture;
    uniform float u_ColorDepth;
    uniform float u_Opa;
    uniform bool u_IsFill;
    uniform vec3 u_FillColor;
    
    #ifdef HSV_ADJUST
#include <hsv_adjust.glsl>
    #endif
    
    void main()
    {
        vec4 texColor;
        if (u_IsFill) {
            texColor = vec4(u_FillColor, 1.0);
        } else {
            texColor = texture2D(u_Texture, v_TexCoord);
        }
        if (abs(u_ColorDepth - 8.0) < 0.1) {
            float gray = texColor.r;
            gl_FragColor = vec4(gray, gray, gray, u_Opa);
        } else {
            float combinedAlpha = texColor.a * u_Opa;
            gl_FragColor = vec4(texColor.rgb * combinedAlpha, combinedAlpha);
        }
        #ifdef HSV_ADJUST
        gl_FragColor.rgb = adjustHSV(gl_FragColor.rgb);
        #endif
    }
)";

/* 这张表不是直接给 OpenGL 编译器读取的“完整 shader 源码”，
 * 而是提供给 `lv_opengl_shader_manager_process_includes()` 用来做源码预处理的“片段库”。
 *
 * 对于 GLSL 300 ES 版本，下面这些片段会在遇到
 *   #include <xxx.glsl>
 * 这种伪 include 语法时，被替换插入到最终 shader 文本里。
 *
 * 这样做有两个好处：
 * 1. 可以把 HSV、亮度、对比度这类可选效果拆成独立的小模块，避免主 shader 太臃肿。
 * 2. v100 和 v300es 可以各自维护一份兼容当前语法版本的 include 片段，
 *    例如 attribute/in、varying/out、texture2D/textureLod 这些差异都可以按版本分开处理。
 *
 * 所以 `src_includes_v300es` 可以理解为：
 * “专门给 GLSL ES 3.00 shader 使用的 include 片段映射表”。 */
static const lv_opengl_shader_t src_includes_v300es[] = {{
        "hsv_adjust.glsl", R"(
        uniform float u_Hue;
        uniform float u_Saturation;
        uniform float u_Value;

        // 把 RGB 颜色转换到 HSV 空间，便于分别调节色相/饱和度/明度
        vec3 rgb2hsv(vec3 c) {
            vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
            vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g));
            vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
            float d = q.x - min(q.w, q.y);
            float e = 1.0e-10;
            return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x);
        }

        // 把调节后的 HSV 再转回 RGB，供最终输出
        vec3 hsv2rgb(vec3 c) {
            vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
            vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
            return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
        }

        vec3 adjustHSV(vec3 color){
            vec3 hsv = rgb2hsv(color);
            hsv.x = fract(hsv.x + u_Hue);
            hsv.y = clamp(hsv.y * u_Saturation, 0.0, 1.0);
            hsv.z = clamp(hsv.z * u_Value, 0.0, 1.0);
            return hsv2rgb(hsv);
        }
    )"
    }, {
        "brightness_adjust.glsl", R"(
        uniform float u_Brightness; // 亮度偏移量，范围大致是 [ -1.0 .. +1.0 ]，0.0 表示不调整

        vec3 adjustBrightness(vec3 color){
            return clamp(color + vec3(u_Brightness), 0.0, 1.0);
        }

    )"
    }, {
        "contrast_adjust.glsl", R"(
        uniform float u_Contrast; // 0.0 表示压到中灰，1.0 表示不变，大于 1.0 会增强对比度

        vec3 adjustContrast(vec3 color){
            // 先平移到以 0 为中心的区间，缩放后再平移回 [0, 1]
            return clamp(((color - 0.5) * u_Contrast) + 0.5, 0.0, 1.0);
        }
    )"
    },
};

/* GLSL ES 3.00 版本的顶点 shader。
 * 和 v100 的职责一样，主要差别在于语法升级成了 `in/out`，
 * 以适配较新的 GLSL ES 语义。 */
static const char * src_vertex_shader_v300es = R"(
    precision mediump float;
    
    in vec4 position;
    in vec2 texCoord;
    
    out vec2 v_TexCoord;
    
    uniform mat3 u_VertexTransform;
    
    void main()
    {
        gl_Position = vec4((u_VertexTransform * vec3(position.xy, 1)).xy, position.zw);
        v_TexCoord = texCoord;
    }
)";

/* GLSL ES 3.00 版本的片元 shader。
 * 整体逻辑与 v100 基本一致，但语法和采样方式做了适配：
 * 1. 输出变量从 `gl_FragColor` 改成显式的 `out vec4 color`
 * 2. 纹理采样这里使用 `textureLod(..., 0.0)`，是为了在某些旋转场景下，
 *    避免导数方向变化导致的采样/LOD 计算问题
 * 3. 同样支持 fill、opacity、颜色深度判断，以及可选的 HSV 调整 */
static const char *src_fragment_shader_v300es = R"(
    precision lowp float;
    
    out vec4 color;
    
    in vec2 v_TexCoord;
    
    uniform sampler2D u_Texture;
    uniform float u_ColorDepth;
    uniform float u_Opa;
    uniform bool u_IsFill;
    uniform vec3 u_FillColor;
    
    #ifdef HSV_ADJUST
#include <hsv_adjust.glsl>
    #endif

    void main()
    {
        vec4 texColor;
        if (u_IsFill) {
            texColor = vec4(u_FillColor, 1.0);
        } else {
            // texColor = texture(u_Texture, v_TexCoord);
            texColor = textureLod(u_Texture, v_TexCoord, 0.0);  // 顶点发生变换、且纹理未生成 mipmap 时，某些旋转角度（尤其 90/270 度）可能导致导数方向翻转；这里固定 LOD 0 来规避这类采样问题
        }
        if (abs(u_ColorDepth - 8.0) < 0.1) {
            float gray = texColor.r;
            color = vec4(gray, gray, gray, u_Opa);
        } else {
            float combinedAlpha = texColor.a * u_Opa;
            color = vec4(texColor.rgb * combinedAlpha, combinedAlpha);
        }
        #ifdef HSV_ADJUST
        color.rgb = adjustHSV(color.rgb);
        #endif
    }
)";

static const size_t src_includes_v100_count = sizeof src_includes_v100 / sizeof src_includes_v100[0];
static const size_t src_includes_v300es_count = sizeof src_includes_v300es / sizeof src_includes_v300es[0];

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/* 根据 GLSL 版本返回“展开 include 之后”的顶点 shader 完整源码。 */
char * lv_opengles_shader_get_vertex(lv_opengl_glsl_version version) {
    switch (version){
        case LV_OPENGL_GLSL_VERSION_300ES:
            return lv_opengl_shader_manager_process_includes(src_vertex_shader_v300es, src_includes_v300es, src_includes_v300es_count);
        case LV_OPENGL_GLSL_VERSION_100:
            return lv_opengl_shader_manager_process_includes(src_vertex_shader_v100, src_includes_v100, src_includes_v100_count);
        case LV_OPENGL_GLSL_VERSION_LAST:
            LV_LOG_ERROR("Invalid glsl version %d", version);
            return NULL;
    }
    LV_UNREACHABLE();
}

/* 根据 GLSL 版本返回“展开 include 之后”的片元 shader 完整源码。 */
char * lv_opengles_shader_get_fragment(lv_opengl_glsl_version version) {
    switch (version){
        case LV_OPENGL_GLSL_VERSION_300ES:
            return lv_opengl_shader_manager_process_includes(src_fragment_shader_v300es, src_includes_v300es, src_includes_v300es_count);
        case LV_OPENGL_GLSL_VERSION_100:
            return lv_opengl_shader_manager_process_includes(src_fragment_shader_v100, src_includes_v100, src_includes_v100_count);
        case LV_OPENGL_GLSL_VERSION_LAST:
            LV_LOG_ERROR("Invalid glsl version %d", version);
            return NULL;
    }
    LV_UNREACHABLE();
}

/* 把当前版本对应的 include 片段表返回给上层。
 * 上层 shader manager 在做宏选择、program 组装时，会直接用到这些片段信息。 */
void lv_opengles_shader_get_source(lv_opengl_shader_portions_t *portions, lv_opengl_glsl_version version)
{
    switch (version){
        case LV_OPENGL_GLSL_VERSION_300ES:
            portions->all = src_includes_v300es;
            portions->count = src_includes_v300es_count;
            return;
        case LV_OPENGL_GLSL_VERSION_100:
            portions->all = src_includes_v100;
            portions->count = src_includes_v100_count;
            return;
        case LV_OPENGL_GLSL_VERSION_LAST:
            LV_LOG_ERROR("Invalid glsl version %d", version);
            portions->count = 0;
            return;
    }

    LV_UNREACHABLE();
}

#endif /*LV_USE_OPENGLES*/
