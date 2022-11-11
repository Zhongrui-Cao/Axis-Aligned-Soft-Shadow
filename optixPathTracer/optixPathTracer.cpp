/* 
 * Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *  * Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *  * Neither the name of NVIDIA CORPORATION nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

//-----------------------------------------------------------------------------
//
// optixPathTracer: simple interactive path tracer
//
//-----------------------------------------------------------------------------

#ifdef __APPLE__
#  include <GLUT/glut.h>
#else
#  include <GL/glew.h>
#  if defined( _WIN32 )
#    include <GL/wglew.h>
#    include <GL/freeglut.h>
#  else
#    include <GL/glut.h>
#  endif
#endif

#include <optixu/optixpp_namespace.h>
#include <optixu/optixu_math_stream_namespace.h>

#include "optixPathTracer.h"
#include <sutil.h>
#include <Arcball.h>

#include <algorithm>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <list>
#include <numeric>
#include <vector>

using namespace optix;

const char* const SAMPLE_NAME = "optixPathTracer";

//------------------------------------------------------------------------------
//
// Globals
//
//------------------------------------------------------------------------------

Context        context = 0;
uint32_t       width  = 1000;
uint32_t       height = 1000;
bool           use_pbo = true;

int            frame_number = 1;
int            sqrt_num_samples = 1;
int            rr_begin_depth = 1;
Program        pgram_intersection = 0;
Program        pgram_bounding_box = 0;

// Camera state
float3         camera_up;
float3         camera_lookat;
float3         camera_eye;
Matrix4x4      camera_rotate;
float          camera_pitch;
float          camera_yaw;
float3         camera_pos;
bool           camera_changed = true;
sutil::Arcball arcball;

// Mouse state
int2           mouse_prev_pos;
int            mouse_button;

// Move state
bool           W = false;
bool           A = false;
bool           S = false;
bool           D = false;
bool           UP = false;
bool           DOWN = false;
float camera_speed = 10.0f;



//------------------------------------------------------------------------------
//
// Forward decls 
//
//------------------------------------------------------------------------------

Buffer getOutputBuffer();
void destroyContext();
void registerExitHandler();
void createContext();
void loadGeometry();
void setupCamera();
void updateCamera();
void glutInitialize( int* argc, char** argv );
void glutRun();

void glutDisplay();
void glutKeyboardPress( unsigned char k, int x, int y );
void glutKeyboardUp(unsigned char k, int x, int y);
void glutMousePress( int button, int state, int x, int y );
void glutMouseMotion( int x, int y);
void glutResize( int w, int h );


//------------------------------------------------------------------------------
//
//  Helper functions
//
//------------------------------------------------------------------------------

Buffer getOutputBuffer()
{
    return context[ "output_buffer" ]->getBuffer();
}


void destroyContext()
{
    if( context )
    {
        context->destroy();
        context = 0;
    }
}


void registerExitHandler()
{
    // register shutdown handler
#ifdef _WIN32
    glutCloseFunc( destroyContext );  // this function is freeglut-only
#else
    atexit( destroyContext );
#endif
}


void setMaterial(
        GeometryInstance& gi,
        Material material,
        const std::string& color_name,
        const float3& color)
{
    gi->addMaterial(material);
    gi[color_name]->setFloat(color);
}


GeometryInstance createParallelogram(
        const float3& anchor,
        const float3& offset1,
        const float3& offset2)
{
    Geometry parallelogram = context->createGeometry();
    parallelogram->setPrimitiveCount( 1u );
    parallelogram->setIntersectionProgram( pgram_intersection );
    parallelogram->setBoundingBoxProgram( pgram_bounding_box );

    float3 normal = normalize( cross( offset1, offset2 ) );
    float d = dot( normal, anchor );
    float4 plane = make_float4( normal, d );

    float3 v1 = offset1 / dot( offset1, offset1 );
    float3 v2 = offset2 / dot( offset2, offset2 );

    parallelogram["plane"]->setFloat( plane );
    parallelogram["anchor"]->setFloat( anchor );
    parallelogram["v1"]->setFloat( v1 );
    parallelogram["v2"]->setFloat( v2 );

    GeometryInstance gi = context->createGeometryInstance();
    gi->setGeometry(parallelogram);
    return gi;
}


void createContext()
{
    context = Context::create();
    context->setRayTypeCount( 2 );
    context->setEntryPointCount( 4 );
    context->setStackSize( 1800 );
    context->setMaxTraceDepth( 2 );

    context[ "scene_epsilon"                  ]->setFloat( 1.e-3f );
    context[ "rr_begin_depth"                 ]->setUint( rr_begin_depth );

    Buffer buffer = sutil::createOutputBuffer( context, RT_FORMAT_FLOAT4, width, height, use_pbo );
    context["output_buffer"]->set( buffer );

    // my buffers
    Buffer resultBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, false);
    context["result_buffer"]->set(resultBuffer);

    Buffer hitBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, false);
    context["hit_buffer"]->set(hitBuffer);

    Buffer normalBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, false);
    context["normal_buffer"]->set(normalBuffer);

    Buffer ffnormalBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, false);
    context["ffnormal_buffer"]->set(ffnormalBuffer);
    
    Buffer objectidBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT, width, height, false);
    context["object_id_buffer"]->set(objectidBuffer);

    Buffer d1Buffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT, width, height, false);
    context["d1_buffer"]->set(d1Buffer);

    Buffer d2MinBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT, width, height, false);
    context["d2_min_buffer"]->set(d2MinBuffer);

    Buffer d2MaxBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT, width, height, false);
    context["d2_max_buffer"]->set(d2MaxBuffer);

    Buffer projectedDistBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT, width, height, false);
    context["projected_dist_buffer"]->set(projectedDistBuffer);

    Buffer sppBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT, width, height, false);
    context["spp_buffer"]->set(sppBuffer);

    Buffer heatmapBuffer = sutil::createOutputBuffer(context, RT_FORMAT_FLOAT3, width, height, false);
    context["heatmap_buffer"]->set(heatmapBuffer);
    // for compiler
    context["input_buffer"]->set(d1Buffer);

    Buffer isLightBuffer = sutil::createOutputBuffer(context, RT_FORMAT_BYTE, width, height, false);
    context["islight_buffer"]->set(isLightBuffer);


    // Setup programs
    const char *ptx = sutil::getPtxString( SAMPLE_NAME, "optixPathTracer.cu" );
    context->setRayGenerationProgram( 0, context->createProgramFromPTXString( ptx, "pathtrace_camera" ) );
    context->setExceptionProgram( 0, context->createProgramFromPTXString( ptx, "exception" ) );
    context->setMissProgram( 0, context->createProgramFromPTXString( ptx, "miss" ) );

    // my programs
    const char* aa_ptx = sutil::getPtxString(SAMPLE_NAME, "AA.cu");
    const char* hm_ptx = sutil::getPtxString(SAMPLE_NAME, "heatmap.cu");

    // get distance: 1
    context->setRayGenerationProgram(1, context->createProgramFromPTXString(aa_ptx, "get_distance"));
    context->setExceptionProgram(1, context->createProgramFromPTXString(aa_ptx, "exception"));
    context->setMissProgram(1, context->createProgramFromPTXString(aa_ptx, "miss"));

    // trace primary ray: 2
    context->setRayGenerationProgram(2, context->createProgramFromPTXString(aa_ptx, "aa_trace_firstpass"));

    // heat map program: 3
    context->setRayGenerationProgram(3, context->createProgramFromPTXString(hm_ptx, "get_heatmap"));

    context[ "sqrt_num_samples" ]->setUint( sqrt_num_samples );
    context[ "bad_color"        ]->setFloat( 1000000.0f, 0.0f, 1000000.0f ); // Super magenta to make sure it doesn't get averaged out in the progressive rendering.
    context[ "bg_color"         ]->setFloat( make_float3(0.0f) );

    // my vars
    context["img_width"]->setUint(width);
    context["img_height"]->setUint(width);
}


void loadGeometry()
{
    // Light buffer
    ParallelogramLight light;
    light.corner   = make_float3( 343.0f, 548.6f, 227.0f);
    light.v1       = make_float3( -130.0f, 0.0f, 0.0f);
    light.v2       = make_float3( 0.0f, 0.0f, 105.0f);
    light.normal   = normalize( cross(light.v1, light.v2) );
    light.emission = make_float3( 20.0f, 20.0f, 20.0f );

    float3 norm = cross(light.v1, light.v2);
    float sigma = sqrt(length(norm) / 4.0f);
    context["light_sigma"]->setFloat(sigma);

    Buffer light_buffer = context->createBuffer( RT_BUFFER_INPUT );
    light_buffer->setFormat( RT_FORMAT_USER );
    light_buffer->setElementSize( sizeof( ParallelogramLight ) );
    light_buffer->setSize( 1u );
    memcpy( light_buffer->map(), &light, sizeof( light ) );
    light_buffer->unmap();
    context["lights"]->setBuffer( light_buffer );


    // Set up material
    Material diffuse = context->createMaterial();
    /*
    const char *ptx = sutil::getPtxString( SAMPLE_NAME, "optixPathTracer.cu" );
    Program diffuse_ch = context->createProgramFromPTXString( ptx, "diffuse" );
    Program diffuse_ah = context->createProgramFromPTXString( ptx, "shadow" );
    diffuse->setClosestHitProgram( 0, diffuse_ch );
    diffuse->setAnyHitProgram( 1, diffuse_ah );
    */
    const char* ptx = sutil::getPtxString(SAMPLE_NAME, "optixPathTracer.cu");
    const char* ptx_aa = sutil::getPtxString(SAMPLE_NAME, "AA.cu");

    Program diffuse_ch = context->createProgramFromPTXString(ptx_aa, "geometry_hit");
    Program diffuse_ah = context->createProgramFromPTXString(ptx_aa, "shadow");
    diffuse->setClosestHitProgram(0, diffuse_ch);
    diffuse->setAnyHitProgram(1, diffuse_ah);

    Material diffuse_light = context->createMaterial();
    Program diffuse_em = context->createProgramFromPTXString(ptx_aa, "diffuseEmitter" );
    diffuse_light->setClosestHitProgram( 0, diffuse_em );

    // Set up parallelogram programs
    ptx = sutil::getPtxString( SAMPLE_NAME, "parallelogram.cu" );
    pgram_bounding_box = context->createProgramFromPTXString( ptx, "bounds" );
    pgram_intersection = context->createProgramFromPTXString( ptx, "intersect" );

    // create geometry instances
    {

    // create geometry instances
    std::vector<GeometryInstance> gis;

    const float3 white = make_float3( 0.8f, 0.8f, 0.8f );
    const float3 green = make_float3( 0.05f, 0.8f, 0.05f );
    const float3 red   = make_float3( 0.8f, 0.05f, 0.05f );
    const float3 light_em = make_float3( 15.0f, 15.0f, 5.0f );

    // Floor
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ),
                                        make_float3( 556.0f, 0.0f, 0.0f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Ceiling
    gis.push_back( createParallelogram( make_float3( 0.0f, 548.8f, 0.0f ),
                                        make_float3( 556.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Back wall
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 559.2f),
                                        make_float3( 0.0f, 548.8f, 0.0f),
                                        make_float3( 556.0f, 0.0f, 0.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Right wall
    /*
    gis.push_back( createParallelogram( make_float3( 0.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 548.8f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", green);
    */


    // Left wall
    gis.push_back( createParallelogram( make_float3( 556.0f, 0.0f, 0.0f ),
                                        make_float3( 0.0f, 0.0f, 559.2f ),
                                        make_float3( 0.0f, 548.8f, 0.0f ) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", red);

    // Short block
    gis.push_back( createParallelogram( make_float3( 130.0f, 165.0f, 65.0f),
                                        make_float3( -48.0f, 0.0f, 160.0f),
                                        make_float3( 160.0f, 0.0f, 49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 290.0f, 0.0f, 114.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( -50.0f, 0.0f, 158.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 130.0f, 0.0f, 65.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( 160.0f, 0.0f, 49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 82.0f, 0.0f, 225.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( 48.0f, 0.0f, -160.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 240.0f, 0.0f, 272.0f),
                                        make_float3( 0.0f, 165.0f, 0.0f),
                                        make_float3( -158.0f, 0.0f, -47.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Tall block
    gis.push_back( createParallelogram( make_float3( 423.0f, 330.0f, 247.0f),
                                        make_float3( -158.0f, 0.0f, 49.0f),
                                        make_float3( 49.0f, 0.0f, 159.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 423.0f, 0.0f, 247.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( 49.0f, 0.0f, 159.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 472.0f, 0.0f, 406.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( -158.0f, 0.0f, 50.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 314.0f, 0.0f, 456.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( -49.0f, 0.0f, -160.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);
    gis.push_back( createParallelogram( make_float3( 265.0f, 0.0f, 296.0f),
                                        make_float3( 0.0f, 330.0f, 0.0f),
                                        make_float3( 158.0f, 0.0f, -49.0f) ) );
    setMaterial(gis.back(), diffuse, "diffuse_color", white);

    // Create shadow group (no light)
    GeometryGroup shadow_group = context->createGeometryGroup(gis.begin(), gis.end());
    shadow_group->setAcceleration( context->createAcceleration( "Trbvh" ) );
    context["top_shadower"]->set( shadow_group );

    // Light
    gis.push_back( createParallelogram( make_float3( 343.0f, 548.6f, 227.0f),
                                        make_float3( -130.0f, 0.0f, 0.0f),
                                        make_float3( 0.0f, 0.0f, 105.0f) ) );
    setMaterial(gis.back(), diffuse_light, "emission_color", light_em);

    // Create geometry group
    GeometryGroup geometry_group = context->createGeometryGroup(gis.begin(), gis.end());
    geometry_group->setAcceleration( context->createAcceleration( "Trbvh" ) );
    context["top_object"]->set( geometry_group );

    }
}

  
void setupCamera()
{
    camera_eye    = make_float3( 278.0f, 273.0f, -900.0f );
    camera_lookat = make_float3( 278.0f, 273.0f,    0.0f );
    camera_up     = make_float3(   0.0f,   1.0f,    0.0f );
    camera_pos    = make_float3(278.0f, 273.0f, -900.0f);
    camera_pitch  = 0.0f;
    camera_yaw    = 1.55f;

    camera_rotate  = Matrix4x4::identity();
}


void updateCamera()
{
    const float fov  = 35.0f;
    const float aspect_ratio = static_cast<float>(width) / static_cast<float>(height);

    float3 front = make_float3(cos(camera_pitch) * cos(camera_yaw), sin(camera_pitch), cos(camera_pitch) * sin(camera_yaw));
    front = normalize(front);
    float3 right = cross(front, make_float3(0.0f, 1.0f, 0.0f));

    //cam movement
    if (D) {
        camera_pos += right * camera_speed;
    }
    if (A) {
        camera_pos -= right * camera_speed;
    }
    if (W) {
        camera_pos += front * camera_speed;
    }
    if (S) {
        camera_pos -= front * camera_speed;
    }
    if (UP) {
        camera_pos += camera_up * camera_speed;
    }
    if (DOWN) {
        camera_pos -= camera_up * camera_speed;
    }

    float3 camera_lookat = camera_pos + front;

    float3 camera_u, camera_v, camera_w;
    sutil::calculateCameraVariables(
        camera_pos, camera_lookat, camera_up, fov, aspect_ratio,
            camera_u, camera_v, camera_w, /*fov_is_vertical*/ true );

    if( camera_changed ) // reset accumulation
        //frame_number = 1; //real time no need
    camera_changed = false;

    context[ "frame_number" ]->setUint( frame_number++ );
    context[ "eye"]->setFloat( camera_pos );
    context[ "U"  ]->setFloat( camera_u );
    context[ "V"  ]->setFloat( camera_v );
    context[ "W"  ]->setFloat( camera_w );

}


void glutInitialize( int* argc, char** argv )
{
    glutInit( argc, argv );
    glutInitDisplayMode( GLUT_RGB | GLUT_ALPHA | GLUT_DEPTH | GLUT_DOUBLE );
    glutInitWindowSize( width, height );
    glutInitWindowPosition( 100, 100 );                                               
    glutCreateWindow( SAMPLE_NAME );
    glutHideWindow();                                                              
}


void glutRun()
{
    // Initialize GL state                                                            
    glMatrixMode(GL_PROJECTION);                                                   
    glLoadIdentity();                                                              
    glOrtho(0, 1, 0, 1, -1, 1 );                                                   

    glMatrixMode(GL_MODELVIEW);                                                    
    glLoadIdentity();                                                              

    glViewport(0, 0, width, height);                                 

    glutShowWindow();                                                              
    glutReshapeWindow( width, height);

    // register glut callbacks
    glutDisplayFunc( glutDisplay );
    glutIdleFunc( glutDisplay );
    glutReshapeFunc( glutResize );
    glutKeyboardFunc( glutKeyboardPress );
    glutKeyboardUpFunc(glutKeyboardUp);
    glutMouseFunc( glutMousePress );
    glutMotionFunc( glutMouseMotion );

    registerExitHandler();

    glutMainLoop();
}


void diaplayHeatmap(Buffer buffer, float estimated_max)
{
    // Normalize and display the beta buffer
    float minValue, maxValue, avgValue;
    context["max_value"]->setFloat(estimated_max);
    context["input_buffer"]->set(buffer);
    context->launch(3, width, height);

    sutil::displayBufferGL(context["heatmap_buffer"]->getBuffer());
}

//------------------------------------------------------------------------------
//
//  GLUT callbacks
//
//------------------------------------------------------------------------------

void glutDisplay()
{
    updateCamera();

    // TODO
    // reference ground truth, not working for some reason??????
    //context->launch(0, width, height);

    // trace primary ray
    context->launch(2, width, height);
    // get distance and trace with adap spp
    context->launch(1, width, height);

    //sutil::displayBufferGL( getOutputBuffer() );
    //diaplayHeatmap(context["d1_buffer"]->getBuffer(), 675.0f);
    //diaplayHeatmap(context["d2_min_buffer"]->getBuffer(), 500.0f);
    //diaplayHeatmap(context["d2_max_buffer"]->getBuffer(), 640.0f);
    //diaplayHeatmap(context["projected_dist_buffer"]->getBuffer(), 30.0f);
    //diaplayHeatmap(context["spp_buffer"]->getBuffer(), 100.0f);
    sutil::displayBufferGL(context["result_buffer"]->getBuffer());

    {
      static unsigned frame_count = 0;
      sutil::displayFps( frame_count++ );
    }

    glutSwapBuffers();
}


void glutKeyboardPress( unsigned char k, int x, int y )
{
    switch( k )
    {
        case( 27 ): // ESC
        {
            destroyContext();
            exit(0);
        }
        case( 'p' ):
        {
            const std::string outputImage = std::string(SAMPLE_NAME) + ".ppm";
            std::cerr << "Saving current frame to '" << outputImage << "'\n";
            sutil::displayBufferPPM( outputImage.c_str(), getOutputBuffer(), false );
            break;
        }
        case('r'):
        {
            setupCamera();
            break;
        }
        case('w'):
        {
            W = true;
            break;
        }
        case('a'):
        {
            A = true;
            break;
        }
        case('s'):
        {
            S = true;
            break;
        }
        case('d'):
        {
            D = true;
            break;
        }
        case('q'):
        {
            UP = true;
            break;
        }
        case('e'):
        {
            DOWN = true;
            break;
        }
    }
}


void glutKeyboardUp( unsigned char k, int x, int y )
{
    switch (k)
    {
        case('w'):
        {
            W = false;
            break;
        }
        case('a'):
        {
            A = false;
            break;
        }
        case('s'):
        {
            S = false;
            break;
        }
        case('d'):
        {
            D = false;
            break;
        }
        case('q'):
        {
            UP = false;
            break;
        }
        case('e'):
        {
            DOWN = false;
            break;
        }
    }

}


void glutMousePress( int button, int state, int x, int y )
{
    if( state == GLUT_DOWN )
    {
        mouse_button = button;
        mouse_prev_pos = make_int2( x, y );
    }
    else
    {
        // nothing
    }
}


void glutMouseMotion( int x, int y)
{
    if( mouse_button == GLUT_RIGHT_BUTTON )
    {
        const float dx = static_cast<float>( x - mouse_prev_pos.x ) /
                         static_cast<float>( width );
        const float dy = static_cast<float>( y - mouse_prev_pos.y ) /
                         static_cast<float>( height );
        const float dmax = fabsf( dx ) > fabs( dy ) ? dx : dy;
        const float scale = std::min<float>( dmax, 0.9f );
        camera_pos = camera_pos + (camera_lookat - camera_pos)*scale;
        camera_changed = true;
    }
    else if( mouse_button == GLUT_LEFT_BUTTON )
    {
        const float dx = static_cast<float>(x - mouse_prev_pos.x) /
            static_cast<float>(width);
        const float dy = static_cast<float>(y - mouse_prev_pos.y) /
            static_cast<float>(height);
        camera_yaw += dx;
        camera_pitch -= dy;
    }

    mouse_prev_pos = make_int2( x, y );
}


void glutResize( int w, int h )
{
    if ( w == (int)width && h == (int)height ) return;

    camera_changed = true;

    width  = w;
    height = h;
    sutil::ensureMinimumSize(width, height);

    sutil::resizeBuffer( getOutputBuffer(), width, height );

    glViewport(0, 0, width, height);                                               

    glutPostRedisplay();
}


//------------------------------------------------------------------------------
//
// Main
//
//------------------------------------------------------------------------------

void printUsageAndExit( const std::string& argv0 )
{
    std::cerr << "\nUsage: " << argv0 << " [options]\n";
    std::cerr <<
        "App Options:\n"
        "  -h | --help               Print this usage message and exit.\n"
        "  -f | --file               Save single frame to file and exit.\n"
        "  -n | --nopbo              Disable GL interop for display buffer.\n"
        "  -d | --dim=<width>x<height> Set image dimensions. Defaults to 512x512\n"
        "App Keystrokes:\n"
        "  q  Quit\n" 
        "  s  Save image to '" << SAMPLE_NAME << ".ppm'\n"
        << std::endl;

    exit(1);
}


int main( int argc, char** argv )
 {
    std::string out_file;
    for( int i=1; i<argc; ++i )
    {
        const std::string arg( argv[i] );

        if( arg == "-h" || arg == "--help" )
        {
            printUsageAndExit( argv[0] );
        }
        else if( arg == "-f" || arg == "--file"  )
        {
            if( i == argc-1 )
            {
                std::cerr << "Option '" << arg << "' requires additional argument.\n";
                printUsageAndExit( argv[0] );
            }
            out_file = argv[++i];
        }
        else if( arg == "-n" || arg == "--nopbo"  )
        {
            use_pbo = false;
        }
        else if( arg.find( "-d" ) == 0 || arg.find( "--dim" ) == 0 )
        {
            size_t index = arg.find_first_of( '=' );
            if( index == std::string::npos )
            {
                std::cerr << "Option '" << arg << " is malformed. Please use the syntax -d | --dim=<width>x<height>.\n";
                printUsageAndExit( argv[0] );
            }
            std::string dim = arg.substr( index+1 );
            try
            {
                sutil::parseDimensions( dim.c_str(), (int&)width, (int&)height );
            }
            catch( const Exception& )
            {
                std::cerr << "Option '" << arg << " is malformed. Please use the syntax -d | --dim=<width>x<height>.\n";
                printUsageAndExit( argv[0] );
            }
        }
        else
        {
            std::cerr << "Unknown option '" << arg << "'\n";
            printUsageAndExit( argv[0] );
        }
    }

    try
    {
        glutInitialize( &argc, argv );

#ifndef __APPLE__
        glewInit();
#endif

        createContext();
        setupCamera();
        loadGeometry();

        context->validate();

        if ( out_file.empty() )
        {
            glutRun();
        }
        else
        {
            updateCamera();
            context->launch( 0, width, height );
            sutil::displayBufferPPM( out_file.c_str(), getOutputBuffer(), false );
            destroyContext();
        }

        return 0;
    }
    SUTIL_CATCH( context->get() )
}

