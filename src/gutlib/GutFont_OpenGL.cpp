#include "Gut.h"
#include "GutOpenGL.h"
#include "GutTexture_OpenGL.h"
#include "GutModel_OpenGL.h"
#include "GutFont_OpenGL.h"

CGutFontOpenGL::CGutFontOpenGL(void)
{
	m_FontTexture = 0;
}

CGutFontOpenGL::~CGutFontOpenGL()
{
}

bool CGutFontOpenGL::LoadTexture(const char *filename)
{
	m_FontTexture = GutLoadTexture_OpenGL(filename);
	return m_FontTexture ? true : false;
}

void CGutFontOpenGL::Render(void)
{
	if ( m_iNumCharacters==0 )
		return;

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	// 使用字体贴图
	sModelMaterial_OpenGL mtl;
	mtl.m_bCullFace = false;
	mtl.m_Textures[0] = m_FontTexture;
	mtl.Submit(NULL);
	// 打开alpha test
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	// 使用正交视图
	Matrix4x4 proj_matrix = GutMatrixOrthoRH_OpenGL(m_fWidth, m_fHeight, 0.0f, 1.0f);
	Matrix4x4 view_matrix; view_matrix.Identity();
	view_matrix[3].Set(-m_fWidth/2.0f, -m_fHeight/2.0f, 0.0f, 1.0f);
	// 使用转换矩阵
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf( (float *) &proj_matrix);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf( (float *) &view_matrix);
	// 设置vertex array来源
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(_FontVertex), &m_pVertexArray[0].m_Position);
	glTexCoordPointer(2, GL_FLOAT, sizeof(_FontVertex), &m_pVertexArray[0].m_Texcoord);
	// 画出所有文字
	glDrawElements(GL_TRIANGLES, m_iNumCharacters * 6, GL_UNSIGNED_SHORT, m_pIndexArray);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopClientAttrib();
}

CGutFontUniCodeOpenGL::CGutFontUniCodeOpenGL(void)
{
	m_FontTexture = 0;
}

CGutFontUniCodeOpenGL::~CGutFontUniCodeOpenGL()
{
}

bool CGutFontUniCodeOpenGL::CreateTexture(int w, int h)
{
	m_iTextureW = w;
	m_iTextureH = h;

	char *p = new char[w*h];
	ZeroMemory(p, w*h);

	glGenTextures(1, &m_FontTexture);
	glBindTexture(GL_TEXTURE_2D, m_FontTexture);

	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER, GL_LINEAR );

	glTexImage2D(GL_TEXTURE_2D, 0, GL_ALPHA, w, h, 0, GL_ALPHA, GL_UNSIGNED_BYTE, p);

	delete [] p;

	return m_FontTexture ? true : false;
}

bool CGutFontUniCodeOpenGL::AccessTexture(WCHAR c, int &x, int &y)
{
	bool bUpdateTexture = CGutFontUniCode::AccessTexture(c, x, y);
	
	if ( bUpdateTexture )
	{
		float tX = (float)x / (float)m_iLayoutW;
		float tY = (float)y / (float)m_iLayoutH;
		float tW = 1.0f/(float)m_iLayoutW;
		float tH = 1.0f/(float)m_iLayoutH;
		
		int left   = (int)(tX * m_iTextureW);
		int width  = (int)(tW * m_iTextureW);
		int right  = left + width;
		int top    = (int)(tY * m_iTextureH);
		int height = (int)(tH * m_iTextureH);
		int bottom = top + height;

		unsigned char *pBuffer = new unsigned char[width*height];
		unsigned char *buffer = pBuffer;

		for ( int y=0; y<height; y++ )
		{
			for ( int x=0; x<width; x++ )
			{
				COLORREF rgb = GetPixel(m_MemDC, x, y);

				int grayscale = GetRValue(rgb) + GetGValue(rgb) + GetBValue(rgb);
				grayscale /= 3;

				*buffer++ = grayscale;
			}
		}

		glBindTexture(GL_TEXTURE_2D, m_FontTexture);
		glTexSubImage2D(GL_TEXTURE_2D, 0, left, top, width, height, GL_ALPHA,  GL_UNSIGNED_BYTE, pBuffer);

		delete [] pBuffer;
	}

	return true;
}

void CGutFontUniCodeOpenGL::Render(void)
{
	if ( m_iNumCharacters==0 )
		return;

	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	// 使用字体贴图
	sModelMaterial_OpenGL mtl;
	mtl.m_bCullFace = false;
	mtl.m_Textures[0] = m_FontTexture;
	mtl.Submit(NULL);
	// 打开alpha test
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.5f);
	// 使用正交视图
	Matrix4x4 proj_matrix = GutMatrixOrthoRH_OpenGL(m_fWidth, m_fHeight, 0.0f, 1.0f);
	Matrix4x4 view_matrix; view_matrix.Identity();
	view_matrix[3].Set(-m_fWidth/2.0f, -m_fHeight/2.0f, 0.0f, 1.0f);
	// 使用转换矩阵
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadMatrixf( (float *) &proj_matrix);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glLoadMatrixf( (float *) &view_matrix);
	// 设置vertex array来源
	glEnableClientState(GL_VERTEX_ARRAY);
	glEnableClientState(GL_TEXTURE_COORD_ARRAY);
	glVertexPointer(3, GL_FLOAT, sizeof(_FontVertex), &m_pVertexArray[0].m_Position);
	glTexCoordPointer(2, GL_FLOAT, sizeof(_FontVertex), &m_pVertexArray[0].m_Texcoord);
	// 画出所有文字
	glDrawElements(GL_TRIANGLES, m_iNumCharacters * 6, GL_UNSIGNED_SHORT, m_pIndexArray);

	glMatrixMode(GL_PROJECTION);
	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);
	glPopMatrix();

	glPopClientAttrib();
}
