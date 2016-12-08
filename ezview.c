#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define SIZE 256


float angle = 0;
float scale = 1;
float shear = 0;
float xTran = 0;
float yTran = 0;


typedef struct {
  float Position[2];
  float TexCoord[2];
} Vertex;

// (-1, 1)  (1, 1)
// (-1, -1) (1, -1)

Vertex vertexes[] = {
  {{1, -1},  {0.99999, 0.99999}},
  {{1, 1}, {0.99999, 0}},
  {{-1, -1}, {0, 0.99999}},
  {{-1, 1}, {0, 0}}
};

GLuint indices[] = {
    0, 1, 3,   // First Triangle
    2, 0, 3     // Second Triangle
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

      if (key == GLFW_KEY_S && action == GLFW_PRESS)
        shear += .05;

      if (key == GLFW_KEY_A && action == GLFW_PRESS)
        shear -= .05;

      if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        xTran += .05;

      if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        xTran -= .05;

      if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        yTran += .05;

      if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        yTran -= .05;

    if(key == GLFW_KEY_R && action == GLFW_PRESS){
      angle -= 1;
      if(angle >= 4) angle = 0;
    }

    if(key == GLFW_KEY_E && action == GLFW_PRESS ){
      angle += 1;
      if(angle <= -4) angle = 0;
    }
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  scale += (float)yoffset / 100;
  if(scale <= 0) scale = 0;
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
void glLinkProgramOrDie(GLuint program) {
  GLint linked;
  glLinkProgram(program);
  glGetProgramiv(program,
		GL_LINK_STATUS,
		&linked);
  if (!linked) {
    GLint infoLen = 0;
    glGetProgramiv(program,
		  GL_INFO_LOG_LENGTH,
		  &infoLen);
    char* info = malloc(infoLen+1);
    GLint done;
    glGetProgramInfoLog(program, infoLen, &done, info);
    printf("Unable to link program: %s\n", info);
    exit(1);
  }
}

void reverse(unsigned char* ar, int n){
  char c;
  for(int i = 0; i < n; i++){
    c = ar[i];
    ar[i] = ar[n];
    ar[n] = c;
    n--;
  }
  return;
}

unsigned char* loadImage(FILE* inFile, int* width, int* height){
  //buffer used for the comments mainly
  char buff[SIZE], *fh;

  int w, h;
  unsigned char* image;
  int read;
  unsigned int maxColors;


  fh = (char *)malloc(sizeof(char) * SIZE);
  fh = fgets(buff, SIZE, inFile);             //Make sure we are reading the right type of file
  if ( (fh == NULL) || ( strncmp(buff, "P6\n", 3) != 0 ) ) perror("Please provide a P6 .ppm file for conversion\n");

  //get rid of comments
  do
        {
           fh = fgets(buff, SIZE, inFile);      //write the comments into the out file
           if ( fh == NULL ) return NULL;
        } while ( strncmp(buff, "#", 1) == 0 );

  //read in the width and height
  read = sscanf(buff, "%u %u", &w, &h);

  //throw error if the width and height aren't in the file
  if(read < 2) {
    perror("File Unreadable. Please check the file format\n");
    return NULL;
  }

  read = fscanf(inFile, "%u", &maxColors);
  printf("%d\n", maxColors);
  //check that the right color format is used
  if(maxColors != 255 || read != 1) {
    perror("Please provide an 24-bit color file");
    return NULL;
  }

  fseek(inFile, 1, SEEK_CUR);
  image = (unsigned char *)malloc(sizeof(char)* 3 * w * h);
  fread(image, sizeof(unsigned char), w * h * 3, inFile);

  //reverse(image, w * h * 3);

  *width = w;
  *height = h;
  return image;
}

int main(int argc, char *argv[])
{

  if(argc < 2 || argc > 2) {
    perror("Usage: ./ezview image-source.ppm \n");
  }

  if(strstr(argv[1], ".ppm") == NULL) {
    perror("Please provide a .ppm file tp be read");
    return 0;
  }

  FILE *inFile = fopen(argv[1], "rb");

  GLint image_width, image_height;

  GLubyte* image = (GLubyte*) loadImage(inFile, &image_width, &image_height);



    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;

    glfwSetErrorCallback(error_callback);

    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    window = glfwCreateWindow(image_width, image_height, "Simple example", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

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
    glLinkProgramOrDie(program);


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


    GLuint texID;
    glGenTextures(1, &texID);
    glEnable( GL_TEXTURE_2D );
    glBindTexture(GL_TEXTURE_2D, texID);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image_width, image_height, 0, GL_RGB,
		 GL_UNSIGNED_BYTE, image);
    glGenerateMipmap(GL_TEXTURE_2D);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texID);
    glUniform1i(tex_location, 0);

    while (!glfwWindowShouldClose(window))
    {
        float ratio;
        int width, height;
        mat4x4 m, p, mvp, s;

        mat4x4 sh = {
            1.0f, 0.0f, 0.0f, 0.0f,
            shear, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        mat4x4 zoom = {
            scale, 0.0f, 0.0f, 0.0f,
            0.0, scale, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        mat4x4 tran = {
            1.0f, 0.0f, 0.0f, xTran,
            0.0, 1.0f, 0.0f, yTran,
            0.0f, 0.0f, 1.0f, 1.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };


        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        mat4x4_identity(m);
        mat4x4_identity(s);
        mat4x4_mul(m, sh, m);
        mat4x4_mul(m, zoom, m);
        mat4x4_translate_in_place(m, xTran, yTran, 1.0);
        //mat4x4_add(m, tran, m);
        mat4x4_rotate_Z(m, m, (angle * M_PI/2));
        //mat4x4_scale(s,s, scale);

        mat4x4_identity(p);
        //mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
        mat4x4_mul(mvp, s, m);

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
