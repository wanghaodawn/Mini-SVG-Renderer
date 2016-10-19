#include "texture.h"

#include <assert.h>
#include <iostream>
#include <algorithm>
#include <math.h>

#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
#define DEBUG_CODE(CODEFRAGMENT) CODEFRAGMENT;
#else
#define DEBUG_CODE(CODEFRAGMENT)
#endif

using namespace std;

namespace CMU462 {

inline void uint8_to_float( float dst[4], unsigned char* src ) {
  uint8_t* src_uint8 = (uint8_t *)src;
  dst[0] = src_uint8[0] / 255.f;
  dst[1] = src_uint8[1] / 255.f;
  dst[2] = src_uint8[2] / 255.f;
  dst[3] = src_uint8[3] / 255.f;
}

inline void float_to_uint8( unsigned char* dst, float src[4] ) {
  uint8_t* dst_uint8 = (uint8_t *)dst;
  dst_uint8[0] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[0])));
  dst_uint8[1] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[1])));
  dst_uint8[2] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[2])));
  dst_uint8[3] = (uint8_t) ( 255.f * max( 0.0f, min( 1.0f, src[3])));
}

void Sampler2DImp::generate_mips(Texture& tex, int startLevel) {

  // This function is run, when set_viewbox function runs

  // NOTE(sky): 
  // The starter code allocates the mip levels and generates a level 
  // map simply fills each level with a color that differs from its
  // neighbours'. The reference solution uses trilinear filtering
  // and it will only work when you have mipmaps.

  // Task 6: Implement this
  DEBUG_CODE(printf("\n\n\n\ngenerate_mips\n\n\n\n"));
  // check start level
  if ( startLevel >= tex.mipmap.size() ) {
    std::cerr << "Invalid start level"; 
  }

  // allocate sublevels
  int baseWidth  = tex.mipmap[startLevel].width;
  int baseHeight = tex.mipmap[startLevel].height;
  int numSubLevels = (int)(log2f( (float)max(baseWidth, baseHeight)));

  numSubLevels = min(numSubLevels, kMaxMipLevels - startLevel - 1); // 8

  tex.mipmap.resize(startLevel + numSubLevels + 1);

  int width  = baseWidth;
  int height = baseHeight;

  // Setup the width and height for each level
  for (int i = 1; i <= numSubLevels; i++) {

    MipLevel& level = tex.mipmap[startLevel + i];

    // handle odd size texture by rounding down
    width  = max( 1, width  / 2); assert(width  > 0);
    height = max( 1, height / 2); assert(height > 0);

    level.width = width;
    level.height = height;
    level.texels = vector<unsigned char>(4 * width * height);

  }

  // Generate mipmap
  int x, y, level, c, temp;

  for (level = 1; level <= numSubLevels; level++) {
    MipLevel& prev_mip = tex.mipmap[level - 1];
    MipLevel& cur_mip = tex.mipmap[level];
    int prev_width = prev_mip.width;
    int cur_width = cur_mip.width;

    for (y = 0; y < cur_mip.height; y++) {
      for (x = 0; x < cur_mip.width; x++) {
        int y1 = y * 2;
        int y2 = y * 2 + 1;
        int x1 = x * 2;
        int x2 = x * 2 + 1;

        for (c = 0; c < 4; c++) {
          temp  = (int) prev_mip.texels[4 * (y1 * prev_width + x1) + c];
          temp += (int) prev_mip.texels[4 * (y1 * prev_width + x2) + c];
          temp += (int) prev_mip.texels[4 * (y2 * prev_width + x1) + c];
          temp += (int) prev_mip.texels[4 * (y2 * prev_width + x2) + c];
          
          temp /= 4;

          cur_mip.texels[4 * (y * cur_width + x) + c] = temp;
        }
      }
    }
  }
}

inline Color getColor(unsigned char* colorStart) {

  float tempColor[4];
  uint8_to_float(tempColor, colorStart);
  return Color(tempColor[0], tempColor[1], tempColor[2], tempColor[3]);
}

Color Sampler2DImp::sample_nearest(Texture& tex, 
                                   float u, float v, 
                                   int level) {

  // Task 5: Implement nearest neighbour interpolation
  // DEBUG_CODE(printf("sample_nearest\n"));

  if (u < 0 || u > 1 || v < 0 || v > 1)
    return Color(1,0,1,1);

  int width = tex.mipmap[level].width;
  int height = tex.mipmap[level].height;

  //printf("u=%f, v=%f\n", u, v);

  int int_u = round(u * width);
  int int_v = round(v * height);

  // printf("int_u=%d, int_v=%d, level=%d\n", int_u, int_v, level);

  Color color = getColor(&tex.mipmap[level].texels[4 * (width * int_u + int_v)]);

  return color;
}

Color Sampler2DImp::sample_bilinear(Texture& tex, 
                                    float u, float v, 
                                    int level) {
  // Task 5: Implement bilinear filtering
  // DEBUG_CODE(printf("sample_bilinear\n"));

  if (u < 0 || u > 1 || v < 0 || v > 1)
    return Color(1,0,1,1);

  int width = tex.mipmap[level].width;
  int height = tex.mipmap[level].height;

  //printf("u=%f, v=%f\n", u, v);

  int u_down = floor(u * width);
  int v_down = floor(v * height);
  int u_up = u_down + 1;
  int v_up = v_down + 1;

  //printf("u_down=%f, v_down=%f, u_up=%f, v_up=%f\n", u_down, v_down, u_up, v_up);

  Color c00 = getColor(&tex.mipmap[level].texels[4 * (width * v_down + u_down)]);
  Color c10 = getColor(&tex.mipmap[level].texels[4 * (width * v_down + u_up)]);
  Color c01 = getColor(&tex.mipmap[level].texels[4 * (width * v_up + u_down)]);
  Color c11 = getColor(&tex.mipmap[level].texels[4 * (width * v_up + u_up)]);

  Color cp1 = c00 * (v_up - v) + c01 * (v - v_down);
  Color cp2 = c10 * (v_up - v) + c11 * (v - v_down);

  Color cp = cp1 * (u_up - u) + cp2 * (u - u_down);

  return Color(cp.r, cp.g, cp.b, cp.a);

}

Color Sampler2DImp::sample_trilinear(Texture& tex, 
                                     float u, float v, 
                                     float u_scale, float v_scale) {
  // Task 6: Implement trilinear filtering
  // DEBUG_CODE(printf("sample_bilinear\n"));

  Color cp;
  float l, d;
  float log2 = log(2);

  if (u_scale > v_scale)
    l = u_scale;
  else
    l = v_scale;

  if (l <= 1) {
    // color = sampler->sample_bilinear(tex, u, v, 0);
    cp = sample_nearest(tex, u, v, 0);
    // printf("c0=%u, c1=%u, c2=%u, c3=%u\n", color.r, color.g, color.b, color.a);
  } else {
    d = log(l) / log2;
    int d_up = ceil(d);
    int d_down = d_up - 1;
    
    Color c_down = sample_bilinear(tex, u, v, d_down);
    Color c_up   = sample_bilinear(tex, u, v, d_up);

    cp = c_down * (d_up - d) + c_up * (d - d_down);
  }
  // printf("l=%f\n",l);

  return cp;
}

} // namespace CMU462
