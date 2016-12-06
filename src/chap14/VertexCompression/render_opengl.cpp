#include <windows.h>

#include "glew.h" // OpenGL extension
// Standard OpenGL header
#include <GL/gl.h>

#include "Gut.h"
#include "GutWin32.h"
#include "render_data.h"

static Matrix4x4 g_proj_matrix;

static GLuint g_VertexShader = 0;
static GLuint g_PixelShader = 0;
static GLuint g_Shader = 0;

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

	// ͶӰ����
	g_proj_matrix = GutMatrixPerspectiveRH_OpenGL(90.0f, 1.0f, 0.1f, 100.0f);
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &g_proj_matrix);

	glMatrixMode(GL_MODELVIEW);	
	glEnable(GL_DEPTH_TEST);
	
	g_VertexShader = GutLoadVertexShaderOpenGL_GLSL("../../shaders/VertexCompression.glvs");
	g_PixelShader = GutLoadFragmentShaderOpenGL_GLSL("../../shaders/VertexCompression.glfs");
	g_Shader = GutCreateProgram(g_VertexShader, g_PixelShader);
	
	return true;
}

bool ReleaseResourceOpenGL(void)
{
	glDeleteShader(g_VertexShader);
	glDeleteShader(g_PixelShader);
	glDeleteProgram(g_Shader);

	return true;
}

// callback function. ���ڴ�С�ı�ʱ�ᱻ����, �������µĴ��ڴ�С.
void ResizeWindowOpenGL(int width, int height)
{
	// ʹ���µĴ��ڴ�С��Ϊ�µĻ�ͼ�ֱ���
	glViewport(0, 0, width, height);
	// ͶӰ����, ����ˮƽ�ʹ�ֱ������ӽ�.
	float aspect = (float) height / (float) width;
	g_proj_matrix = GutMatrixPerspectiveRH_OpenGL(60.0f, aspect, 0.1f, 100.0f);
	// �����ӽ�ת������
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &g_proj_matrix);
}

// ʹ��OpenGL����ͼ
void RenderFrameOpenGL(void)
{
	// �������
	glClearColor(0.4f, 0.4f, 0.4f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	// ʹ�þ���
	Matrix4x4 object_matrix = g_Control.GetObjectMatrix();
	Matrix4x4 view_matrix = g_Control.GetViewMatrix();
	Matrix4x4 wv_matrix = object_matrix * view_matrix;

	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( (float *) &g_proj_matrix);
	glMatrixMode(GL_MODELVIEW);
	glLoadMatrixf( (float *) &wv_matrix);
	glMatrixMode(GL_TEXTURE);
	glLoadMatrixf( (float *) &object_matrix);
	// ʹ��shader
	glUseProgram(g_Shader);
	// ���ö���������Դ
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CompressedVertex), g_pCompressedVertices[0].m_Position);
	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(CompressedVertex), g_pCompressedVertices[0].m_Normal);
	// ����
	glDrawElements(GL_TRIANGLES, g_iNumSphereIndices, GL_UNSIGNED_SHORT, g_pSphereIndices);
	// �ѱ���backbuffer�Ļ�����ʾ����
	GutSwapBuffersOpenGL();
}
