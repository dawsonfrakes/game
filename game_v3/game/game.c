#include "game.h"

#include <math.h>

#define LENGTH(ARRAY) (sizeof(ARRAY) / sizeof((ARRAY)[0]))
#define JUST_PRESSED(KEY) (input->keys[KEY][FW_CURRENT] == KS_PRESSED && input->keys[KEY][FW_PREVIOUS] == KS_RELEASED)

static V3 camera_pos = {0.0f, 0.0f, 1.0f}, camera_target = {0.0f, 0.0f, -1.0f}, camera_rot = {0.0f, -90.0f, 0.0f};
static M4 projection;
static struct ShaderProgram texture_shader;
static struct VertexArray myvao;
static struct Texture smiletexture;

static const float vertices[] = {
    -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
    0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
    -0.5f, 0.5f, 0.0, 0.0f, 1.0f
};

static const uint32_t indices[] = {
    0, 1, 2, 2, 3, 0
};

static struct BufferAttribute attributes[] = {
    ATTRIB(AT_Float3, "a_position"),
    ATTRIB(AT_Float2, "a_texcoord")
};

static float fov = PI/4.0f;
static void recalculateperspective(const struct WindowState *window)
{
    projection = m4perspective(fov, window->width / (float) window->height, 0.1f, 100.0f);
}

struct SceneData {
    M4 vp;
} current_scene_data;

static void begin_scene(const M4 *vp)
{
    current_scene_data.vp = *vp;
}

static void end_scene(void)
{

}

static void submit_vertex_array(const struct Graphics *gfx, const struct ShaderProgram *shader, const struct VertexArray *va, const M4 *transform)
{
    gfx->shader_bind(shader);
    gfx->shader_set_mat4(shader, "u_vp", m4p(current_scene_data.vp));
    gfx->shader_set_mat4(shader, "u_model", m4pp(transform));

    gfx->vertex_array_bind(va);
    gfx->render_command.draw_indexed(va);
}

GAME_UPDATE(game_update)
{
    if (!memory->initialized) {
        gfx->render_command.clear_color(0.1f, 0.1f, 0.1f, 1.0f);
        input->mode[FW_CURRENT] = IM_CAPTURED;

        texture_shader = gfx->shader("texture");
        smiletexture = gfx->texture("assets/smile_texture.bmp", TT_2D);

        struct BufferLayout layout = gfx->buffer_layout(attributes, LENGTH(attributes));
        struct VertexBuffer vertex_buffer = gfx->vertex_buffer(vertices, sizeof(vertices), &layout);
        struct IndexBuffer index_buffer = gfx->index_buffer(indices, LENGTH(indices));
        myvao = gfx->vertex_array();
        gfx->vertex_array_add_vertex_buffer(&myvao, &vertex_buffer);
        gfx->vertex_array_set_index_buffer(&myvao, &index_buffer);

        memory->initialized = true;
    }

    if (window->just_resized) {
        recalculateperspective(window);
        window->just_resized = false;
    }

    if (JUST_PRESSED(K_ESCAPE)) {
        window->running = false;
    }

    if (JUST_PRESSED(K_V)) {
        input->mode[FW_CURRENT] = (input->mode[FW_CURRENT] + 1) % IM_LAST;
    }

    if (JUST_PRESSED(K_F11)) {
        window->fullscreen[FW_CURRENT] = !window->fullscreen[FW_CURRENT];
    }

    if (input->dz) {
        fov += input->dz * 0.05f;
        fov = CLAMP(fov, PI/4.0f, PI/2.0f);
        recalculateperspective(window);
    }

    if (input->mode[FW_CURRENT] == IM_CAPTURED) {
        const float sens = 0.01f;
        const float xoffset = input->dx * sens;
        const float yoffset = -input->dy * sens;

        camera_rot.y += xoffset;
        camera_rot.x = CLAMP(camera_rot.x + yoffset, -89.0f, 89.0f);

        camera_target = v3normalized(v3(
            cosf(RAD(camera_rot.y)) * cosf(RAD(camera_rot.x)),
            sinf(RAD(camera_rot.x)),
            sinf(RAD(camera_rot.y)) * cosf(RAD(camera_rot.x))
        ));
    }

    const V3 world_up = v3(0.0f, 1.0f, 0.0f);
    const float camera_speed = 5.0f;
    const V3 camera_forward = v3normalized(v3flatten(camera_target));
    const V3 camera_right = v3cross(camera_forward, world_up);
    V3 direction = V3_ZERO;
    if (input->keys[K_W][FW_CURRENT]) {
        direction = v3add(direction, camera_forward);
    }
    if (input->keys[K_S][FW_CURRENT]) {
        direction = v3sub(direction, camera_forward);
    }
    if (input->keys[K_D][FW_CURRENT]) {
        direction = v3add(direction, camera_right);
    }
    if (input->keys[K_A][FW_CURRENT]) {
        direction = v3sub(direction, camera_right);
    }
    if (input->keys[K_E][FW_CURRENT]) {
        direction.y += 1.0f;
    }
    if (input->keys[K_Q][FW_CURRENT]) {
        direction.y -= 1.0f;
    }
    camera_pos = v3add(camera_pos, v3mul(v3normalized(direction), camera_speed * time->delta));

    gfx->render_command.clear(CV_Color);

    const M4 view = m4lookat(camera_pos, v3add(camera_pos, camera_target), world_up);
    const M4 vp = m4mul(projection, view);

    begin_scene(&vp);

    gfx->texture_bind(&smiletexture, 0);
    const M4 transform = m4scale(M4_IDENTITY, v3f(1.5f));
    submit_vertex_array(gfx, &texture_shader, &myvao, &transform);

    end_scene();
}
