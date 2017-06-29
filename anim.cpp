////////////////////////////////////////////////////
// anim.cpp version 4.1
// Template code for drawing an articulated figure.
// CS 174A
////////////////////////////////////////////////////

#ifdef _WIN32
#include <windows.h>
#include <GL/glew.h>
#include <GL/freeglut.h>
#include <GL/freeglut_ext.h>
#include "al.h" 
#include "alc.h" 
#include "alut.h"

// put alut.lib into the directory.
	
#else
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>
#include <time.h>
#include <string>
#include <sstream>

#ifdef _WIN32
#include "GL/freeglut.h"
#else
#include <GLUT/glut.h>
#endif

#include "Ball.h"
#include "FrameSaver.h"
#include "Timer.h"
#include "Shapes.h"
#include "tga.h"

#include "Angel/Angel.h"

#ifdef __APPLE__
#define glutInitContextVersion(a,b)
#define glutInitContextProfile(a)
#define glewExperimental int glewExperimentalAPPLE
#define glewInit()
#endif

FrameSaver FrSaver ;
Timer TM ;

BallData *Arcball = NULL ;
int Width = 800;
int Height = 800;
int Button = -1;
float Zoom = 1;
int PrevY = 0;
float angle = 0;
float xtrans, ytrans, ztrans = 0.0;
int initx, inity;

HMatrix r;

//initial bullet firing positions
float b_x_rot = 0.0;
float b_y_rot = 0.0;
float x_rot_angle = 0.0;
float y_rot_angle = 0.0;
//float z_rot_angle = 0.0;

bool begin_game = false;
static bool shooting = false;
static bool next_level = false;

int Animate = 1 ;
int Recording = 0 ;

void resetArcball() ;
void save_image();
void instructions();
void set_colour(float r, float g, float b) ;

const int STRLEN = 100;
typedef char STR[STRLEN];

// Timing variables
double TIME_LAST = TM.GetElapsedTime() ;
double DTIME  = 0.0;;
DWORD root_milliseconds; // Time stamp for bubbles
DWORD root_milliseconds2; // Time stamp for bullet
DWORD root_next_level; // Time stamp for clearing level

// Gameplay Variables
int background = 0; // Change background level
int speed = 2;
int score = 0; // DANIEL: Score for the game
int level = 1;
int hits = 0;
int shots = -1;
int accuracy = 0; // DANIEL: Accuracy to be displayed
int bonus = 0;
bool collision[9] = {false};
vec3 rand_offset[9];

#define PI 3.1415926535897
#define X 0
#define Y 1
#define Z 2

//OpenAL
ALCdevice* alDevice;
ALCcontext* alContext;

//glFont
void printw (float x, float y, float z, char* format, ...);


//texture
GLuint texture_cylinder;
GLuint texture_cube;
GLuint texture_earth;
GLuint texture_earth1;


// Structs that hold the Vertex Array Object index and number of vertices of each shape.
ShapeData cubeData;
ShapeData cubeData_2;
ShapeData sphereData;
ShapeData sphereData1;
ShapeData coneData;
ShapeData cylData;

Angel::vec4 up(0.0,1.0,0.0,0.0);

class Camera{
	public:
		mat4 mat_view();
		void move(int x, int y);
		Camera();
	protected:
		vec4 pos;
		vec4 target;
		float phi;
		float theta;
		int old_x;
		int old_y;
};

void Camera::move(int x, int y) {
	int dx = x - old_x;
	int dy = y - old_y;
	old_x = x;
	old_y = y;
	theta += (float)dx / 200.0;
	if(theta >= PI)
		theta = PI;
	if(theta <= 0.0)
		theta = 0.0;
	x_rot_angle = theta;
	phi += (float)dy / 200.0;
	if(phi >= 3.0/2.0 * PI)
		phi = 3.0/2.0 * PI;
	if(phi <= PI / 2.0)
		phi = PI / 2.0;
	y_rot_angle = phi;

	//printf("Pitch: %f, Yaw: %f\n", theta, phi);
	target.x = pos.x - cos(theta);
	target.y = pos.y + sin(phi);
}

mat4 Camera::mat_view() {
	return LookAt(pos, target, up);
}

Camera::Camera() {
	pos = vec4(0.0f, 0.0f, 30.0f, 1.0f);
	target = vec4(0.0f, 0.0f, 29.0f, 1.0f);
	phi = 0.0;
	theta = 0.0;
	old_x = initx + 100;
	old_y = inity - 100;
}

