#include <windows.h>

#include "glew.h" // OpenGL extension
// Standard OpenGL header
#include <GL/gl.h>

#include "Gut.h"
#include "GutOpenGL.h"
#include "GutWin32.h"
#include "GutTexture_OpenGL.h"

#include "render_data.h"

static GLuint g_texture = 0;

static GLuint g_BlurVS = 0;
static GLuint g_BlurPS = 0;
static GLuint g_BlurProgram = 0;

static GLuint g_BrightnessPS = 0;
static GLuint g_BrightnessProgram = 0;

static sImageInfo g_ImageInfo;

static GLuint g_framebuffer[2] = {0, 0};
static GLuint g_frametexture[2] = {0, 0};

// One time Init
bool InitResourceOpenGL(void)
{
	if ( NULL==glCreateProgram )
	{
		printf("Driver does not support GLSL\n");
		return false;
	}
	if ( NULL==glBindFramebufferEXT )
	{
		printf("Driver does not support framebuffer object\n");
		return false;
	}

	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	glEnable(GL_TEXTURE_2D);

	g_texture = GutLoadTexture_OpenGL("../../textures/space.tga", &g_ImageInfo);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	if ( g_texture==0 )
		return false;

	// 载入Vertex Shader
	g_BlurVS = GutLoadVertexShaderOpenGL_GLSL("../../shaders/posteffect_blur.glvs");
	// 载入Pixel Shader
	g_BlurPS = GutLoadFragmentShaderOpenGL_GLSL("../../shaders/posteffect_blur.glfs");
	if ( 0==g_BlurVS || 0==g_BlurPS )
		return false;
	// 创建Shader Program对象
	g_BlurProgram = glCreateProgram();
	if ( 0==g_BlurProgram )
		return false;
	// 把Vertex Shader代入g_BlurProgram里
	glAttachShader(g_BlurProgram, g_BlurVS);
	// 把Pixel Shader代入g_BlurProgram里
	glAttachShader(g_BlurProgram, g_BlurPS);
	// 把g_BlurProgram中的Vertex Shader和Pixel Shader连结起来
	glLinkProgram(g_BlurProgram);

	g_BrightnessPS = GutLoadFragmentShaderOpenGL_GLSL("../../shaders/posteffect_brightness.glfs");
	if ( 0==g_BrightnessPS )
		return false;

	g_BrightnessProgram = GutCreateProgram(g_BlurVS, g_BrightnessPS);
	if ( 0==g_BrightnessProgram )
		return false;

	int w = g_ImageInfo.m_iWidth/4;
	int h = g_ImageInfo.m_iHeight/4;

	if ( !GutCreateRenderTargetOpenGL(w, h, GL_RGBA8, &g_framebuffer[0], &g_frametexture[0]) )
		return false;

	if ( !GutCreateRenderTargetOpenGL(w, h, GL_RGBA8, &g_framebuffer[1], &g_frametexture[1]) )
		return false;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	return true;
}

bool ReleaseResourceOpenGL(void)
{
	if ( g_texture )
	{
		glDeleteTextures(1, &g_texture);
		g_texture = 0;
	}

	glDeleteShader(g_BlurVS);
	glDeleteShader(g_BlurPS);
	glDeleteProgram(g_BlurProgram);

	glDeleteShader(g_BrightnessPS);
	glDeleteShader(g_BrightnessProgram);

	for ( int i=0; i<2; i++ )
	{
		glDeleteFramebuffersEXT(1, &g_framebuffer[i]);
		glDeleteTextures(1, &g_frametexture[i]);
	}

	return true;
}

// callback function. 窗口大小改变时会被调用, 并传入新的窗口大小.
void ResizeWindowOpenGL(int width, int height)
{
	// 使用新的窗口大小做为新的绘图分辨率
	glViewport(0, 0, width, height);
}

