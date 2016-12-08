#include <OpenGL/gl.h>
#include <GLFW/glfw3.h>

#include "linmath.h"

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define SIZE 256

//global variables for paramaters changed by input callbacks
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
//Define the vertexes for the rectangle being drawn on
Vertex vertexes[] = {
  {{1, -1},  {0.99999, 0.99999}},
  {{1, 1}, {0.99999, 0}},
  {{-1, -1}, {0, 0.99999}},
  {{-1, 1}, {0, 0}}
};
//Define the two triangles of the square
GLuint indices[] = {
    0, 1, 3,   // First Triangle
    2, 0, 3     // Second Triangle
};


//vertex shader code
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

//fragment shader code
static const char* fragment_shader_text =
"varying vec2 TexCoordOut;\n"
"uniform sampler2D Texture;\n"
"void main()\n"
"{\n"
"    gl_FragColor = texture2D(Texture, TexCoordOut);\n"
"}\n";

//generic error handling
static void error_callback(int error, const char* description)
{
    fprintf(stderr, "Error: %s\n", description);
}

//handle all user input from the keyboard
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    //if the escape key is pressed, close the window
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GLFW_TRUE);

    // Shear the image to the right with the 'S' key
      if (key == GLFW_KEY_S && action == GLFW_PRESS)
        shear += .05;

    // Shear the image to the left with the 'A' key
      if (key == GLFW_KEY_A && action == GLFW_PRESS)
        shear -= .05;

    // Pan the image to the left with the 'LEFT' key
      if (key == GLFW_KEY_LEFT && action == GLFW_PRESS)
        xTran += .05;

    // Pan the image to the right with the 'RIGHT' key
      if (key == GLFW_KEY_RIGHT && action == GLFW_PRESS)
        xTran -= .05;

    // Pan the image down with the 'DOWN' key
      if (key == GLFW_KEY_DOWN && action == GLFW_PRESS)
        yTran += .05;

    // Pan the image up with the 'UP' key
      if (key == GLFW_KEY_UP && action == GLFW_PRESS)
        yTran -= .05;

    // Rotate the image to the right using the 'R' key
    if(key == GLFW_KEY_R && action == GLFW_PRESS){
      angle -= 1;
      if(angle >= 4) angle = 0;
    }

  // Rotate the image to the left using the 'E' key
    if(key == GLFW_KEY_E && action == GLFW_PRESS ){
      angle += 1;
      if(angle <= -4) angle = 0;
    }
}

//handle zoom using a callback to the scroll
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
  //scale the image using the y axis offset from the scroll
  scale += (float)yoffset / 100;
  if(scale <= 0) scale = 0;
}

// Program to handle the compiling of the shader, and upon failure the calling of an error and exit of the program
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

// Program to handle the linking of the program, and upon failure the calling of an error and exit of the program
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

// method used to handle the loading of the image to be viewed, in .ppm format
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
  //Read the image data from the file
  fread(image, sizeof(unsigned char), w * h * 3, inFile);

  *width = w;
  *height = h;
  return image;
}

int main(int argc, char *argv[])
{
  //Check for propper arguments
  if(argc < 2 || argc > 2) {
    perror("Usage: ./ezview image-source.ppm \n");
  }

  if(strstr(argv[1], ".ppm") == NULL) {
    perror("Please provide a .ppm file tp be read");
    return 0;
  }

  //open the image file
  FILE *inFile = fopen(argv[1], "rb");

  GLint image_width, image_height;

  //read in the image file and store it
  GLubyte* image = (GLubyte*) loadImage(inFile, &image_width, &image_height);

    GLFWwindow* window;
    GLuint vertex_buffer, vertex_shader, fragment_shader, program;
    GLint mvp_location, vpos_location, vcol_location;


    glfwSetErrorCallback(error_callback);

    //initialize glfw
    if (!glfwInit())
        exit(EXIT_FAILURE);

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);

    //create the glfw window, use the image to set width and height, and file name for title
    window = glfwCreateWindow(image_width, image_height, argv[1], NULL, NULL);
    if (!window)
    {   //terminate if unable to open
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    //set the callbacks for the input
    glfwSetKeyCallback(window, key_callback);
    glfwSetScrollCallback(window, scroll_callback);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    //set up the element buffer object
    GLuint EBO;
    glGenBuffers(1, &EBO);


    glGenBuffers(1, &vertex_buffer);
    glBindBuffer(GL_ARRAY_BUFFER, vertex_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexes), vertexes, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    //element buffer object used for indeces
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
    sizeof(indices), indices, GL_STATIC_DRAW);

    //initialize the vertex shader
    vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex_shader, 1, &vertex_shader_text, NULL);
    glCompileShaderOrDie(vertex_shader);

    //initialize the fragment shader
    fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment_shader, 1, &fragment_shader_text, NULL);
    glCompileShaderOrDie(fragment_shader);

    // Create the program
    program = glCreateProgram();
    glAttachShader(program, vertex_shader);
    glAttachShader(program, fragment_shader);
    glLinkProgramOrDie(program);

    //set the mvp location from the vertex shader
    mvp_location = glGetUniformLocation(program, "MVP");
    assert(mvp_location != -1);

    //set the vpos location from the vertex shader
    vpos_location = glGetAttribLocation(program, "vPos");
    assert(vpos_location != -1);
    //set the texture coordinate location from the fragment shader
    GLint texcoord_location = glGetAttribLocation(program, "TexCoordIn");
    assert(texcoord_location != -1);
    //set the texture location from the fragment shader
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

    //setup textures
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

    //main program loop
    while (!glfwWindowShouldClose(window))
    {
        GLfloat ratio;
        int width, height;
        mat4x4 m, p, mvp, s;

        //matrix used for the shear operation
        mat4x4 sh = {
            1.0f, 0.0f, 0.0f, 0.0f,
            shear, 1.0f, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };
        //matrix used for the zoom operation
        mat4x4 zoom = {
            scale, 0.0f, 0.0f, 0.0f,
            0.0, scale, 0.0f, 0.0f,
            0.0f, 0.0f, 1.0f, 0.0f,
            0.0f, 0.0f, 0.0f, 1.0f
        };

        glfwGetFramebufferSize(window, &width, &height);
        ratio = width / (float) height;

        glViewport(0, 0, width, height);
        glClear(GL_COLOR_BUFFER_BIT);

        //set up matrix for transformation
        mat4x4_identity(m);
        //apply shear
        mat4x4_mul(m, sh, m);
        //apply zoom
        mat4x4_mul(m, zoom, m);
        //apply translate
        mat4x4_translate_in_place(m, xTran, yTran, 1.0);
        //apply rotate
        mat4x4_rotate_Z(m, m, (angle * M_PI/2));

        mat4x4_identity(p);
        //apply all transformations
        mat4x4_mul(mvp,p, m);

        glUseProgram(program);
        glUniformMatrix4fv(mvp_location, 1, GL_FALSE, (const GLfloat*) mvp);
        //draw the updated geometry to the screen
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    //exit
    glfwTerminate();
    exit(EXIT_SUCCESS);
}
