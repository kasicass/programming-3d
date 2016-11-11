#include <stdio.h>
#include <conio.h>
#include <math.h>

#include "Gut.h"
#include "GutInput.h"
#include "GutTimer.h"

#include "render_dx9.h"
#include "render_dx10.h"
#include "render_opengl.h"
#include "render_data.h"

GutTimer g_Timer;
float g_fFrame_Time = 0.0f;
float g_fTimeElapsed = 0.0f;
float g_fAnimation = 1.0f;

void GetUserInput(void)
{
	g_fFrame_Time = g_Timer.Stop();
	g_Timer.Restart();
	GutReadKeyboard();
	g_Control.Update(g_fFrame_Time, CGutUserControl::CONTROLLER_ROTATEOBJECT);
	//g_Control.Update(g_fFrame_Time, CGutUserControl::CONTROLLER_FPSCAMERA);
}

void KeyDown_1(void)
{
	if ( g_iMode & 0x01 )
	{
		g_iMode &= ~(0x01);
	}
	else
	{
		g_iMode |= 0x01;
	}
}

void KeyDown_2(void)
{
	if ( g_iMode & 0x02 )
	{
		g_iMode &= ~(0x02);
	}
	else
	{
		g_iMode |= 0x02;
	}
}

void KeyDown_3(void)
{
	if ( g_iMode & 0x04 )
	{
		g_iMode &= ~(0x04);
	}
	else
	{
		g_iMode |= 0x04;
	}
}

void KeyDown_Space(void)
{
	g_fAnimation = g_fAnimation > 0 ? 0 : 1.0f;
}

float rnd(void)
{
	float r = float(rand()%1024)/1023.0f;
	return r;
}

void frame_move(void)
{
	Vector4 vPos(0.0f, 0.0f, 1.05f, 1.0f);
	
	float speedA = 0.1f;
	float speedB = 0.2f;
	float speed = speedB * g_fTimeElapsed * MATH_PI;

	// 重设 seed
	srand(1234);

	// stateless particle
	// 从 random number 数列来重建出每个光源的位置和颜色数据

	for ( int i=1; i<NUM_LIGHTS; i++ )
	{
		sGutLight *pLight = &g_Lights[i];
		
		float rx = rnd() * MATH_PI;
		float ry = rnd() * MATH_PI * 2;
		float rz = rnd() * MATH_PI;
		float spin = (rnd()+speedA) * speed;

		Matrix4x4 local_mat; 

		local_mat.RotateX_Replace(rx);
		local_mat.RotateY(ry);
		local_mat.RotateZ(rz);

		Matrix4x4 rot_mat;
		rot_mat.RotateY_Replace(spin);

		pLight->m_vPosition = vPos * rot_mat * local_mat * g_Control.GetObjectMatrix();
		
		pLight->m_vAmbient.Set(0.0f);

		pLight->m_vDiffuse[0] = rnd()*3.0f + 0.5f;
		pLight->m_vDiffuse[1] = rnd()*3.0f + 0.5f;
		pLight->m_vDiffuse[2] = rnd()*3.0f + 0.5f;
		pLight->m_vDiffuse[3] = rnd()*3.0f + 0.5f;

		// 把位置和颜色放在放在另一个数组中, 方便绘图.
		g_LightsVC[i].m_Position = pLight->m_vPosition;
		g_LightsVC[i].m_Color = pLight->m_vDiffuse;
		
		// 光源范围放在W
		pLight->m_vPosition[3] = rnd()*0.2f + 0.1f;
	}

	g_fTimeElapsed += g_fFrame_Time * g_fAnimation;
}

void main(void)
{
	// 默认使用DirectX 9来绘图
	char *device = "dx9";
	void (*render)(void) = RenderFrameDX9;
	bool (*init_resource)(void) = InitResourceDX9;
	bool (*release_resource)(void) = ReleaseResourceDX9;
	void (*resize_func)(int width, int height) = ResizeWindowDX9;

#ifdef _ENABLE_DX10_
	printf("Press\n(1) for Direct3D9\n(2) for OpenGL\n(3) for Direct3D10\n");
#else
	printf("Press\n(1) for Direct3D9\n(2) for OpenGL\n");
#endif
	int c = _getche();

	switch(c)
	{
	default:
	case '1':
		render = RenderFrameDX9;
		init_resource = InitResourceDX9;
		release_resource = ReleaseResourceDX9;
		resize_func = ResizeWindowDX9;
		break;
	case '2':
		device = "opengl";
		init_resource = InitResourceOpenGL;
		release_resource = ReleaseResourceOpenGL;
		render = RenderFrameOpenGL;
		resize_func = ResizeWindowOpenGL;
		break;
	case '3':
	#ifdef _ENABLE_DX10_
		device = "dx10";
		init_resource = InitResourceDX10;
		release_resource = ReleaseResourceDX10;
		render = RenderFrameDX10;
		resize_func = ResizeWindowDX10;
	#endif
		break;
	}

	GutResizeFunc( resize_func );
	GutCreateWindow(100, 100, 512, 512, device);

	if ( !GutInitGraphicsDevice(device) )
	{
		printf("Failed to initialize %s device\n", device);
		exit(0);
	}

	// ambient
	g_Lights[0].m_vPosition.Set(0.0f, 0.0f, 0.0f, 1.0f);
	g_Lights[0].m_vDiffuse.Set(0.0f);
	g_Lights[0].m_vAmbient.Set(0.3f);

	GutInputInit();

	GutRegisterKeyDown(GUTKEY_1, KeyDown_1);
	GutRegisterKeyDown(GUTKEY_2, KeyDown_2);
	GutRegisterKeyDown(GUTKEY_3, KeyDown_3);
	GutRegisterKeyDown(GUTKEY_SPACE, KeyDown_Space);

	g_Control.SetCamera(
		Vector4(0.0f, 3.0f, 0.0f), 
		Vector4(0.0f, 0.0f, 0.0f),
		Vector4(0.0f, 0.0f, 1.0f) );

	g_Model.Load_ASCII("..\\..\\models\\earth_normalmapped.gma");
	g_Sphere.Load_ASCII("..\\..\\models\\sphere_simple.gma");

	// 载入shader
	if ( !init_resource() )
	{
		release_resource();
		printf("Failed to load resources\n");
		exit(0);
	}
	// 主循环
	while( GutProcessMessage() )
	{
		GetUserInput();
		frame_move();
		render();
	}
	// 释放shader
	release_resource();
	// 关闭OpenGL/DirectX绘图设备
	GutReleaseGraphicsDevice();
}