Camera global_camera;





//Sound class

class Sound {
public:
	unsigned int bufferID;
	unsigned int sourceID;
	void* data;
	bool overlap;
	int size;
	int format;
	int sampleRate;

	Sound(bool overlap, int format, int sampleRate, void* data, int size) {
		alGenBuffers(1, &bufferID);
		alBufferData(bufferID, format, data, size, sampleRate);
		alGenSources(1, &sourceID);
		alSourcei(sourceID, AL_BUFFER, bufferID);

		this->format = format;
		this->sampleRate = sampleRate;
		this->data = data;
		this->size = size;
		this->overlap = overlap;
	}

	void Play() {
		int state;
		alGetSourcei(sourceID, AL_SOURCE_STATE, &state);
		if(state != AL_PLAYING || overlap) {
			alSourcePlay(sourceID);
		}

	}

	static Sound* loadWAVE(const char* filename, bool overlap) {
		FILE* fp = NULL;
		fp = fopen(filename, "r");
		if(!fp) {
			std::cout << "Could Not Open: " << filename << std::endl;
			fclose(fp);
			return NULL;
		}

		char* ChunkID = new char[5];

		fread(ChunkID, 4, sizeof(char), fp);
		ChunkID[4] = '\0';

		if(strcmp(ChunkID, "RIFF")) {
			std::cout << ChunkID << std::endl;
			delete [] ChunkID;
			std::cout << "Not a RIFF " << filename << std::endl;
			fclose(fp);
			return NULL;
		}

		fseek(fp, 8, SEEK_SET);
		char* Format = new char[5];
		fread(Format, 4, sizeof(char), fp);
		Format[4] = '\0';
		if(strcmp(Format, "WAVE")) {
			std::cout << Format << std::endl;
			delete [] ChunkID;
			delete [] Format;
			std::cout << "Not a WAVE " << filename << std::endl;
			fclose(fp);
			return NULL;
		}

		char* SubChunk1ID = new char[5];
		fread(SubChunk1ID, 4, sizeof(char), fp);
		SubChunk1ID[4] = '\0';
		if(strcmp(SubChunk1ID, "fmt ")) {
			std::cout << SubChunk1ID << std::endl;
			delete [] ChunkID;
			delete [] Format;
			delete [] SubChunk1ID;
			std::cout << "Corrupted File " << filename << std::endl;
			fclose(fp);
			return NULL;
		}

		unsigned int SubChunk1Size;
		fread(&SubChunk1Size, 1, sizeof(int), fp);
		unsigned int SubChunk2Location = (unsigned int)ftell(fp) + SubChunk1Size;

		unsigned short AudioFormat;
		fread(&AudioFormat, 1, sizeof(unsigned short), fp);
		if(AudioFormat != 1)
		{
			delete [] ChunkID;
			delete [] Format;
			delete [] SubChunk1ID;
			std::cout << "Audio Is Not PCM " << filename << std::endl;
			fclose(fp);
			return NULL;
		}

		unsigned short NumChannels;
		fread(&NumChannels, 1, sizeof(unsigned short), fp);

		unsigned short SampleRate;
		fread(&SampleRate, 1, sizeof(unsigned short), fp);

		fseek(fp, 34, SEEK_SET);
		unsigned short BitsPerSample;
		fread(&BitsPerSample, 1, sizeof(unsigned short), fp);

		int ALFormat;
		if(NumChannels == 1 && BitsPerSample == 8) {
			ALFormat = AL_FORMAT_MONO8;
		} else if (NumChannels == 1 && BitsPerSample == 16) {
			ALFormat = AL_FORMAT_MONO16;
		}else if (NumChannels == 2 && BitsPerSample == 8) {
			ALFormat = AL_FORMAT_STEREO8;
		}else if (NumChannels == 2 && BitsPerSample == 16) {
			ALFormat = AL_FORMAT_STEREO16;
		} else {
			delete [] ChunkID;
			delete [] Format;
			delete [] SubChunk1ID;
			std::cout << "Audio Is Not Correctly Formatted NumChannels: " << NumChannels << " BitsPerSample: " << BitsPerSample << std::endl;
			fclose(fp);
			return NULL;
		}
		std::cout << "Formatted NumChannels: " << NumChannels << " BitsPerSample: " << BitsPerSample << std::endl;
		fseek(fp, SubChunk2Location, SEEK_SET);
		char* SubChunk2ID = new char[5];
		fread(SubChunk2ID, 4, sizeof(char), fp);
		SubChunk2ID[4] = '\0';
		if(strcmp(SubChunk2ID, "data")) {
			delete [] ChunkID;
			delete [] Format;
			delete [] SubChunk1ID;
			delete [] SubChunk2ID;
			std::cout << "File Corrupted " << filename << std::endl;
			fclose(fp);
			return NULL;
		}

		unsigned int SubChunk2Size;
		fread(&SubChunk2Size, 1, sizeof(unsigned int), fp);

		unsigned char* Data= new unsigned char[SubChunk2Size];
		fread(Data, SubChunk2Size, sizeof(unsigned char), fp);

		delete [] ChunkID;
		delete [] Format;
		delete [] SubChunk1ID;
		delete [] SubChunk2ID;
		fclose(fp);

		return new Sound(overlap, ALFormat, SampleRate, Data, SubChunk2Size);
	}


};

