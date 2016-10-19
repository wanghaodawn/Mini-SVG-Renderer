#include "software_renderer.h"

#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <assert.h>
#include <math.h>

#include "triangulation.h"
#include "texture.h"

using namespace std;

#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
#define DEBUG_CODE(CODEFRAGMENT) CODEFRAGMENT;
#else
#define DEBUG_CODE(CODEFRAGMENT)
#endif


namespace CMU462 {
// Implements SoftwareRenderer //
  unsigned char* small_target;
  int doneSampleRate = 0;

void SoftwareRendererImp::draw_svg( SVG& svg ) {
  
  DEBUG_CODE(printf("draw_svg\n"));

  clear_target();
  // set top level transformation
  transformation = canvas_to_screen;

  // draw all elements
  for ( size_t i = 0; i < svg.elements.size(); ++i ) {
    draw_element(svg.elements[i]);
  }

  // draw canvas outline
  Vector2D a = transform(Vector2D(    0    ,     0    )); a.x--; a.y++;
  Vector2D b = transform(Vector2D(svg.width,     0    )); b.x++; b.y++;
  Vector2D c = transform(Vector2D(    0    ,svg.height)); c.x--; c.y--;
  Vector2D d = transform(Vector2D(svg.width,svg.height)); d.x++; d.y--;

  rasterize_line(a.x, a.y, b.x, b.y, Color::Black);
  rasterize_line(a.x, a.y, c.x, c.y, Color::Black);
  rasterize_line(d.x, d.y, b.x, b.y, Color::Black);
  rasterize_line(d.x, d.y, c.x, c.y, Color::Black);

  // resolve and send to render target
  // After finishing painting all elements
  resolve();
}

void SoftwareRendererImp::set_sample_rate( size_t sample_rate ) {
  DEBUG_CODE(printf("set_sample_rate\n"));

  // Task 3: 
  // You may want to modify this for supersampling support
  this->sample_rate = sample_rate;

  printf("target_w=%lu target_h=%lu, \n", target_w, target_h);
  if (sample_rate > 1) {
    doneSampleRate = 1;
    size_t big_w = target_w * sample_rate;
    size_t big_h = target_h * sample_rate;
    small_target = render_target;

    printf("small_w=%lu small_h=%lu, \n", target_w, target_h);
  
    // Malloc big buffer
    unsigned char* supersample_target = (unsigned char*)malloc(sizeof(char) *\
                                        4 * big_w * big_h);
    this->render_target = supersample_target;
    this->target_w = big_w;
    this->target_h = big_h;
  } else {
    doneSampleRate = 0;
  }
}

void SoftwareRendererImp::set_render_target( unsigned char* render_target,
                                             size_t width, size_t height ) {
  // Task 5: 
  // You may want to modify this for supersampling support
  DEBUG_CODE(printf("set_render_target\n"));

  this->render_target = render_target;
  this->target_w = width;
  this->target_h = height;
}

void SoftwareRendererImp::draw_element( SVGElement* element ) {
  // DEBUG_CODE(printf("draw_element\n"));

  // Task 4 (part 1):
  // Modify this to implement the transformation stack

  transformation= transformation * element->transform;

  switch(element->type) {
    case POINT:
      draw_point(static_cast<Point&>(*element));
      break;
    case LINE:
      draw_line(static_cast<Line&>(*element));
      break;
    case POLYLINE:
      draw_polyline(static_cast<Polyline&>(*element));
      break;
    case RECT:
      draw_rect(static_cast<Rect&>(*element));
      break;
    case POLYGON:
      draw_polygon(static_cast<Polygon&>(*element));
      break;
    case ELLIPSE:
      draw_ellipse(static_cast<Ellipse&>(*element));
      break;
    case IMAGE:
      draw_image(static_cast<Image&>(*element));
      break;
    case GROUP:
      draw_group(static_cast<Group&>(*element));
      break;
    default:
      break;
  }

  transformation= transformation * element->transform.inv();
}


// Primitive Drawing //

void SoftwareRendererImp::draw_point( Point& point ) {
  // DEBUG_CODE(printf("draw_point\n"));

  Vector2D p = transform(point.position);
  rasterize_point( p.x, p.y, point.style.fillColor );

}

void SoftwareRendererImp::draw_line( Line& line ) { 
  DEBUG_CODE(printf("draw_line\n"));

  Vector2D p0 = transform(line.from);
  Vector2D p1 = transform(line.to);
  rasterize_line( p0.x, p0.y, p1.x, p1.y, line.style.strokeColor );

}

void SoftwareRendererImp::draw_polyline( Polyline& polyline ) {
  DEBUG_CODE(printf("draw_polyline\n"));

  Color c = polyline.style.strokeColor;

  if( c.a != 0 ) {
    int nPoints = polyline.points.size();
    for( int i = 0; i < nPoints - 1; i++ ) {
      Vector2D p0 = transform(polyline.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polyline.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_rect( Rect& rect ) {
  DEBUG_CODE(printf("draw_rect\n"));

  Color c;
  
  // draw as two triangles
  float x = rect.position.x;
  float y = rect.position.y;
  float w = rect.dimension.x;
  float h = rect.dimension.y;

  Vector2D p0 = transform(Vector2D(   x   ,   y   ));
  Vector2D p1 = transform(Vector2D( x + w ,   y   ));
  Vector2D p2 = transform(Vector2D(   x   , y + h ));
  Vector2D p3 = transform(Vector2D( x + w , y + h ));
  
  // draw fill
  c = rect.style.fillColor;
  if (c.a != 0 ) {
    rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    rasterize_triangle( p2.x, p2.y, p1.x, p1.y, p3.x, p3.y, c );
  }

  // draw outline
  c = rect.style.strokeColor;
  if( c.a != 0 ) {
    rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    rasterize_line( p1.x, p1.y, p3.x, p3.y, c );
    rasterize_line( p3.x, p3.y, p2.x, p2.y, c );
    rasterize_line( p2.x, p2.y, p0.x, p0.y, c );
  }

}

void SoftwareRendererImp::draw_polygon( Polygon& polygon ) {
  DEBUG_CODE(printf("draw_polygon\n"));

  Color c;

  // draw fill
  c = polygon.style.fillColor;
  if( c.a != 0 ) {

    // triangulate
    vector<Vector2D> triangles;
    triangulate( polygon, triangles );

    // draw as triangles
    for (size_t i = 0; i < triangles.size(); i += 3) {
      Vector2D p0 = transform(triangles[i + 0]);
      Vector2D p1 = transform(triangles[i + 1]);
      Vector2D p2 = transform(triangles[i + 2]);
      rasterize_triangle( p0.x, p0.y, p1.x, p1.y, p2.x, p2.y, c );
    }
  }

  // draw outline
  c = polygon.style.strokeColor;
  if( c.a != 0 ) {
    int nPoints = polygon.points.size();
    for( int i = 0; i < nPoints; i++ ) {
      Vector2D p0 = transform(polygon.points[(i+0) % nPoints]);
      Vector2D p1 = transform(polygon.points[(i+1) % nPoints]);
      rasterize_line( p0.x, p0.y, p1.x, p1.y, c );
    }
  }
}

void SoftwareRendererImp::draw_ellipse( Ellipse& ellipse ) {
  DEBUG_CODE(printf("draw_ellipse\n"));
  // Extra credit 

}

void SoftwareRendererImp::draw_image( Image& image ) {
  DEBUG_CODE(printf("draw_image\n"));
  Vector2D p0 = transform(image.position);
  Vector2D p1 = transform(image.position + image.dimension);

  rasterize_image( p0.x, p0.y, p1.x, p1.y, image.tex );
}

void SoftwareRendererImp::draw_group( Group& group ) {
  DEBUG_CODE(printf("draw_group\n"));
  for ( size_t i = 0; i < group.elements.size(); ++i ) {
    draw_element(group.elements[i]);
  }

}

// Rasterization //

// The input arguments in the rasterization functions 
// below are all defined in screen space coordinates

void SoftwareRendererImp::rasterize_point( float x, float y, Color color ) {
  //DEBUG_CODE(printf("rasterize_point\n"));

  // fill in the nearest pixel
  int sx = (int) round(x);
  int sy = (int) round(y);
  
  if (doneSampleRate == 1) {
    sx *= sample_rate;
    sy *= sample_rate;
  }

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  // fill sample - NOT doing alpha blending!
  // Super sampling, draw sample_rate^2 points, instead of just 1
  if (doneSampleRate == 1) {
    int i, j;
    for (i = sy; i < sy + sample_rate; i++) {
      for (j = sx; j < sx + sample_rate; j++) {
        render_target[4 * (j + i * target_w)    ] = (uint8_t) (color.r * 255);
        render_target[4 * (j + i * target_w) + 1] = (uint8_t) (color.g * 255);
        render_target[4 * (j + i * target_w) + 2] = (uint8_t) (color.b * 255);
        render_target[4 * (j + i * target_w) + 3] = (uint8_t) (color.a * 255);
      }
    }
  } else {
    render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
    render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
    render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
    render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);
  }
}

void SoftwareRendererImp::rasterize_point_1( float x, float y, Color color ) {
  //DEBUG_CODE(printf("rasterize_point_1\n"));
  
  // fill in the nearest pixel
  int sx = (int) round(x);
  int sy = (int) round(y);

  // check bounds
  if ( sx < 0 || sx >= target_w ) return;
  if ( sy < 0 || sy >= target_h ) return;

  // fill sample - NOT doing alpha blending!
  // Super sampling, draw sample_rate^2 points, instead of ju
  render_target[4 * (sx + sy * target_w)    ] = (uint8_t) (color.r * 255);
  render_target[4 * (sx + sy * target_w) + 1] = (uint8_t) (color.g * 255);
  render_target[4 * (sx + sy * target_w) + 2] = (uint8_t) (color.b * 255);
  render_target[4 * (sx + sy * target_w) + 3] = (uint8_t) (color.a * 255);
}

void SoftwareRendererImp::rasterize_line( float x0, float y0,
                                          float x1, float y1,
                                          Color color) {
  // Task 1
  // Implement line rasterization
  DEBUG_CODE(printf("rasterize_line\n"));

  // The implementation algorithm is Bresenham Line-drawing Algorithm
  // It is referenced from
  // http://www.cs.helsinki.fi/group/goa/mallinnus/lines/bresenh.html

  // Fill in the nearest pixel
  float sx0 = floor(x0);
  float sy0 = floor(y0);
  float sx1 = floor(x1);
  float sy1 = floor(y1);

  // Check bounds
  if ( sx0 < 0.0 || sx0 >= target_w ) return;
  if ( sy0 < 0.0 || sy0 >= target_h ) return;
  if ( sx1 < 0.0 || sx1 >= target_w ) return;
  if ( sy1 < 0.0 || sy1 >= target_h ) return;
  
  float stepX, stepY;
  float x, y;
  float epsilon;

  if (sx1 >= sx0)
    stepX = 1.0;
  else
    stepX = -1.0;
  
  if (sy1 >= sy0)
    stepY = 1.0;
  else
    stepY = -1.0;

  float slope = (sy1 - sy0) / (sx1 - sx0);

  x = sx0;
  y = sy0;

  // Plot (x0, y0)
  rasterize_point(sx0, sy0, color);

  // Draw the line
  if (abs(slope) > 1.0) {
    slope = 1 / slope;
    epsilon = x0 - sx0;

    for(; y * stepY < sy1 * stepY; y += stepY) {
      if ((epsilon + slope) * stepX >= 0.5) {
        x += stepX;
        epsilon += stepY * slope - stepX;
        rasterize_point(x, y + stepY, color);
      } else {
        rasterize_point(x, y + stepY, color);
        epsilon += stepY * slope;
      }
    }
  } else {
    epsilon = y0 - sy0;

    for (; x * stepX < sx1 * stepX; x += stepX){
      if ((epsilon + slope) * stepY >= 0.5) {
        y += stepY;
        epsilon += stepX * slope - stepY;
        rasterize_point(x + stepX, y, color);
      } else {
        rasterize_point(x + stepX, y, color);
        epsilon += stepX * slope;
      }
    }
  }
}

float getMin(float a, float b, float c) {
  // DEBUG_CODE(printf("getMin\n"));
  float min; 
  if (a > b)
    min = b;
  else
    min = a;

  if (c > min)
    return min;
  else
    return c;
}

float getMax(float a, float b, float c) {
  // DEBUG_CODE(printf("getMax\n"));
  float max;
  if (a > b)
    max = a;
  else
    max = b;

  if (c > max)
    return c;
  else
    return max;
}

void SoftwareRendererImp::rasterize_triangle( float x0, float y0,
                                              float x1, float y1,
                                              float x2, float y2,
                                              Color color ) {
  // Task 2: 
  // Implement triangle rasterization
  DEBUG_CODE(printf("rasterize_triangle\n"));

  // Draw the edges
  // Assume A(x0, y0)  B(x1, y1)  C(x2, y2)
  /*
  rasterize_line(x0, y0, x1, y1, color);  // AB
  rasterize_line(x1, y1, x2, y2, color);  // BC
  rasterize_line(x0, y0, x2, y2, color);  // AC
  */

  // Get the small rectangular's four vertexs

  x0 = round(x0);
  x1 = round(x1);
  x2 = round(x2);
  y0 = round(y0);
  y1 = round(y1);
  y2 = round(y2);

  float minX = getMin(x0, x1, x2);
  float maxX = getMax(x0, x1, x2);
  float minY = getMin(y0, y1, y2);
  float maxY = getMax(y0, y1, y2);

  if (doneSampleRate == 1) {
    minX *= sample_rate;
    maxX *= sample_rate;
    minY *= sample_rate;
    maxY *= sample_rate;
    x0 *= sample_rate;
    x1 *= sample_rate;
    x2 *= sample_rate;
    y0 *= sample_rate;
    y1 *= sample_rate;
    y2 *= sample_rate;
  }

  // Assume P(x, y)
  float x, y;
  float P_AB, C_AB, P_BC, A_BC, P_AC, B_AC;

  for (x = minX; x < maxX; x++) {
    for (y = minY; y < maxY; y++) {
      P_AB = (x - x1) * (y0 - y1) - (y - y1) * (x0 - x1);   //P - AB
      C_AB = (x2 - x1) * (y0 - y1) - (y2 - y1) * (x0 - x1); //C - AB

      P_BC = (x - x2) * (y1 - y2) - (y - y2) * (x1 - x2);   //P - BC
      A_BC = (x0 - x2) * (y1 - y2) - (y0 - y2) * (x1 - x2); //A - BC

      P_AC = (x - x2) * (y0 - y2) - (y - y2) * (x0 - x2);   //P - AC
      B_AC = (x1 - x2) * (y0 - y2) - (y1 - y2) * (x0 - x2); //B - AC

      // Whether the point is in the triangle or not
      if ((P_AB * C_AB >= 0.0) && (P_BC * A_BC >= 0.0) && (P_AC * B_AC >= 0.0)) {
        if (doneSampleRate == 0)
          rasterize_point(x, y, color);
        else
          rasterize_point_1(x, y, color);
      }
    }
  }
}

void SoftwareRendererImp::rasterize_image( float x0, float y0,
                                           float x1, float y1,
                                           Texture& tex ) {
  // Task ?: 
  // Implement image rasterization
  DEBUG_CODE(printf("rasterize_image\n"));

  x0 = floor(x0);
  y0 = floor(y0);
  x1 = floor(x1);
  y1 = floor(y1);

  // printf("x0=%f, y0=%f, x1=%f, y1=%f\n", x0, y0, x1, y1);
  // (426, 106) -> (1493, 1173)

  int level;
  int tex_width = tex.width;  // test7 -> 256
  int tex_height = tex.height;// test7 -> 256
  float canvas_width = x1 - x0;
  float canvas_height = y1 - y0;
  int scale_u, scale_v;
  //printf("tex_width=%d, tex_height=%d, screen_width=%d, screen_height=%d\n", tex_width, tex_height, screen_width, screen_height);
  
  float x, y, u, v, u_scale, v_scale, l, d;
  int d_near;
  float log2 = log(2);
  Color color;

  for (y = y0 + 0.5; y < y1; y ++) {
    v = (y - y0) / canvas_height;
    v_scale = tex_height / canvas_height;
    //printf("texture_v=%f\n", texture_v);
    for (x = x0 + 0.5; x < x1; x ++) {
      u = (x - x0) / canvas_width;
      u_scale = tex_width / canvas_width;

      if (u_scale > v_scale)
        l = u_scale;
      else
        l = v_scale;
      /*
      if (l <= 1) {
        color = sampler->sample_bilinear(tex, u, v, 0);
        // color = sampler->sample_nearest(tex, u, v, 0);
        // printf("c0=%u, c1=%u, c2=%u, c3=%u\n", color.r, color.g, color.b, color.a);
      } else {
        d = log(l) / log2;
        d_near = (int)round(d);
        color = sampler->sample_bilinear(tex, u, v, d_near);
        // color = sampler->sample_nearest(tex, u, v, d_near);
        // printf("c0=%u, c1=%u, c2=%u, c3=%u\n", color.r, color.g, color.b, color.a);
      }
      */

      
      color = sampler->sample_trilinear(tex, u, v, u_scale, v_scale);
        
      if (doneSampleRate == 0)
        rasterize_point(x, y, color);
      else
        rasterize_point_1(x, y, color);
      }
    }
    // printf("l=%f\n",l);
  printf("x0=%f, y0=%f, x1=%f, y1=%f\n", x0, y0, x1, y1);
  //printf("level=%lu\n", tex.mipmap.size());    tex.mipmap.size()

  /*
  int level;
  float x, y, u, v;
  // Different layers
  for (level = 0; level < tex.mipmap.size(); level++) {
    for (y = y0; y < y1; y += tex.height) {
      for (x = x0; x < x1 ; x += tex.width) {
        for (v = 0; v < tex.height; v++) {
          for (u = 0; u < tex.width; u++) {
            if (doneSampleRate == 0)
              rasterize_point(x, y, color_nearest(tex, u, v, level));
            else
              rasterize_point_1(x, y, color_nearest(tex, u, v, level));
          }
        }
      }
    }
  }*/
}

// resolve samples to render target
void SoftwareRendererImp::resolve() {

  // Task 3:
  // Implement supersampling
  // You may also need to modify other functions marked with "Task 3".
  DEBUG_CODE(printf("resolve\n"));

  if (doneSampleRate == 0) {
    DEBUG_CODE(printf("doneSampleRate = 0\n"));
    DEBUG_CODE(printf("finish resolve\n"));
    return;
  }
  DEBUG_CODE(printf("doneSampleRate = 1\n"));

  size_t small_w = target_w / sample_rate;
  size_t small_h = target_h / sample_rate;
  size_t i, j, m, n, k, x, y, temp;

  for (m = 0, i = 0; i < target_h; i += sample_rate, m++) {
    for (n = 0, j = 0; j < target_w; j += sample_rate, n++) {
      //printf("i=%lu, j=%lu, m=%lu, n=%lu\n", i, j, m, n);
      // Compute the average
      for (k = 0; k < 4; k++) {
        temp = 0;
        for (y = 0; y < sample_rate; y++) {
          for (x = 0; x < sample_rate; x++) {
            temp += render_target[4 * ((i + y) * target_w + (j + x)) + k];
          }
        }
        temp = temp / sample_rate / sample_rate;
        small_target[4 * (m * small_w + n) + k] = (uint8_t) temp;
      }     
    }
  }
  // Change the target back to the smaller one

  free(render_target);
  this->render_target = small_target;
  this->target_w = small_w;
  this->target_h = small_h;

  DEBUG_CODE(printf("finish resolve\n"));
  return;
}

// Modified the declaration in 'software_render.h'
void SoftwareRendererImp::clear_target() {
  // Task 3:
  // Clear the supersample_target
  DEBUG_CODE(printf("clear_target\n"));

  int i = 0;
  int num = 4 * target_w * target_h;
  for (; i < num; i++) {
    render_target[i] = 0;
  }
  return;
}
} // namespace CMU462
