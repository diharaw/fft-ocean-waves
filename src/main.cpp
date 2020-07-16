#define _USE_MATH_DEFINES
#include <ogl.h>
#include <application.h>
#include <mesh.h>
#include <camera.h>
#include <material.h>
#include <memory>
#include <iostream>
#include <stack>
#include <random>
#include <chrono>
#include <random>

#undef min
#define CAMERA_FAR_PLANE 1000.0f
#define DEBUG_CAMERA_FAR_PLANE 10000.0f
#define DISPLACEMENT_MAP_SIZE 256
#define COMPUTE_LOCAL_WORK_GROUP_SIZE 32
#define GRID_SIZE 1024
#define GRID_UV_SCALE 64

struct GlobalUniforms
{
    DW_ALIGNED(16)
    glm::mat4 view_proj;
};

const unsigned char g_bit_rev_table[] = {
    0x00, 0x80, 0x40, 0xC0, 0x20, 0xA0, 0x60, 0xE0, 0x10, 0x90, 0x50, 0xD0, 0x30, 0xB0, 0x70, 0xF0, 0x08, 0x88, 0x48, 0xC8, 0x28, 0xA8, 0x68, 0xE8, 0x18, 0x98, 0x58, 0xD8, 0x38, 0xB8, 0x78, 0xF8, 0x04, 0x84, 0x44, 0xC4, 0x24, 0xA4, 0x64, 0xE4, 0x14, 0x94, 0x54, 0xD4, 0x34, 0xB4, 0x74, 0xF4, 0x0C, 0x8C, 0x4C, 0xCC, 0x2C, 0xAC, 0x6C, 0xEC, 0x1C, 0x9C, 0x5C, 0xDC, 0x3C, 0xBC, 0x7C, 0xFC, 0x02, 0x82, 0x42, 0xC2, 0x22, 0xA2, 0x62, 0xE2, 0x12, 0x92, 0x52, 0xD2, 0x32, 0xB2, 0x72, 0xF2, 0x0A, 0x8A, 0x4A, 0xCA, 0x2A, 0xAA, 0x6A, 0xEA, 0x1A, 0x9A, 0x5A, 0xDA, 0x3A, 0xBA, 0x7A, 0xFA, 0x06, 0x86, 0x46, 0xC6, 0x26, 0xA6, 0x66, 0xE6, 0x16, 0x96, 0x56, 0xD6, 0x36, 0xB6, 0x76, 0xF6, 0x0E, 0x8E, 0x4E, 0xCE, 0x2E, 0xAE, 0x6E, 0xEE, 0x1E, 0x9E, 0x5E, 0xDE, 0x3E, 0xBE, 0x7E, 0xFE, 0x01, 0x81, 0x41, 0xC1, 0x21, 0xA1, 0x61, 0xE1, 0x11, 0x91, 0x51, 0xD1, 0x31, 0xB1, 0x71, 0xF1, 0x09, 0x89, 0x49, 0xC9, 0x29, 0xA9, 0x69, 0xE9, 0x19, 0x99, 0x59, 0xD9, 0x39, 0xB9, 0x79, 0xF9, 0x05, 0x85, 0x45, 0xC5, 0x25, 0xA5, 0x65, 0xE5, 0x15, 0x95, 0x55, 0xD5, 0x35, 0xB5, 0x75, 0xF5, 0x0D, 0x8D, 0x4D, 0xCD, 0x2D, 0xAD, 0x6D, 0xED, 0x1D, 0x9D, 0x5D, 0xDD, 0x3D, 0xBD, 0x7D, 0xFD, 0x03, 0x83, 0x43, 0xC3, 0x23, 0xA3, 0x63, 0xE3, 0x13, 0x93, 0x53, 0xD3, 0x33, 0xB3, 0x73, 0xF3, 0x0B, 0x8B, 0x4B, 0xCB, 0x2B, 0xAB, 0x6B, 0xEB, 0x1B, 0x9B, 0x5B, 0xDB, 0x3B, 0xBB, 0x7B, 0xFB, 0x07, 0x87, 0x47, 0xC7, 0x27, 0xA7, 0x67, 0xE7, 0x17, 0x97, 0x57, 0xD7, 0x37, 0xB7, 0x77, 0xF7, 0x0F, 0x8F, 0x4F, 0xCF, 0x2F, 0xAF, 0x6F, 0xEF, 0x1F, 0x9F, 0x5F, 0xDF, 0x3F, 0xBF, 0x7F, 0xFF
};

