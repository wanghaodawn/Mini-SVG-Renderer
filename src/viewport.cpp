#include "viewport.h"

#include "CMU462.h"
#include "matrix3x3.h"

#define ENABLE_DEBUG
#ifdef ENABLE_DEBUG
#define DEBUG_CODE(CODEFRAGMENT) CODEFRAGMENT;
#else
#define DEBUG_CODE(CODEFRAGMENT)
#endif

namespace CMU462 {

void ViewportImp::set_viewbox( float x, float y, float span ) {

  // Task 4 (part 2): 
  // Set svg to normalized device coordinate transformation. Your input
  // arguments are defined as SVG canvans coordinates.

  DEBUG_CODE(printf("set_viewbox\n"));

  Matrix3x3 trans, scale;
  trans.zero(0.0);
  scale.zero(0.0);
  
  trans(0, 0) = 1.0;         trans(0, 1) = 0.0;         trans(0, 2) = span-x;
  trans(1, 0) = 0.0;         trans(1, 1) = 1.0;         trans(1, 2) = span-y;
  trans(2, 0) = 0.0;         trans(2, 1) = 0.0;         trans(2, 2) = 1.0;

  scale(0, 0) = 1/(2*span);  scale(0, 1) = 0.0;         scale(0, 2) = 0.0;
  scale(1, 0) = 0.0;         scale(1, 1) = 1/(2*span);  scale(1, 2) = 0.0;
  scale(2, 0) = 0.0;         scale(2, 1) = 0.0;         scale(2, 2) = 1.0;

  trans = scale * trans;
  set_canvas_to_norm(trans);

  this->x = x;
  this->y = y;
  this->span = span;
}

void ViewportImp::update_viewbox( float dx, float dy, float scale ) { 
  DEBUG_CODE(printf("update_viewbox\n"));

  this->x -= dx;
  this->y -= dy;
  this->span *= scale;
  set_viewbox( x, y, span );
}

} // namespace CMU462