Sound* sound;
Sound* sound2;

// Matrix stack that can be used to push and pop the modelview matrix.
class MatrixStack {
    int    _index;
    int    _size;
    mat4*  _matrices;
    
public:
    MatrixStack( int numMatrices = 32 ):_index(0), _size(numMatrices)
    { _matrices = new mat4[numMatrices]; }
    
    ~MatrixStack()
	{ delete[]_matrices; }
    
    void push( const mat4& m ) {
        assert( _index + 1 < _size );
        _matrices[_index++] = m;
    }
    
    mat4& pop( void ) {
        assert( _index - 1 >= 0 );
        _index--;
        return _matrices[_index];
    }
};

MatrixStack  mvstack;
mat4         model_view;
GLint        uModelView, uProjection, uView;
GLint        uAmbient, uDiffuse, uSpecular, uLightPos, uShininess;
GLint        uTex, uEnableTex;



//-------------------------------------------------------------------------
//  Draws a string at the specified coordinates.
//-------------------------------------------------------------------------
void printw (float x, float y, float z, GLvoid *font_style, char* format, ...)
{

	// GLvoid *font_style = GLUT_BITMAP_HELVETICA_18;
    va_list arg_list;
    char str[256];
	int i;
    
    va_start(arg_list, format);
    vsprintf(str, format, arg_list);
    va_end(arg_list);
    
    glRasterPos3f (x, y, z);

    for (i = 0; str[i] != '\0'; i++)
        glutBitmapCharacter(font_style, str[i]);
}

// The eye point and look-at point.
// Currently unused. Use to control a camera with LookAt().
Angel::vec4 eye(0, 0.0, 10.0,1.0);
Angel::vec4 at(0.0, 0.0, 0.0,1.0);


float camerax = 5.0;
float cameray = 5.0;
float cameraz = 5.0;


double TIME = 0.0 ;

void drawCylinder(void)
{
	//? Isaac 
    
	glBindTexture( GL_TEXTURE_2D, texture_cylinder );
	glUniform1i( uEnableTex, 1 ); 
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
	glBindVertexArray( cylData.vao );
    glDrawArrays( GL_TRIANGLES, 0, cylData.numVertices );
	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);


    //glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_NEAREST);
	glUniform1i( uEnableTex, 1 );
}

void drawCone(void)
{
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( coneData.vao );
    glDrawArrays( GL_TRIANGLES, 0, coneData.numVertices );
}


//////////////////////////////////////////////////////
//    PROC: drawCube()
//    DOES: this function draws a cube with dimensions 1,1,1
//          centered around the origin.
//
// Don't change.
//////////////////////////////////////////////////////

void drawCube(int type)
{
    glBindTexture( GL_TEXTURE_2D, texture_cube );
    
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    if(type == 0){
        glUniform1i( uEnableTex, 1 );
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

        glBindVertexArray( cubeData.vao );
        glDrawArrays( GL_TRIANGLES, 0, cubeData.numVertices );
    }
    else if(type == 1){
        glUniform1i( uEnableTex, 0 );
        //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glBindVertexArray( cubeData_2.vao );
        glDrawArrays( GL_TRIANGLES, 0, cubeData_2.numVertices );
        
    }
        
    glUniform1i( uEnableTex, 0 );
}


void drawSphere(void)
{
    glBindTexture( GL_TEXTURE_2D, texture_earth);
    glUniform1i( uEnableTex, 1);
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( sphereData.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData.numVertices );
    glUniform1i( uEnableTex, 0 );
}