static GLuint Brightness(GLuint texture, sImageInfo *pInfo)
{
	int w = pInfo->m_iWidth/4;
	int h = pInfo->m_iHeight/4;
	// `对动态贴图来画面`
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_framebuffer[1]);
	glViewport(0, 0, w, h);
	// `使用对亮度做变化的Shader`
	glUseProgram(g_BrightnessProgram);
	// `使用贴图`
	glBindTexture(GL_TEXTURE_2D, texture);
	// `设置亮度变化参数`
	GLint reg_offset = glGetUniformLocation(g_BrightnessProgram, "IntensityOffset");
	GLint reg_scale = glGetUniformLocation(g_BrightnessProgram, "IntensityScale");
	glUniform4fv(reg_offset, 1, (float *)&g_vBrightnessOffset);
	glUniform4fv(reg_scale, 1, (float *)&g_vBrightnessScale);
	// `设置数据数据格式`
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuad[0].m_Position);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuadInv[0].m_Texcoord);
	// 
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return g_frametexture[1];
}

static GLuint BlurImage(GLuint texture, sImageInfo *pInfo)
{	
	int w = pInfo->m_iWidth/4;
	int h = pInfo->m_iHeight/4;

	float fTexelW = 1.0f/(float)w;
	float fTexelH = 1.0f/(float)h;
	
	const int num_samples = KERNELSIZE;

	Vector4 vTexOffsetX[num_samples];
	Vector4 vTexOffsetY[num_samples];

	for ( int i=0; i<num_samples; i++ )
	{
		vTexOffsetX[i].Set(g_uv_offset_table[i] * fTexelW, 0.0f, 0.0f, g_weight_table[i]);
		vTexOffsetY[i].Set(0.0f, g_uv_offset_table[i] * fTexelH, 0.0f, g_weight_table[i]);
	}

	glUseProgram(g_BlurProgram);

	GLint reg = glGetUniformLocation(g_BlurProgram, "vTexOffset");

	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuad[0].m_Position);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuadInv[0].m_Texcoord);

	// `水平方向模糊`
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_framebuffer[0]);
		glViewport(0, 0, w, h);
		glBindTexture(GL_TEXTURE_2D, texture);
		if ( reg>=0 )
			glUniform4fv(reg, num_samples, (float *)vTexOffsetX);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
	// `垂直方向模糊`
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, g_framebuffer[1]);
		glBindTexture(GL_TEXTURE_2D, g_frametexture[0]);
		if ( reg>=0 )
			glUniform4fv(reg, num_samples, (float *)vTexOffsetY);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

	return g_frametexture[1];
}

// `使用OpenGL来绘图`
void RenderFrameOpenGL(void)
{
	int w, h;
	GutGetWindowSize(w, h);
	glViewport(0, 0, w, h);
	// `清除画面`
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// `不使用` Shader
	glUseProgram(0);
	// `设置纹理, 使用贴图.`
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
	glBindTexture(GL_TEXTURE_2D, g_texture);
	// `设置顶点数据格式`
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuad[0].m_Position);
	glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuad[0].m_Texcoord);
	
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

	if ( g_bPosteffect )
	{
		// `取出图片中偏亮的部分`
		GLuint texture = Brightness(g_texture, &g_ImageInfo);
		// `对图片做模糊化`
		texture = BlurImage(texture, &g_ImageInfo);
		// `使用主画面`
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		glViewport(0, 0, w, h);
		// `不使用` Shader
		glUseProgram(0);
		// `设置纹理, 使用动态贴图.`
		glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
		glBindTexture(GL_TEXTURE_2D, texture);
		// `启动混合功能`
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
		// `设置顶点数据格式`
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);
		glVertexPointer(3, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuad[0].m_Position);
		glTexCoordPointer(2, GL_FLOAT, sizeof(Vertex_VT), g_FullScreenQuad[0].m_Texcoord);
		// `画出看板`
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		// `关闭混合`
		glDisable(GL_BLEND);
	}
	
	GutSwapBuffersOpenGL();
}
