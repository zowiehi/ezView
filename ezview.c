#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define SIZE 256

typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

// (-1, 1)  (1, 1)
// (-1, -1) (1, -1)

Vertex vertexes[] = {
  {{1, 1}, {0.99999, 0}},
  {{1, -1},  {0.99999, 0.99999}},
  {{-1, -1}, {0, 0.99999}},
  {{-1, 1}, {0, 0.99999}}
};

GLuint indices[] = {
    0, 1, 3,   // First Triangle
    1, 2, 3    // Second Triangle
};

static const char* vertex_shader_text =
"uniform mat4 MVP;\n"
"attribute vec2 TexCoordIn;\n"
"attribute vec2 vPos;\n"
"varying vec2 TexCoordOut;\n"
"void main()\n"
"{\n"
"    gl_Position = MVP * vec4(vPos, 0.0, 1.0);\n"
"    TexCoordOut = TexCoordIn;\n"
"}\n";

static const char* fragment_shader_text =
"varying vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}\n";

static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);
}

void glCompileShaderOrDie(GLuint shader) {
  GLint compiled;
  glCompileShader(shader);
  glGetShaderiv(shader,
		GL_COMPILE_STATUS,
		&compiled);
  if (!compiled) {
    GLint infoLen = 0;
    glGetShaderiv(shader,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetShaderInfoLog(shader, infoLen, &done, info);
    printf("Unable to compile shader: %s\n", info);
    exit(1);
  }
}

// 4 x 4 image..
unsigned char image[] = {
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,
  255, 0, 0, 255,

  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,
  0, 255, 0, 255,

  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,
  0, 0, 255, 255,

  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255,
  255, 0, 255, 255
};

//represents a single pixel object
typedef struct RGB {
  unsigned char r, g, b;
}RGBPix;

//this struct is used to store the entire image data, along with the width and height
typedef struct Image {
  int width, height;
  unsigned char *data;
}PPMImage;

int main(int argc, char *argv[])
{

  if(argc < 2 || argc > 2) {
    perror("Usage: ./ezview image-source.ppm \n");
  }

  if(strstr(argv[1], ".ppm") == NULL) {
    perror("Please provide a .ppm file tp be read");
    return 0;
  }
  printf("here\n" );
  FILE *inFile = fopen(argv[1], "rb");


  //buffer used for the comments mainly
  char buff[SIZE], *fh;

  PPMImage im;

  int read;
  unsigned int maxColors;


  fh = (char *)malloc(sizeof(char) * SIZE);
  fh = fgets(buff, SIZE, inFile);             //Make sure we are reading the right type of file
  if ( (fh == NULL) || ( strncmp(buff, "P6\n", 3) != 0 ) ) perror("Please provide a P6 .ppm file for conversion\n");

  //get rid of comments
  do
        {
           fh = fgets(buff, SIZE, inFile);      //write the comments into the out file
           if ( fh == NULL ) return 1;
        } while ( strncmp(buff, "#", 1) == 0 );

  //read in the width and height
  read = sscanf(buff, "%u %u", &im.width, &im.height);

  printf("here\n" );
  //throw error if the width and height aren't in the file
  if(read < 2) {
    perror("File Unreadable. Please check the file format\n");
    return 1;
  }
  im.data = (unsigned char *)malloc(sizeof(char)* 3 * im.width * im.height);

  read = fscanf(inFile, "%u", &maxColors);
  //check that the right color format is used
  if(maxColors != 255 || read != 1) {
    perror("Please provide an 24-bit color file");
    return 1;
  }

  fread(im.data, sizeof(char)*3, im.width * im.height, inFile);

    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(640, 480, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);

    glfwMakeContextCurrent(window);
    // gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval(1);

    // NOTE: OpenGL error checks have been omitted for brevity

    GLuint EBO;
    glGenBuffers(1, &EBO);

    // GLuint VAO;
    // glGenVertexArrays(1, &VAO);
    // glBindVertexArray(VAO);

    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices), indices, GL_STATIC_DRAW);

    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgram(program);
    // more error checking! glLinkProgramOrDie!

    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);

    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);

    GLint tex_location = glGetUniformLocation(program, "Texture");
    assert(tex_location != -1);

    glEnableVertexAttribArray(vpos_location);
    glVertexAttribPointer(vpos_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) 0);

    glEnableVertexAttribArray(texcoord_location);
    glVertexAttribPointer(texcoord_location,
			  2,
			  GL_FLOAT,
			  GL_FALSE,
                          sizeof(Vertex),
			  (void*) (sizeof(float) * 2));

    int image_width = 4;
    int image_height = 4;

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA,
		 GL_UNSIGNED_BYTE, image);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 m, p, mvp;

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        //mat4x4_rotate_Z(m, m, (float) glfwGetTime());
        mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, p, m);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);

    glfwTerminate();
    exit(EXIT_SUCCESS);
}