const unsigned short g_bit_rev_masks[17] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0X0100, 0X0300, 0X0700, 0X0F00, 0X1F00, 0X3F00, 0X7F00, 0XFF00 };

unsigned long reverse_bits(unsigned long codeword, unsigned char maxLength)
{
    if (maxLength <= 8)
    {
        codeword = g_bit_rev_table[codeword << (8 - maxLength)];
    }
    else
    {
        unsigned char sc = maxLength - 8; // shift count in bits and index into masks array

        if (maxLength <= 16)
        {
            codeword = (g_bit_rev_table[codeword & 0X00FF] << sc)
                | g_bit_rev_table[codeword >> sc];
        }
        else if (maxLength & 1) // if maxLength is 17, 19, 21, or 23
        {
            codeword = (g_bit_rev_table[codeword & 0X00FF] << sc)
                | g_bit_rev_table[codeword >> sc] | (g_bit_rev_table[(codeword & g_bit_rev_masks[sc]) >> (sc - 8)] << 8);
        }
        else // if maxlength is 18, 20, 22, or 24
        {
            codeword = (g_bit_rev_table[codeword & 0X00FF] << sc)
                | g_bit_rev_table[codeword >> sc]
                | (g_bit_rev_table[(codeword & g_bit_rev_masks[sc]) >> (sc >> 1)] << (sc >> 1));
        }
    }

    return codeword;
}

struct GridVertex
{
    glm::vec3 position;
    glm::vec2 texcoord;
};

class FFTOceanWaves : public dw::Application
{
protected:
    // -----------------------------------------------------------------------------------------------------------------------------------