void drawSphere(int type)
{
    glBindTexture( GL_TEXTURE_2D, texture_earth1);
    glUniform1i( uEnableTex, 1);
    glUniformMatrix4fv( uModelView, 1, GL_TRUE, model_view );
    glBindVertexArray( sphereData1.vao );
    glDrawArrays( GL_TRIANGLES, 0, sphereData1.numVertices );
    glUniform1i( uEnableTex, 0 );
}

void resetArcball()
{
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}

void randomize(void)
{
	// Initiate the random offset matrix
	for(int i = 0; i < 9; i++)
	{
		rand_offset[i] = vec3(	((double)rand()/ (RAND_MAX+1)) * 2.0 + 2.0, 
								((double)rand()/ (RAND_MAX+1)) * 10.0 - 5.0, 
								((double)rand()/ (RAND_MAX+1)) * 8.0 - 4.0);
	}
}

void myKey(unsigned char key, int x, int y)
{
    float time ;
    switch (key) {
        case 'q':
        case 'Q':
        case 27:
            exit(0);
        case 's':
            FrSaver.DumpPPM(Width,Height) ;
            break;
        case 'r':
            resetArcball() ;
            glutPostRedisplay() ;
            break ;
		case 'b':
			for(int i=0; i < 9; i++)
			{
				collision[i] = true;
			}
			break ;
        case 'a': // toggle animation
            Animate = 1 - Animate ;
            // reset the timer to point to the current time
            time = TM.GetElapsedTime() ;
            TM.Reset() ;
            // printf("Elapsed time %f\n", time) ;
            break ;
        case 'm':
            if( Recording == 1 )
            {
                printf("Frame recording disabled.\n") ;
                Recording = 0 ;
            }
            else
            {
                printf("Frame recording enabled.\n") ;
                Recording = 1  ;
            }
            FrSaver.Toggle(Width);
            break ;
        case 'h':
        case '?':
            instructions();
            break;
           
    }
    glutPostRedisplay() ;
    
}



/*********************************************************
 PROC: myinit()
 DOES: performs most of the OpenGL intialization
 -- change these with care, if you must.
 
 **********************************************************/

void myinit(void)
{
    // Load shaders and use the resulting shader program
    GLuint program = InitShader( "vshader.glsl", "fshader.glsl" );
    glUseProgram(program);
    
    // Generate vertex arrays for geometric shapes
    generateCube(program, &cubeData, 0);
    generateCube(program, &cubeData_2, 1);
    generateSphere(program, &sphereData);
	generateSphere(program, &sphereData1);
    generateCone(program, &coneData);
    generateCylinder(program, &cylData);
    //generateCube(program, &cubeData, 0);
    uModelView  = glGetUniformLocation( program, "ModelView"  );
    uProjection = glGetUniformLocation( program, "Projection" );
    uView       = glGetUniformLocation( program, "View"       );
    
    glClearColor( 0.1, 0.1, 0.2, 1.0 ); // dark blue background
    
    uAmbient   = glGetUniformLocation( program, "AmbientProduct"  );
    uDiffuse   = glGetUniformLocation( program, "DiffuseProduct"  );
    uSpecular  = glGetUniformLocation( program, "SpecularProduct" );
    uLightPos  = glGetUniformLocation( program, "LightPosition"   );
    uShininess = glGetUniformLocation( program, "Shininess"       );
    uTex       = glGetUniformLocation( program, "Tex"             );
    uEnableTex = glGetUniformLocation( program, "EnableTex"       );
    
    glUniform4f(uAmbient,    0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uDiffuse,    0.6f,  0.6f,  0.6f, 1.0f);
    glUniform4f(uSpecular,   0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uLightPos,  15.0f, 15.0f, 30.0f, 0.0f);
    glUniform1f(uShininess, 100.0f);
    
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);  //? Isaac
    TgaImage coolImage;
	if (!coolImage.loadTGA("Planets.tga"))
	{
		printf("Error loading image file\n");
		exit(1);
	}

    TgaImage earthImage;
    if (!earthImage.loadTGA("glitter_bubble.tga"))
    {
        printf("Error loading image file\n");
        exit(1);
    }
 //? Isaac- make texture cylinder for gun file///////////////////////////
	TgaImage gunImage;
    if (!gunImage.loadTGA("Space1.tga"))
    {
        printf("Error loading image file\n");
        exit(1);
    }
	    
	glGenTextures( 1, &texture_cylinder );
    glBindTexture( GL_TEXTURE_2D, texture_cylinder );
    glTexImage2D(GL_TEXTURE_2D, 0, 4, gunImage.width, gunImage.height, 0,
                 (gunImage.byteCount == 3) ? GL_BGR : GL_BGRA,
                 GL_UNSIGNED_BYTE, gunImage.data );

    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
