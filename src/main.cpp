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

struct GlobalUniforms
{
    DW_ALIGNED(16)
    glm::mat4 view_proj;
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
        tilde_h0_k();
        
        return true;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void update(double delta) override
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Update camera.
        update_camera();

        update_uniforms();

        m_debug_draw.aabb(glm::vec3(10.0f), glm::vec3(-10.0f), glm::vec3(1.0f, 0.0f, 0.0f));
        
        tilde_h0_t();
        render_visualization_quad(m_tilde_h0_t_dy);

        m_debug_draw.render(nullptr, m_width, m_height, m_global_uniforms.view_proj, m_main_camera->m_position);
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    void window_resized(int width, int height) override
    {
        // Override window resized method to update camera projection.
        m_main_camera->update_projection(60.0f, 1.0f, CAMERA_FAR_PLANE, float(m_width) / float(m_height));
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

        settings.resizable    = true;
        settings.maximized    = false;
        settings.refresh_rate = 60;
        settings.major_ver    = 4;
        settings.width        = 1920;
        settings.height       = 1080;
        settings.title        = "FFT Ocean Waves (c) 2020 Dihara Wijetunga";

        return settings;
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

private:
    // -----------------------------------------------------------------------------------------------------------------------------------

    void render_visualization_quad(std::unique_ptr<dw::gl::Texture2D>& texture)
    {
        glViewport(0, 0, texture->width(), texture->height());

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

        glEnable(GL_DEPTH_TEST);
        glEnable(GL_CULL_FACE);
        glEnable(GL_BLEND);
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
    }

    // -----------------------------------------------------------------------------------------------------------------------------------

    bool create_shaders()
    {
        {
            // Create general shaders
            m_triangle_vs        = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_VERTEX_SHADER, "shader/triangle_vs.glsl"));
            m_visualize_image_fs = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_FRAGMENT_SHADER, "shader/visualize_image_fs.glsl"));
            m_tilde_h0_k_cs       = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/tilde_h0_k_cs.glsl"));
            m_tilde_h0_t_cs      = std::unique_ptr<dw::gl::Shader>(dw::gl::Shader::create_from_file(GL_COMPUTE_SHADER, "shader/tilde_h0_t_cs.glsl"));

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

                m_visualize_image_program->uniform_block_binding("GlobalUniforms", 0);
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
        m_tilde_h0_t_dx       = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RG32F, GL_RG, GL_FLOAT);
        m_tilde_h0_t_dy    = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RG32F, GL_RG, GL_FLOAT);
        m_tilde_h0_t_dz       = std::make_unique<dw::gl::Texture2D>(m_N, m_N, 1, 1, 1, GL_RG32F, GL_RG, GL_FLOAT);

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

        m_tilde_h0_t_dx->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_tilde_h0_t_dx->set_min_filter(GL_NEAREST);
        m_tilde_h0_t_dx->set_mag_filter(GL_NEAREST);

        m_tilde_h0_t_dy->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_tilde_h0_t_dy->set_min_filter(GL_NEAREST);
        m_tilde_h0_t_dy->set_mag_filter(GL_NEAREST);

        m_tilde_h0_t_dz->set_wrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
        m_tilde_h0_t_dz->set_min_filter(GL_NEAREST);
        m_tilde_h0_t_dz->set_mag_filter(GL_NEAREST);
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
        m_main_camera = std::make_unique<dw::Camera>(60.0f, 1.0f, CAMERA_FAR_PLANE, float(m_width) / float(m_height), glm::vec3(50.0f, 20.0f, 0.0f), glm::vec3(-1.0f, 0.0, 0.0f));
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

    std::unique_ptr<dw::gl::Program> m_visualize_image_program;
    std::unique_ptr<dw::gl::Program> m_tilde_h0_k_program;
    std::unique_ptr<dw::gl::Program> m_tilde_h0_t_program;

    std::unique_ptr<dw::gl::Texture2D> m_noise0;
    std::unique_ptr<dw::gl::Texture2D> m_noise1;
    std::unique_ptr<dw::gl::Texture2D> m_noise2;
    std::unique_ptr<dw::gl::Texture2D> m_noise3;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_k;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_minus_k;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_t_dy;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_t_dx;
    std::unique_ptr<dw::gl::Texture2D> m_tilde_h0_t_dz;

    std::unique_ptr<dw::gl::UniformBuffer> m_global_ubo;
    std::unique_ptr<dw::Camera>            m_main_camera;

    GlobalUniforms m_global_uniforms;

    // Camera controls.
    bool  m_mouse_look         = false;
    float m_heading_speed      = 0.0f;
    float m_sideways_speed     = 0.0f;
    float m_camera_sensitivity = 0.05f;
    float m_camera_speed       = 0.05f;

    // Camera orientation.
    float m_camera_x;
    float m_camera_y;
    float m_light_size = 0.07f;

    // FFT options
    float       m_wind_speed         = 80.0f;
    float       m_amplitude          = 2.0f;
    float       m_suppression_factor = 0.1f;
    glm::vec2   m_wind_direction     = glm::vec2(1.0f, 1.0f);
    int32_t     m_N                  = DISPLACEMENT_MAP_SIZE;
    int32_t     m_L                  = 1000;
};

DW_DECLARE_MAIN(FFTOceanWaves)