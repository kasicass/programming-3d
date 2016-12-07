#include "Vector4.h"
#include "Matrix4x4.h"

#include "GutUserControl.h"
#include "GutModel.h"

struct Vertex_VT
{
	float m_Position[3]; // ����λ��
	float m_Texcoord[2]; // ��ͼ����
};

#define KERNELSIZE 9

extern float g_uv_offset_table[KERNELSIZE];
extern float g_weight_table[KERNELSIZE];

//
extern bool g_bPosteffect;
// ����
extern Vertex_VT g_FullScreenQuad[4];