///////////////////////////////////////////////////////////////////////////  


    glGenTextures( 1, &texture_cube );
    glBindTexture( GL_TEXTURE_2D, texture_cube );
    glTexImage2D(GL_TEXTURE_2D, 0, 4, coolImage.width, coolImage.height, 0,
                 (coolImage.byteCount == 3) ? GL_BGR : GL_BGRA,
                 GL_UNSIGNED_BYTE, coolImage.data );

    
    glGenerateMipmap(GL_TEXTURE_2D);
    
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
       // glTexImage2D(GL_TEXTURE_2D, 0, 4, coolImage.width/2, coolImage.height/2, 0, (coolImage.byteCount == 3) ? GL_BGR : GL_BGRA, GL_UNSIGNED_BYTE, coolImage.data);
    
    glGenTextures( 1, &texture_earth );
    glBindTexture( GL_TEXTURE_2D, texture_earth );
    glTexImage2D(GL_TEXTURE_2D, 0, 4, earthImage.width, earthImage.height, 0,
                 (earthImage.byteCount == 3) ? GL_BGR : GL_BGRA,
                 GL_UNSIGNED_BYTE, earthImage.data );
    
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
    //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    //glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
    // Set texture sampler variable to texture unit 0
    // (set in glActiveTexture(GL_TEXTURE0))
	 glGenTextures( 1, &texture_earth1 );
    glBindTexture( GL_TEXTURE_2D, texture_earth1 );
    glTexImage2D(GL_TEXTURE_2D, 0, 4, gunImage.width, gunImage.height, 0,
                 (gunImage.byteCount == 3) ? GL_BGR : GL_BGRA,
                 GL_UNSIGNED_BYTE, gunImage.data );
    
    
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    
 
    
    glUniform1i( uTex, 0);
    
	srand((unsigned)time(NULL));

	randomize();

    Arcball = new BallData ;
    Ball_Init(Arcball);
    Ball_Place(Arcball,qOne,0.75);
}

/*********************************************************
 PROC: set_colour();
 DOES: sets all material properties to the given colour
 -- don't change
 **********************************************************/

void set_colour(float r, float g, float b)
{
    float ambient  = 0.2f;
    float diffuse  = 0.6f;
    float specular = 0.2f;
    glUniform4f(uAmbient,  ambient*r,  ambient*g,  ambient*b,  1.0f);
    glUniform4f(uDiffuse,  diffuse*r,  diffuse*g,  diffuse*b,  1.0f);
    glUniform4f(uSpecular, specular*r, specular*g, specular*b, 1.0f);
}