    bool init(int argc, const char* argv[]) override
    {
        // Create GPU resources.
        if (!create_shaders())
            return false;

        create_ocean_textures();

        if (!create_uniform_buffer())
            return false;

        // Create camera.
        create_camera();

        // FFT precomputations
        tilde_h0_k();
        generate_bit_reversed_indices();
        generate_twiddle_factors();
        generate_grid(GRID_SIZE, GRID_UV_SCALE);

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update(double delta) override
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        debug_gui();

        // Update camera.
        update_camera();

        update_uniforms();

        tilde_h0_t();
        butterfly_fft(m_tilde_h0_t_dy, m_dy);
        butterfly_fft(m_tilde_h0_t_dx, m_dx);
        butterfly_fft(m_tilde_h0_t_dz, m_dz);
        generate_normal_map();

        render_grid_wireframe_tessellated();
        render_visualization_quad(m_dy);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void window_resized(int width, int height) override
    {
        // Override window resized method to update camera projection.
        m_main_camera->update_projection(60.0f, 0.1f, CAMERA_FAR_PLANE, float(m_width) / float(m_height));
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void key_pressed(int code) override
    {
        // Handle forward movement.
        if (code == GLFW_KEY_W)
            m_heading_speed = m_camera_speed;
        else if (code == GLFW_KEY_S)
            m_heading_speed = -m_camera_speed;

        // Handle sideways movement.
        if (code == GLFW_KEY_A)
            m_sideways_speed = -m_camera_speed;
        else if (code == GLFW_KEY_D)
            m_sideways_speed = m_camera_speed;

        if (code == GLFW_KEY_SPACE)
            m_mouse_look = true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void key_released(int code) override
    {
        // Handle forward movement.
        if (code == GLFW_KEY_W || code == GLFW_KEY_S)
            m_heading_speed = 0.0f;

        // Handle sideways movement.
        if (code == GLFW_KEY_A || code == GLFW_KEY_D)
            m_sideways_speed = 0.0f;

        if (code == GLFW_KEY_SPACE)
            m_mouse_look = false;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void mouse_pressed(int code) override
    {
        // Enable mouse look.
        if (code == GLFW_MOUSE_BUTTON_RIGHT)
            m_mouse_look = true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void mouse_released(int code) override
    {
        // Disable mouse look.
        if (code == GLFW_MOUSE_BUTTON_RIGHT)
            m_mouse_look = false;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

protected:
    // -----------------------------------------------------------------------------------------------------------------------------------

    dw::AppSettings intial_app_settings() override
    {
        dw::AppSettings settings;

        settings.resizable             = true;
        settings.maximized             = false;
        settings.refresh_rate          = 60;
        settings.major_ver             = 4;
        settings.width                 = 1920;
        settings.height                = 1080;
        settings.title                 = "FFT Ocean Waves (c) 2020 Dihara Wijetunga";
        settings.enable_debug_callback = false;

        return settings;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:

    // -----------------------------------------------------------------------------------------------------------------------------------

    void debug_gui()
    {
        ImGui::InputFloat("Displacement Scale", &m_displacement_scale);
        ImGui::InputFloat("Choppiness", &m_choppiness);
        ImGui::InputFloat("Wind Speed (m/s)", &m_wind_speed);
        ImGui::InputFloat("Amplitude", &m_amplitude);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void render_visualization_quad(std::unique_ptr<dw::gl::Texture2D>& texture, int w = 0, int h = 0)
    {
        glViewport(0, 0, w == 0 ? texture->width() : w, h == 0 ? texture->height() : h);

        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        // Bind shader program.
        m_visualize_image_program->use();

        if (m_visualize_image_program->set_uniform("s_Image", 0))
            texture->bind(0);

        if (texture->format() == GL_RED)
            m_visualize_image_program->set_uniform("u_NumChannels", 1);
        else if (texture->format() == GL_RG)
            m_visualize_image_program->set_uniform("u_NumChannels", 2);
        else if (texture->format() == GL_RGB)
            m_visualize_image_program->set_uniform("u_NumChannels", 3);
        else if (texture->format() == GL_RGBA)
            m_visualize_image_program->set_uniform("u_NumChannels", 4);

        // Render fullscreen triangle
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void render_grid_wireframe()
    {
        glViewport(0, 0, m_width, m_height);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        m_ocean_wireframe_no_tess_program->use();
        m_global_ubo->bind_base(0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        m_grid_vao->bind();

        glDrawElements(GL_TRIANGLES, m_grid_num_indices, GL_UNSIGNED_INT, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void render_grid_wireframe_tessellated()
    {
        glViewport(0, 0, m_width, m_height);

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glDisable(GL_BLEND);

        m_ocean_wireframe_program->use();
        m_global_ubo->bind_base(0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

        m_ocean_wireframe_program->set_uniform("u_CameraPos", m_main_camera->m_position);
        m_ocean_wireframe_program->set_uniform("u_DisplacementScale", m_displacement_scale);
        m_ocean_wireframe_program->set_uniform("u_Choppiness", m_choppiness);

        if (m_ocean_wireframe_program->set_uniform("s_Dy", 0))
            m_dy->bind(0);

        if (m_ocean_wireframe_program->set_uniform("s_Dx", 1))
            m_dx->bind(1);

        if (m_ocean_wireframe_program->set_uniform("s_Dz", 2))
            m_dz->bind(2);

        if (m_ocean_wireframe_program->set_uniform("s_NormalMap", 3))
            m_normal_map->bind(3);

        m_grid_vao->bind();

        glPatchParameteri(GL_PATCH_VERTICES, 3);
        glDrawElements(GL_PATCHES, m_grid_num_indices, GL_UNSIGNED_INT, 0);

        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void tilde_h0_k()
    {
        m_wind_direction = glm::normalize(m_wind_direction);

        m_tilde_h0_k_program->use();

        if (m_tilde_h0_k_program->set_uniform("noise0", 2))
            m_noise0->bind(2);
        if (m_tilde_h0_k_program->set_uniform("noise1", 3))
            m_noise1->bind(3);
        if (m_tilde_h0_k_program->set_uniform("noise2", 4))
            m_noise2->bind(4);
        if (m_tilde_h0_k_program->set_uniform("noise3", 5))
            m_noise3->bind(5);

        m_tilde_h0_k_program->set_uniform("u_Amplitude", m_amplitude);
        m_tilde_h0_k_program->set_uniform("u_WindSpeed", m_wind_speed);
        m_tilde_h0_k_program->set_uniform("u_WindDirection", m_wind_direction);
        m_tilde_h0_k_program->set_uniform("u_SuppressFactor", m_suppression_factor);
        m_tilde_h0_k_program->set_uniform("u_N", m_N);
        m_tilde_h0_k_program->set_uniform("u_L", m_L);

        m_tilde_h0_k->bind_image(0, 0, 0, GL_WRITE_ONLY, m_tilde_h0_k->internal_format());
        m_tilde_h0_minus_k->bind_image(1, 0, 0, GL_WRITE_ONLY, m_tilde_h0_minus_k->internal_format());

        uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

        glDispatchCompute(num_groups, num_groups, 1);

        glFinish();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void tilde_h0_t()
    {
        m_wind_direction = glm::normalize(m_wind_direction);

        m_tilde_h0_t_program->use();

        m_tilde_h0_k->bind_image(0, 0, 0, GL_READ_ONLY, m_tilde_h0_k->internal_format());
        m_tilde_h0_minus_k->bind_image(1, 0, 0, GL_READ_ONLY, m_tilde_h0_minus_k->internal_format());
        m_tilde_h0_t_dx->bind_image(2, 0, 0, GL_WRITE_ONLY, m_tilde_h0_t_dx->internal_format());
        m_tilde_h0_t_dy->bind_image(3, 0, 0, GL_WRITE_ONLY, m_tilde_h0_t_dy->internal_format());
        m_tilde_h0_t_dz->bind_image(4, 0, 0, GL_WRITE_ONLY, m_tilde_h0_t_dz->internal_format());

        m_tilde_h0_t_program->set_uniform("u_Time", float(glfwGetTime()));
        m_tilde_h0_t_program->set_uniform("u_N", m_N);
        m_tilde_h0_t_program->set_uniform("u_L", m_L);

        uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

        glDispatchCompute(num_groups, num_groups, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void butterfly_fft(std::unique_ptr<dw::gl::Texture2D>& tilde_h0_t, std::unique_ptr<dw::gl::Texture2D>& dst)
    {
        m_butterfly_program->use();

        m_twiddle_factors->bind_image(0, 0, 0, GL_READ_ONLY, m_twiddle_factors->internal_format());
        tilde_h0_t->bind_image(1, 0, 0, GL_READ_WRITE, tilde_h0_t->internal_format());
        m_ping_pong->bind_image(2, 0, 0, GL_READ_WRITE, m_ping_pong->internal_format());

        int log_2_n  = int(log(m_N) / log(2));
        int pingpong = 0;

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "1D FFT Horizontal");

        // 1D FFT horizontal
        for (int i = 0; i < log_2_n; i++)
        {
            m_butterfly_program->set_uniform("u_PingPong", pingpong);
            m_butterfly_program->set_uniform("u_Direction", 0);
            m_butterfly_program->set_uniform("u_Stage", i);

            uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

            glDispatchCompute(num_groups, num_groups, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            pingpong++;
            pingpong %= 2;
        }

        glPopDebugGroup();

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "1D FFT Vertical");

        // 1D FFT vertical
        for (int i = 0; i < log_2_n; i++)
        {
            m_butterfly_program->set_uniform("u_PingPong", pingpong);
            m_butterfly_program->set_uniform("u_Direction", 1);
            m_butterfly_program->set_uniform("u_Stage", i);

            uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

            glDispatchCompute(num_groups, num_groups, 1);

            glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

            pingpong++;
            pingpong %= 2;
        }

        glPopDebugGroup();

        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "FFT Inversion");

        m_inversion_program->use();

        m_inversion_program->set_uniform("u_PingPong", pingpong);
        m_inversion_program->set_uniform("u_N", m_N);

        dst->bind_image(0, 0, 0, GL_WRITE_ONLY, dst->internal_format());
        tilde_h0_t->bind_image(1, 0, 0, GL_READ_ONLY, tilde_h0_t->internal_format());
        m_ping_pong->bind_image(2, 0, 0, GL_READ_ONLY, m_ping_pong->internal_format());

        uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

        glDispatchCompute(num_groups, num_groups, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glPopDebugGroup();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void generate_normal_map()
    {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0, -1, "Generate Normal Map");

        m_normal_map_program->use();

        m_normal_map->bind_image(0, 0, 0, GL_WRITE_ONLY, m_normal_map->internal_format());

        if (m_normal_map_program->set_uniform("s_HeightMap", 0))
            m_dy->bind(0);

        m_normal_map_program->set_uniform("u_N", m_N);

        uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

        glDispatchCompute(num_groups, num_groups, 1);

        glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

        glPopDebugGroup();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void generate_twiddle_factors()
    {
        m_twiddle_factors_program->use();

        uint32_t log_2_n    = int(log(m_N) / log(2));
        uint32_t num_groups = m_N / COMPUTE_LOCAL_WORK_GROUP_SIZE;

        m_twiddle_factors->bind_image(0, 0, 0, GL_WRITE_ONLY, m_twiddle_factors->internal_format());

        m_bit_reversed_indices->bind_base(0);

        m_twiddle_factors_program->set_uniform("u_N", m_N);
        m_twiddle_factors_program->set_uniform("u_DisplacementScale", m_N);
        m_twiddle_factors_program->set_uniform("u_Choppiness", m_N);

        glDispatchCompute(log_2_n, num_groups, 1);

        glFinish();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void generate_bit_reversed_indices()
    {
        std::vector<int32_t> indices;
        indices.resize(m_N);

        int32_t num_bits = int(log(m_N) / log(2));

        for (int i = 0; i < m_N; i++)
            indices[i] = reverse_bits(i, num_bits);

        m_bit_reversed_indices = std::unique_ptr<dw::gl::ShaderStorageBuffer>(new dw::gl::ShaderStorageBuffer(GL_STATIC_DRAW, sizeof(int32_t) * m_N, indices.data()));
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void generate_grid(const int32_t& num_squares_per_side, const float& texcoord_scale)
    {
        std::vector<GridVertex> vertices;
        std::vector<uint32_t>   indices;
        const int32_t           n       = num_squares_per_side;
        const int32_t           half_n  = n / 2;
        const int32_t           n_verts = n + 1;

        vertices.reserve(n_verts * n_verts);
        indices.reserve(n_verts * n_verts * 3);

        // Generate vertices
        for (int32_t y = -half_n; y <= half_n; y++)
        {
            for (int32_t x = -half_n; x <= half_n; x++)
            {
                GridVertex vert;

                vert.position = glm::vec3(float(x), 0.0f, float(y));

                float u = float(x + half_n) / float(n);
                float v = float(y + half_n) / float(n);

                vert.texcoord = glm::vec2(u * texcoord_scale, v * texcoord_scale);

                vertices.push_back(vert);
            }
        }

        // Generate indices
        for (int32_t y = 0; y < n; y++)
        {
            for (int32_t x = 0; x < n; x++)
            {
                uint32_t i0 = (n_verts * y) + x;
                uint32_t i1 = (n_verts * (y + 1)) + x;
                uint32_t i2 = i0 + 1;

                indices.push_back(i0);
                indices.push_back(i1);
                indices.push_back(i2);

                uint32_t i3 = i2;
                uint32_t i4 = i1;
                uint32_t i5 = i1 + 1;

                indices.push_back(i3);
                indices.push_back(i4);
                indices.push_back(i5);
            }
        }

        m_grid_num_vertices = vertices.size();
        m_grid_num_indices  = indices.size();

        // Create vertex buffer.
        m_grid_vbo = std::make_unique<dw::gl::VertexBuffer>(GL_STATIC_DRAW, sizeof(GridVertex) * vertices.size(), vertices.data());

        if (!m_grid_vbo)
            DW_LOG_ERROR("Failed to create Vertex Buffer");

        // Create index buffer.
        m_grid_ibo = std::make_unique<dw::gl::IndexBuffer>(GL_STATIC_DRAW, sizeof(uint32_t) * indices.size(), indices.data());

        if (!m_grid_ibo)
            DW_LOG_ERROR("Failed to create Index Buffer");

        // Declare vertex attributes.
        dw::gl::VertexAttrib attribs[] = { { 3, GL_FLOAT, false, 0 },
                                           { 2, GL_FLOAT, false, offsetof(GridVertex, texcoord) } };

        // Create vertex array.
        m_grid_vao = std::make_unique<dw::gl::VertexArray>(m_grid_vbo.get(), m_grid_ibo.get(), sizeof(GridVertex), 2, attribs);

        if (!m_grid_vao)
            DW_LOG_ERROR("Failed to create Vertex Array");
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    bool create_shaders()
    {
        {
            // Create general shaders
            m_triangle_vs        = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_VERTEX_SHADER, "shader/triangle_vs.glsl"));
            m_visualize_image_fs = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_FRAGMENT_SHADER, "shader/visualize_image_fs.glsl"));
            m_tilde_h0_k_cs      = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/tilde_h0_k_cs.glsl"));
            m_tilde_h0_t_cs      = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/tilde_h0_t_cs.glsl"));
            m_twiddle_factors_cs = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/twiddle_factors_cs.glsl"));
            m_butterfly_cs       = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/butterfly_cs.glsl"));
            m_inversion_cs       = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/inversion_cs.glsl"));
            m_normal_map_cs      = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/normal_map_cs.glsl"));
            m_grid_vs            = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_VERTEX_SHADER, "shader/grid_vs.glsl"));
            m_grid_non_tess_vs   = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_VERTEX_SHADER, "shader/grid_non_tess_vs.glsl"));
            m_grid_tcs           = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_TESS_CONTROL_SHADER, "shader/grid_tcs.glsl"));
            m_grid_tes           = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_TESS_EVALUATION_SHADER, "shader/grid_tes.glsl"));
            m_ocean_wireframe_fs = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_FRAGMENT_SHADER, "shader/wireframe_fs.glsl"));

            {
                if (!m_triangle_vs || !m_visualize_image_fs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_triangle_vs.get(), m_visualize_image_fs.get() };
                m_visualize_image_program = std::make_unique<dw::gl::Program>(2, shaders);

                if (!m_visualize_image_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_tilde_h0_k_cs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_tilde_h0_k_cs.get() };
                m_tilde_h0_k_program      = std::make_unique<dw::gl::Program>(1, shaders);

                if (!m_tilde_h0_k_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_tilde_h0_t_cs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_tilde_h0_t_cs.get() };
                m_tilde_h0_t_program      = std::make_unique<dw::gl::Program>(1, shaders);

                if (!m_tilde_h0_t_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_twiddle_factors_cs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_twiddle_factors_cs.get() };
                m_twiddle_factors_program = std::make_unique<dw::gl::Program>(1, shaders);

                if (!m_twiddle_factors_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_butterfly_cs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_butterfly_cs.get() };
                m_butterfly_program       = std::make_unique<dw::gl::Program>(1, shaders);

                if (!m_butterfly_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_inversion_cs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_inversion_cs.get() };
                m_inversion_program       = std::make_unique<dw::gl::Program>(1, shaders);

                if (!m_inversion_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_normal_map_cs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_normal_map_cs.get() };
                m_normal_map_program      = std::make_unique<dw::gl::Program>(1, shaders);

                if (!m_normal_map_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }
            }

            {
                if (!m_grid_non_tess_vs || !m_ocean_wireframe_fs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[]         = { m_grid_non_tess_vs.get(), m_ocean_wireframe_fs.get() };
                m_ocean_wireframe_no_tess_program = std::make_unique<dw::gl::Program>(2, shaders);

                if (!m_ocean_wireframe_no_tess_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }

                m_ocean_wireframe_no_tess_program->uniform_block_binding("GlobalUniforms", 0);
            }

            {
                if (!m_grid_vs || !m_grid_tcs || !m_grid_tes || !m_ocean_wireframe_fs)
                {
                    DW_LOG_FATAL("Failed to create Shaders");
                    return false;
                }

                // Create general shader program
                dw::gl::Shader* shaders[] = { m_grid_vs.get(), m_grid_tcs.get(), m_grid_tes.get(), m_ocean_wireframe_fs.get() };
                m_ocean_wireframe_program = std::make_unique<dw::gl::Program>(4, shaders);

                if (!m_ocean_wireframe_program)
                {
                    DW_LOG_FATAL("Failed to create Shader Program");
                    return false;
                }

                m_ocean_wireframe_program->uniform_block_binding("GlobalUniforms", 0);
            }
        }

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void create_ocean_textures()
    {
        m_noise0           = std::unique_ptr<dw::gl::Texture2D>(dw::gl::Texture2D::create_from_files("noise/LDR_LLL1_0.png", false, false));
        m_noise1           = std::unique_ptr<dw::gl::Texture2D>(dw::gl::Texture2D::create_from_files("noise/LDR_LLL1_1.png", false, false));
        m_noise2           = std::unique_ptr<dw::gl::Texture2D>(dw::gl::Texture2D::create_from_files("noise/LDR_LLL1_2.png", false, false));
        m_noise3           = std::unique_ptr<dw::gl::Texture2D>(dw::gl::Texture2D::create_from_files("noise/LDR_LLL1_3.png", false, false));
        m_tilde_h0_k       = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RG32F, GL_RG, GL_FLOAT);
        m_tilde_h0_minus_k = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RG32F, GL_RG, GL_FLOAT);
        m_tilde_h0_t_dx    = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        m_tilde_h0_t_dy    = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        m_tilde_h0_t_dz    = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        m_twiddle_factors  = std::make_unique<dw::gl::Texture2D>(log(m_N) / log(2), m_N, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        m_ping_pong        = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);
        m_dx               = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_R32F, GL_RED, GL_FLOAT);
        m_dy               = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_R32F, GL_RED, GL_FLOAT);
        m_dz               = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_R32F, GL_RED, GL_FLOAT);
        m_normal_map       = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RGBA32F, GL_RGBA, GL_FLOAT);

        m_noise0->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_noise0->set_min_filter(GL_NEAREST);
        m_noise0->set_mag_filter(GL_NEAREST);

        m_noise1->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_noise1->set_min_filter(GL_NEAREST);
        m_noise1->set_mag_filter(GL_NEAREST);

        m_noise2->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_noise2->set_min_filter(GL_NEAREST);
        m_noise2->set_mag_filter(GL_NEAREST);

        m_noise3->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_noise3->set_min_filter(GL_NEAREST);
        m_noise3->set_mag_filter(GL_NEAREST);

        m_tilde_h0_k->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_tilde_h0_k->set_min_filter(GL_NEAREST);
        m_tilde_h0_k->set_mag_filter(GL_NEAREST);

        m_tilde_h0_minus_k->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_tilde_h0_minus_k->set_min_filter(GL_NEAREST);
        m_tilde_h0_minus_k->set_mag_filter(GL_NEAREST);

        m_twiddle_factors->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_twiddle_factors->set_min_filter(GL_NEAREST);
        m_twiddle_factors->set_mag_filter(GL_NEAREST);

        m_tilde_h0_t_dx->set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
        m_tilde_h0_t_dx->set_min_filter(GL_LINEAR);
        m_tilde_h0_t_dx->set_mag_filter(GL_LINEAR);

        m_tilde_h0_t_dy->set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
        m_tilde_h0_t_dy->set_min_filter(GL_LINEAR);
        m_tilde_h0_t_dy->set_mag_filter(GL_LINEAR);

        m_tilde_h0_t_dz->set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
        m_tilde_h0_t_dz->set_min_filter(GL_LINEAR);
        m_tilde_h0_t_dz->set_mag_filter(GL_LINEAR);

        m_normal_map->set_wrapping(GL_REPEAT, GL_REPEAT, GL_REPEAT);
        m_normal_map->set_min_filter(GL_LINEAR);
        m_normal_map->set_mag_filter(GL_LINEAR);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    bool create_uniform_buffer()
    {
        // Create uniform buffer for global data
        m_global_ubo = std::make_unique<dw::gl::UniformBuffer>(GL_DYNAMIC_DRAW, sizeof(GlobalUniforms));

        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void create_camera()
    {
        m_main_camera = std::make_unique<dw::Camera>(60.0f, 0.1f, CAMERA_FAR_PLANE, float(m_width) / float(m_height), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(-1.0f, 0.0, 0.0f));
        m_main_camera->set_rotatation_delta(glm::vec3(0.0f, -90.0f, 0.0f));
        m_main_camera->update();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_uniforms()
    {
        void* ptr = m_global_ubo->map(GL_WRITE_ONLY);
        memcpy(ptr, &m_global_uniforms, sizeof(GlobalUniforms));
        m_global_ubo->unmap();
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_transforms(dw::Camera* camera)
    {
        // Update camera matrices.
        m_global_uniforms.view_proj = camera->m_projection * camera->m_view;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update_camera()
    {
        dw::Camera* current = m_main_camera.get();

        float forward_delta = m_heading_speed * m_delta;
        float right_delta   = m_sideways_speed * m_delta;

        current->set_translation_delta(current->m_forward, forward_delta);
        current->set_translation_delta(current->m_right, right_delta);

        m_camera_x = m_mouse_delta_x * m_camera_sensitivity;
        m_camera_y = m_mouse_delta_y * m_camera_sensitivity;

        if (m_mouse_look)
        {
            // Activate Mouse Look
            current->set_rotatation_delta(glm::vec3((float)(m_camera_y),
                                                    (float)(m_camera_x),
                                                    (float)(0.0f)));
        }
        else
        {
            current->set_rotatation_delta(glm::vec3((float)(0),
                                                    (float)(0),
                                                    (float)(0)));
        }

        current->update();
        update_transforms(current);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:
    // General GPU resources.
    std::unique_ptr<dw::gl::Shader> m_triangle_vs;
    std::unique_ptr<dw::gl::Shader> m_visualize_image_fs;
    std::unique_ptr<dw::gl::Shader> m_tilde_h0_k_cs;
    std::unique_ptr<dw::gl::Shader> m_tilde_h0_t_cs;
    std::unique_ptr<dw::gl::Shader> m_twiddle_factors_cs;
    std::unique_ptr<dw::gl::Shader> m_butterfly_cs;
    std::unique_ptr<dw::gl::Shader> m_inversion_cs;
    std::unique_ptr<dw::gl::Shader> m_normal_map_cs;
    std::unique_ptr<dw::gl::Shader> m_grid_vs;
    std::unique_ptr<dw::gl::Shader> m_grid_non_tess_vs;
    std::unique_ptr<dw::gl::Shader> m_grid_tcs;
    std::unique_ptr<dw::gl::Shader> m_grid_tes;
    std::unique_ptr<dw::gl::Shader> m_ocean_wireframe_fs;
    std::unique_ptr<dw::gl::Shader> m_ocean_lit_fs;

    std::unique_ptr<dw::gl::Program> m_visualize_image_program;
    std::unique_ptr<dw::gl::Program> m_tilde_h0_k_program;
    std::unique_ptr<dw::gl::Program> m_tilde_h0_t_program;
    std::unique_ptr<dw::gl::Program> m_twiddle_factors_program;
    std::unique_ptr<dw::gl::Program> m_butterfly_program;
    std::unique_ptr<dw::gl::Program> m_inversion_program;
    std::unique_ptr<dw::gl::Program> m_normal_map_program;
    std::unique_ptr<dw::gl::Program> m_ocean_wireframe_program;
    std::unique_ptr<dw::gl::Program> m_ocean_wireframe_no_tess_program;
    std::unique_ptr<dw::gl::Program> m_ocean_lit_program;

    std::unique_ptr<dw::gl::Texture2D> m_noise0;
    std::unique_ptr<dw::gl::Texture2D> m_noise1;
    std::unique_ptr<dw::gl::Texture2D> m_noise2;
    std::unique_ptr<dw::gl::Texture2D> m_noise3;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_k;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_minus_k;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_t_dy;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_t_dx;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_t_dz;
    std::unique_ptr<dw::gl::Texture2D> m_twiddle_factors;
    std::unique_ptr<dw::gl::Texture2D> m_ping_pong;
    std::unique_ptr<dw::gl::Texture2D> m_dx;
    std::unique_ptr<dw::gl::Texture2D> m_dy;
    std::unique_ptr<dw::gl::Texture2D> m_dz;
    std::unique_ptr<dw::gl::Texture2D> m_normal_map;

    std::unique_ptr<dw::gl::ShaderStorageBuffer> m_bit_reversed_indices;

    std::unique_ptr<dw::gl::VertexBuffer> m_grid_vbo;
    std::unique_ptr<dw::gl::IndexBuffer>  m_grid_ibo;
    std::unique_ptr<dw::gl::VertexArray>  m_grid_vao;
    uint32_t                              m_grid_num_vertices = 0;
    uint32_t                              m_grid_num_indices  = 0;

    std::unique_ptr<dw::gl::UniformBuffer> m_global_ubo;
    std::unique_ptr<dw::Camera>            m_main_camera;

    GlobalUniforms m_global_uniforms;

    // Camera controls.
    bool  m_mouse_look         = false;
    float m_heading_speed      = 0.0f;
    float m_sideways_speed     = 0.0f;
    float m_camera_sensitivity = 0.05f;
    float m_camera_speed       = 0.001f;

    // Ocean
    float m_displacement_scale = 0.5f;
    float m_choppiness         = 0.75f;

    // Camera orientation.
    float m_camera_x;
    float m_camera_y;

    // FFT options
    float     m_wind_speed         = 80.0f;
    float     m_amplitude          = 2.0f;
    float     m_suppression_factor = 0.1f;
    glm::vec2 m_wind_direction     = glm::vec2(1.0f, 1.0f);
    int32_t   m_N                  = DISPLACEMENT_MAP_SIZE;
    int32_t   m_L                  = 1000;
};

DW_DECLARE_MAIN(FFTOceanWaves)