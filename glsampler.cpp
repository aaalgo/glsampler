#include <string>
#include <vector>
#include <thread>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glog/logging.h>
#include "glsampler.h"

namespace glsampler {
using std::string;
using std::vector;

string const VERTEX_SHADER = R"gl(
#version 330 core
layout(location = 0) in vec2 pos_in;
layout(location = 1) in vec3 tex_in;
out vec3 tex; 
uniform mat4 mvp;
void main(){
    gl_Position =  vec4(pos_in,0,1);
    tex = (mvp * vec4(tex_in, 1)).xyz;
}
)gl";

string const FRAGMENT_SHADER = R"gl(
#version 330 core
in vec3 tex;
//layout(location=0)
out float color;
uniform sampler3D sampler;
void main(){
    color = texture(sampler, tex).r;
}
)gl";

GLuint LoadShader (GLenum shaderType, string const &buf) {
	GLuint ShaderID = glCreateShader(shaderType);
	GLint Result = GL_FALSE;
	int InfoLogLength;
    char const *ptr = buf.c_str();
	glShaderSource(ShaderID, 1, &ptr , NULL);
	glCompileShader(ShaderID);
	// Check Vertex Shader
	glGetShaderiv(ShaderID, GL_COMPILE_STATUS, &Result);
	glGetShaderiv(ShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if (InfoLogLength > 0){
		string msg;
        msg.resize(InfoLogLength+1);
		glGetShaderInfoLog(ShaderID, InfoLogLength, NULL, &msg[0]);
        LOG(WARNING) << msg;
	}
    CHECK(Result);
    return ShaderID;
}

GLuint LoadProgram (string const &vshader, string const &fshader) {
	// Create the shaders
	GLuint VertexShaderID = LoadShader(GL_VERTEX_SHADER, vshader);
	GLuint FragmentShaderID = LoadShader(GL_FRAGMENT_SHADER, fshader);
	// Link the program
	LOG(INFO) << "Linking program";
	GLuint program = glCreateProgram();
	glAttachShader(program, VertexShaderID);
	glAttachShader(program, FragmentShaderID);
	glLinkProgram(program);

	GLint Result = GL_FALSE;
	int InfoLogLength;
	// Check the program
	glGetProgramiv(program, GL_LINK_STATUS, &Result);
    CHECK(Result);
	glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength);
	if ( InfoLogLength > 0 ){
		string msg;
        msg.resize(InfoLogLength+1);
		glGetProgramInfoLog(program, InfoLogLength, NULL, &msg[0]);
        LOG(WARNING) << msg;
	}
	
	glDetachShader(program, VertexShaderID);
	glDetachShader(program, FragmentShaderID);
	
	glDeleteShader(VertexShaderID);
	glDeleteShader(FragmentShaderID);

	return program;
}

#define CHECK_POWER_OF_TWO(x) CHECK(((x)&((x)-1)) == 0)

class SamplerImplGl: public Sampler {
    GLFWwindow* window;
    GLuint program;
    GLuint sampler;
    GLuint itexture, otexture;
    GLuint framebuffer;
    GLuint v_pos, v_tex, v_array;
    GLuint matrix;
    std::thread::id thread_id;

    void check_thread () {
        if (thread_id != std::this_thread::get_id()) {
            LOG(ERROR) << "Cross thread rendering is not working!";
            CHECK(false);
        }
    }
public:
    SamplerImplGl (): thread_id(std::this_thread::get_id()) {
        LOG(WARNING) << "Constructing sampler";
        CHECK_POWER_OF_TWO(CUBE_SIZE);
        CHECK_POWER_OF_TWO(VOLUME_SIZE);
        CHECK_POWER_OF_TWO(VIEW_SIZE);
        CHECK(CUBE_SIZE * CUBE_SIZE * CUBE_SIZE == VIEW_SIZE * VIEW_SIZE);
        if(!glfwInit()) CHECK(false) << "Failed to initialize GLFW";
        glfwWindowHint(GLFW_SAMPLES, 4);
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // To make MacOS happy; should not be needed
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        // we are not going to use the window and will render to texture, so size doesn't matter
        window = glfwCreateWindow(32, 32, "", NULL, NULL);
        CHECK(window) << "Failed to open GLFW window";
        glfwMakeContextCurrent(window);
        // Initialize GLEW
        glewExperimental = true; // Needed for core profile
        if (glewInit() != GLEW_OK) {
            CHECK(false) << "Failed to initialize GLEW";
        }
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_3D);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

        program = LoadProgram(VERTEX_SHADER, FRAGMENT_SHADER);
        sampler = glGetUniformLocation(program, "sampler");
        matrix = glGetUniformLocation(program, "mvp");

        vector<float> pos;
        vector<float> tex;