void display(void)
{
	DWORD curr_milliseconds = GetTickCount();
	DWORD offset = curr_milliseconds - root_milliseconds;
	DWORD offset2 = curr_milliseconds - root_milliseconds2;
	DWORD offset_next_level = curr_milliseconds - root_next_level;

	//printf("Seconds offset = %d %d %d \n", root_milliseconds, curr_milliseconds, offset);

    // Clear the screen with the background colour (set in myinit)
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    model_view = mat4(1.0f);
    
    
    //model_view *= Translate(0.0f, 0.0f, -15.0f);
    
    Ball_Value(Arcball,r);
    

    mat4 mat_arcball_rot(
                         r[0][0], r[0][1], r[0][2], r[0][3],
                         r[1][0], r[1][1], r[1][2], r[1][3],
                         r[2][0], r[2][1], r[2][2], r[2][3],
                         r[3][0], r[3][1], r[3][2], r[3][3]);


    //model_view *= mat_arcball_rot;
    
	// model_view *= Translate(0.0f, 0.0f, -60.0f);
	model_view *= global_camera.mat_view();
	model_view *= Translate(0.0f, -4.0f, 60.0f);

    mat4 view = model_view;
    
    
    //model_view = Angel::LookAt(eye, ref, up);//just the view matrix;
    
    glUniformMatrix4fv( uView, 1, GL_TRUE, model_view );
    
    // Previously glScalef(Zoom, Zoom, Zoom);
    model_view *= Scale(Zoom);
    
    // Draw Something
   // set_colour(0.8f, 0.8f, 0.8f);
  
    model_view *= Scale(camerax, cameray, cameraz);

	// WARP: Beginning of drawing objects
	

	// Draw Background
	model_view *= Translate(0.0f, 0.0f, -60.0f);

	model_view *= Scale(30.0f, 30.0f, 30.0f);
	drawCube(0);

	model_view *= Translate(1.2f, 0.0f, 0.5f);
	mat4 rotation = RotateY(45);
	model_view *= rotation;
	drawCube(0);

	rotation = RotateY(-45);
	model_view *= rotation;

	model_view *= Translate(-2.4f, 0.0f, 0.0f);
	rotation = RotateY(-45);
	model_view *= rotation;
	drawCube(0);

	rotation = RotateY(45);
	model_view *= rotation;
	model_view *= Translate(1.2, 0.0, -0.5f);

	// Draw the gun
	mat4 ph_model_view = model_view;

	if(shooting)
	{
		vec4 eye( 0.0f, 4.0f, 8.0f, 1.0);
		vec4 at( 0.0f, 3.0f, 0.0f, 1.0);
		vec4 up( 0.0f, 1.0f, 0.0f, 0.0f);
		model_view = LookAt( eye, at, up );
		drawSphere(1);
	
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Scale( 3.0f, 1.0f, 1.2f );
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
	}
	else 
	{
		vec4 eye( 0.0f, 4.0f, 10.0f, 1.0);
		vec4 at( 0.0f, 3.0f, 0.0f, 1.0);
		vec4 up( 0.0f, 1.0f, 0.0f, 0.0f);
		model_view = LookAt( eye, at, up );
		drawSphere(1);
	
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
		model_view *= Scale( 3.0f, 1.0f, 1.2f );
		model_view *= Translate( 0.0f, 0.0f, 0.5f );
		drawSphere(1);
	}
	
	// Slow down gun animation
	if(offset > 200)
	{
		shooting = false;
	}
    eye = vec4( 0.0, 0.0, 2.0, 1.0 );
    at = vec4( 0.0, 0.0, 0.0, 1.0 );

    // Draw the crosshair
    glUniform4f(uAmbient,    1.0f,  1.0f,  1.0f, 1.0f);
    glUniform4f(uDiffuse,    0,  0,  0, 1.0f);
    glUniform4f(uSpecular,   0,  0,  0, 1.0f);
    eye = vec4( 0.0f, 0.0f, 40.0f, 1.0f);
    at = vec4( 0.0f, 0.0f, 39.0f, 1.0f);
    model_view = LookAt( eye, at, up );
    model_view *= Scale( 0.2f, 0.2f, 0.2f );
    model_view *= Translate( 0.0f, -5.0f, 20.0f );
    drawCube(1); //middle
    model_view *= Translate( 0.0f, 1.0f, 0.0f );
    drawCube(1); //top 1
    model_view *= Translate( 0.0f, 1.0f, 0.0f );
    drawCube(1); //top 2
    model_view *= Translate( 1.0f, -2.0f, 0.0f );
    drawCube(1); //right 1
    model_view *= Translate( 1.0f, 0.0f, 0.0f );
    drawCube(1); //right 2
    model_view *= Translate( -3.0f, 0.0f, 0.0f );
    drawCube(1); //left 1
    model_view *= Translate( -1.0f, 0.0f, 0.0f );
    drawCube(1); //left 2
    model_view *= Translate( 2.0f, -1.0f, 0.0f );
    drawCube(1); //bottom 1
    model_view *= Translate( 0.0f, -1.0f, 0.0f );
    drawCube(1); //bottom 2

	// Print Next Level message
	if(next_level)
	{
		
		std::string accbonus;
		// Add incremental bonus display
		if(offset_next_level <= 1500)
		{
			int bonus_mod = bonus * (float)offset_next_level/1500.0;
			accbonus = std::to_string(bonus_mod);
		}
		else
		{
			accbonus = std::to_string(bonus);
		}
		char const *_accuracybonus = accbonus.c_str();
		printw (-15, 15, 0, GLUT_BITMAP_TIMES_ROMAN_24, "Next Level !");
		printw (-25, -5, 0, GLUT_BITMAP_TIMES_ROMAN_24, "Accuracy Bonus: %s", _accuracybonus);
	}

	if(offset_next_level > 2500)
	{
		next_level = false;
	}

	// Print Scores
	std::string l = std::to_string(level);
	char const *_level = l.c_str();
	std::string s = std::to_string(score);
	char const *_score = s.c_str();
	std::string a = std::to_string(accuracy);
	char const *_accuracy = a.c_str();

	printw (15, 80, 0, GLUT_BITMAP_HELVETICA_18, "Level: %s   Score: %s   Acc: %s %%", _level, _score, _accuracy);

    // Reset the color
    glUniform4f(uAmbient,    0.2f,  0.2f,  0.2f, 1.0f);
    glUniform4f(uDiffuse,    0.6f,  0.6f,  0.6f, 1.0f);
    glUniform4f(uSpecular,   0.2f,  0.2f,  0.2f, 1.0f);

	model_view = ph_model_view;
	
	// Draw Bubbles
	model_view *= Scale(1.0/30.0f, 1.0/30.0f, 1.0/30.0f);
	
	model_view *= Translate(0.0f, -4.0f, 30.0f); // Move to bottom and front of background
	model_view *= Scale(1.0f/5.0f, 1.0f/5.0f, 1.0f/5.0f);

	// Variables

	float theta = 0.0 ; // Sideways angle of bullet
	theta = theta*PI/180 - b_y_rot;

	float phi = 0.0 ; // Vertical angle of bullet
	phi = phi*PI/180 + b_x_rot;

	float height = 20.0; 

	float vert = (speed*offset2 % 40000); // y bounds -> -20 to 20
	float horiz = (100*offset) % 200000; // Magnitude of hypotenuse

	// Updating accuracy value
	if(shots != 0)
			accuracy = (int)((float)hits/(float)shots * 100.0);
	else
			accuracy = 0;

	// ** Draw the bullet
	vec3 bullet(0.0f, 0.0f, 1000.f);

    if(offset <= 2000)
    { 
    	float dist = horiz/300;
		ztrans = dist;
		ytrans = dist*sin(-b_y_rot - PI);
		xtrans = dist*sin(b_x_rot - PI/2.0); 

    	model_view *= Translate(xtrans, height + ytrans, 120.0-ztrans);
    	drawSphere();
    	bullet.x = xtrans/5.0;
		bullet.y = ytrans/5.0;
		bullet.z = (120.0 - ztrans)/5.0 - 20.0;

    	// Reset back to normal state for bubbles
    	model_view *= Translate(-xtrans, -height - ytrans, -120.0+ztrans);
    }
	
	// ** End of bullet transformation


	// Scale to bubble size
	model_view *= Scale(5.0f, 5.0f, 5.0f);

	// All bubbles move up at the same speed
	model_view *= Translate(-20.0f, -16+vert/1000, 0.0f);

	vec3 bubble(-20.0, -20.0 + vert/1000, -20.0);

	bool level_cleared = true;

	for(int i=0; i < 9; i++)
	{
		bubble += rand_offset[i];
		model_view *= Translate(rand_offset[i]);
			
		if(collision[i] == false)
		{
			level_cleared = false;

			// Calculate distance between bullet and bubble
			float dist = (bubble.x - bullet.x)*(bubble.x - bullet.x) + (bubble.y - bullet.y)*(bubble.y - bullet.y) + (bubble.z - bullet.z)*(bubble.z - bullet.z);
			if (dist < 2.0)
			{
				// printf("COLLISION: Bubble %d: x %4.2f y %4.2f z %4.2f Bullet x %4.2f y %4.2f z %4.2f\n", i, bubble.x, bubble.y, bubble.z, bullet.x, bullet.y, bullet.z);
				collision[i] = true;
				score += 100 * level;
				hits++;
				sound2 ->Play(); // Collision sound
				break;
			}
			drawSphere();
		}
	}

	if(level_cleared)
	{
		bonus = level * accuracy * 10;
		level++;
		randomize();
		speed += 2;
		background = (background + 1) % 4;
		root_next_level = GetTickCount();
		next_level = true;
		TgaImage coolImage;

		char* backName;
		switch(background)
		{
			case 0:
				backName = "Space.tga";
				break;
			case 1:
				backName = "Space1.tga";
				break;
			case 2:
				backName = "Space2.tga";
				break;
			case 3:
				backName = "Space3.tga";
				break;
		}

		if(!coolImage.loadTGA(backName))
		{
			printf("Error loading image file\n");
			exit(1);
		}

		glGenTextures( 1, &texture_cube );
		glBindTexture( GL_TEXTURE_2D, texture_cube );
		glTexImage2D(GL_TEXTURE_2D, 0, 4, coolImage.width, coolImage.height, 0,
					 (coolImage.byteCount == 3) ? GL_BGR : GL_BGRA,
					 GL_UNSIGNED_BYTE, coolImage.data );

		
		glGenerateMipmap(GL_TEXTURE_2D);
    
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
		glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

		root_milliseconds2 = GetTickCount();
		score += bonus;

		for(int i=0; i < 9; i++)
		{
			collision[i] = false;
		}
		hits = 0;
		shots = 0;
	}

    glutSwapBuffers();
    if(Recording == 1)
        FrSaver.DumpPPM(Width, Height) ;
}

