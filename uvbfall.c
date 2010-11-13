#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <inttypes.h>
#include <math.h>
#include <complex.h> 
#include <fftw3.h>

#include <SDL/SDL.h>
#include <GL/gl.h>
#include <GL/glu.h>

uint16_t *raw;
fftw_complex *in,
			 *out;
fftw_plan plan;

uint8_t* tex_ptr;

uint8_t init_opengl(int res_x, int res_y)
{
	if(SDL_Init(SDL_INIT_VIDEO) != -1)
	{
		SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );
		SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 24 );
		SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
		
		if(SDL_SetVideoMode(res_x, res_y, 32, SDL_ANYFORMAT|SDL_OPENGL))
		{
			glEnable(GL_TEXTURE_2D);
			glViewport(0,0,res_x,res_y);
			gluOrtho2D(0,res_x,0,res_y);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			return 1;
		}
	}
	return 0;
}

void draw_frame(int len, int res_x, int res_y)
{
		if (!fread(raw, sizeof(uint16_t), len, stdin))
			exit(1);
		for (int i = 0; i < len; i++)
    		in[i] = ((float)raw[i] / 65535.0) - 1;
		fftw_execute(plan);
		memmove(tex_ptr+res_x, tex_ptr, res_x*(res_y-1));
		tex_ptr[0] = 0;

		for (int i = 1; i < len/2; i++)
			tex_ptr[i] = cabs(out[i] + out[len-i]) / (float) len * 255.0;
		glTexImage2D(GL_TEXTURE_2D,0,GL_LUMINANCE,
						len/2,len/2,0,
						GL_LUMINANCE,
						GL_UNSIGNED_BYTE, tex_ptr);
		glClear(GL_COLOR_BUFFER_BIT);
		glBegin(GL_QUADS);
			glTexCoord2f(0.0,0.0); glVertex2f(0.0,0.0);
			glTexCoord2f(1.0,0.0); glVertex2f(res_x,0.0);
			glTexCoord2f(1.0,1.0); glVertex2f(res_x,res_y);
			glTexCoord2f(0.0,1.0); glVertex2f(0.0,res_y);
		glEnd();
		SDL_GL_SwapBuffers(); 
}

int main(int argc, char* argv[])
{
	int len;
	int res_x = 0;
	int res_y = 0;
	if(argc < 2)
	{
			printf("Usage: %s <DFT length> [-xNNN -yNNN]", argv[0]);
			return -1;
	}
	for(int a = 1; a < argc; a++)
	{
		if(((char*)argv[a])[0] == '-')
		{
			char dim;
			int res;
			sscanf((char*)argv[a],"-%c%u",&dim,&res);
			switch(dim)
			{
				case 'x': res_x = res; break;
				case 'y': res_y = res; break;
				default: printf("Unknown dimension: %c", dim);
						 return -2;
			}
		}
		else
		{
			len = atoi(argv[a]);
		}
	}

	if((res_x == 0) && (res_y == 0))
	{
		res_x = res_y = len/2;
	}

	if((res_x == 0) || (res_y == 0))
	{
		puts("Only one dimension specified.");
		return -3;
	}

	raw = calloc(len, sizeof(uint16_t));
	in = fftw_malloc(sizeof(fftw_complex) * len);
	out = fftw_malloc(sizeof(fftw_complex) * len);
	plan = fftw_plan_dft_1d(len, in, out, FFTW_FORWARD, FFTW_MEASURE);
	tex_ptr = calloc(len * len /2, sizeof(uint8_t));

	if(!init_opengl(res_x,res_y))
	{
		puts("OpenGL init failed.");
		return -4;
	}

	while(1)
	{
			draw_frame(len, res_x, res_y);
	}

	fftw_destroy_plan(plan);
	fftw_free(in);
	fftw_free(out);
	free(raw);
	free(tex_ptr);
}