        int o = 0;
        for (int i = 0; i < CUBE_SIZE; ++i) {
            for (int j = 0; j < CUBE_SIZE; ++j) {
                for (int k = 0; k < CUBE_SIZE; ++k) {
                    pos.push_back(2.0*(o%VIEW_SIZE+1)/VIEW_SIZE-1);
                    pos.push_back(2.0*(o/VIEW_SIZE+1)/VIEW_SIZE-1);
                    tex.push_back(1.0 * k / CUBE_SIZE);
                    tex.push_back(1.0 * j / CUBE_SIZE);
                    tex.push_back(1.0 * i / CUBE_SIZE);
                    ++o;
                }
            }
        }

        glGenVertexArrays(1, &v_array);
        glBindVertexArray(v_array);
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);

        // output position doesn't change
        glGenBuffers(1, &v_pos);
        glBindBuffer(GL_ARRAY_BUFFER, v_pos);
        glBufferData(GL_ARRAY_BUFFER, sizeof(pos[0]) * pos.size(), &pos[0], GL_STATIC_DRAW);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        // texture sampler position doesn't change
        glGenBuffers(1, &v_tex);
        glBindBuffer(GL_ARRAY_BUFFER, v_tex);
        glBufferData(GL_ARRAY_BUFFER, sizeof(tex[0]) * tex.size(), &tex[0], GL_STATIC_DRAW);

		glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);

        glGenTextures(1, &otexture);
        glBindTexture(GL_TEXTURE_2D, otexture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, VIEW_SIZE, VIEW_SIZE, 0, GL_RED, GL_UNSIGNED_BYTE, 0);

        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, otexture, 0);

        CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
        glViewport(0, 0, VIEW_SIZE, VIEW_SIZE);
		glUseProgram(program);

        glGenTextures(1, &itexture);
        glBindTexture(GL_TEXTURE_3D, itexture);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    ~SamplerImplGl () {
        //check_thread();
		glDisableVertexAttribArray(0);
		glDisableVertexAttribArray(1);
        glDeleteBuffers(1, &v_pos);
        glDeleteBuffers(1, &v_tex);
        glDeleteTextures(1, &itexture);
        glDeleteTextures(1, &otexture);
        glDeleteFramebuffers(1, &framebuffer);
        glDeleteVertexArrays(1, &v_array);
        glDeleteProgram(program);
        glfwTerminate();
    }

    void load (uint8_t const *buf) {
        check_thread();
        glTexImage3D(GL_TEXTURE_3D, 0, GL_RED,
                     VOLUME_SIZE, VOLUME_SIZE, VOLUME_SIZE,
                     0, GL_RED, GL_UNSIGNED_BYTE, buf);
        glUniform1i(sampler, 0);
    }

    void sample (float x, float y, float z, float phi, float theta, float kappa, float scale0, uint8_t *buf) {
        //TODO fix scale0

        check_thread();
        glm::vec3 center(x, y, z);
        glm::vec3 rotate(phi, theta, kappa);
        // 1 -> scale -> rotate -> shift
        float scale = 1.0 * CUBE_SIZE / VOLUME_SIZE / scale0;
        /*
        cout << "CENTER: " << center[0] << ' ' << center[1] << ' ' << center[2] << endl;
        cout << "ANGLE: " << rotate[0] << ' ' << rotate[1] << ' ' << rotate[2] << endl;
        cout << "SCALE: " << scale0 << endl;
        */
        glm::mat4 mvp = glm::translate(float(1.0/VOLUME_SIZE) * center) *
                        glm::scale(glm::vec3(scale, scale, scale)) *
                        glm::rotate(float(rotate[2] * 180/M_PI),   // glm requires digrees
                                glm::vec3(
                                    sin(rotate[0]) * cos(rotate[1]),
                                    sin(rotate[0]) * sin(rotate[1]),
                                    cos(rotate[0]))) *
                        glm::translate(glm::vec3(-0.5, -0.5, -0.5));
        glUniformMatrix4fv(matrix, 1, GL_FALSE, &mvp[0][0]);

		glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, VIEW_PIXELS);

        glFinish();


        glBindTexture(GL_TEXTURE_2D, otexture);
        GLint v;
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &v);
        CHECK(v == VIEW_SIZE);
        glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_HEIGHT, &v);
        CHECK(v == VIEW_SIZE);

        if (buf) {
            glGetTexImage(GL_TEXTURE_2D, 0, GL_RED, GL_UNSIGNED_BYTE, buf);
        }
    }
};

Sampler *implGl = 0;

Sampler *Sampler::get (bool gl) {
    if (gl) {
        if (!implGl) {
            implGl = new SamplerImplGl();
        }
        return implGl;
    }
    return 0;
}

void Sampler::cleanup () {
    if (implGl) delete implGl;
}

}