/**********************************************
 PROC: myReshape()
 DOES: handles the window being resized
 
 -- don't change
 **********************************************************/

void myReshape(int w, int h)
{
    Width = w;
    Height = h;
    
    glViewport(0, 0, w, h);
    
    mat4 projection = Perspective(50.0f, (float)w/(float)h, 1.0f, 1000.0f);
    glUniformMatrix4fv( uProjection, 1, GL_TRUE, projection );
}

void instructions()
{
	printf("***** Welcome to Bubble Arena! *****\n");
    printf("Press:\n");
    printf("  s to save the image\n");
	printf("  b to skip ahead to the next level\n") ;
    printf("  a to toggle the animation.\n") ;
    printf("  m to toggle frame dumping.\n") ;
    printf("  q to quit.\n");
}

// start or end interaction
void myMouseCB(int button, int state, int x, int y)
{
    Button = button ;
    if( Button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
    {
        Button = -1 ;
		begin_game = true;
		initx = x;
		inity = y;
		root_milliseconds = GetTickCount();
		b_x_rot = x_rot_angle;
		b_y_rot = y_rot_angle;
		shots++;
		sound ->Play(); // Gun shot sound
    }   

    if( Button == GLUT_LEFT_BUTTON && state == GLUT_UP)
	{
		shooting = true;
	}

    // Tell the system to redraw the window
    glutPostRedisplay() ;
}

// interaction (mouse motion)
void myMotionCB(int x, int y)
{
	if (begin_game)
	{
		global_camera.move(x,y);
        glutPostRedisplay() ;
	}
}


void idleCB(void)
{
    if( Animate == 1 )
    {
        // TM.Reset() ; // commenting out this will make the time run from 0
        // leaving 'Time' counts the time interval between successive calls to idleCB
        if( Recording == 0 )
            TIME = TM.GetElapsedTime() ;
        else
            TIME += 0.033 ; // save at 30 frames per second.

        
        eye.x = 20*sin(TIME);
        eye.z = 20*cos(TIME);
        TIME = TM.GetElapsedTime() ;
        
        DTIME = TIME - TIME_LAST;
        TIME_LAST = TIME;
        angle += DTIME*360;
        glutPostRedisplay() ;
    }
	glutPostRedisplay();
}
/*********************************************************
 PROC: main()
 DOES: calls initialization, then hands over control
 to the event handler, which calls
 display() whenever the screen needs to be redrawn
 **********************************************************/

int main(int argc, char** argv)
{
	root_milliseconds = GetTickCount();
	root_milliseconds2 = GetTickCount();
    glutInit(&argc, argv);
    // If your code fails to run, uncommenting these lines may help.
    //glutInitContextVersion(3, 2);
    //glutInitContextProfile(GLUT_CORE_PROFILE);
    glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowPosition (0, 0);
    glutInitWindowSize(Width,Height);


	// OpenAL initialization
	alDevice = alcOpenDevice(NULL);
	if(!alDevice) {
		std::cout << "Couldn't Setup OpenAL Device" << std::endl;
		return 1;
	}
	alContext = alcCreateContext(alDevice, NULL);
	alcMakeContextCurrent(alContext);


	sound = Sound::loadWAVE("gun.wav", true);
	sound2 = Sound::loadWAVE("pop.wav", true);

    glutCreateWindow(argv[0]);
    printf("GL version %s\n", glGetString(GL_VERSION));
//    glewExperimental = GL_TRUE;
    glewInit();
    
    myinit();
    
    glutIdleFunc(idleCB) ;
    glutReshapeFunc (myReshape);
    glutKeyboardFunc( myKey );
    glutMouseFunc(myMouseCB) ;
    glutMotionFunc(myMotionCB) ;
	glutPassiveMotionFunc(myMotionCB);
	glutSetCursor(GLUT_CURSOR_NONE); 
    instructions();
    
    glutDisplayFunc(display);
    glutMainLoop();
    
    TM.Reset() ;
    return 0;         // never reached
}




