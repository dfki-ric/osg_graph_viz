#version 120

varying vec4 modelVertex;
varying vec3 scale;
uniform vec3 size;
uniform vec3 frame;
uniform vec4 bodyColor;
uniform vec4 frameColor;

float boxDist(vec2 p, vec2 size, float radius) {
	size -= vec2(radius+0.4);
	vec2 d = abs(p) - (size);
  return min(max(d.x, d.y), 0.0) + length(max(d, 0.0)) - radius;
}


float fillMask(float dist) {
	return clamp(-dist, 0.0, 1.0);
}


float borderMask(float dist, float width) {
	float alpha1 = clamp(dist + width, 0.0, 1.0);
	float alpha2 = clamp(dist, 0.0, 1.0);
	return alpha1 - alpha2;
}


// step(x, y) 0 fi x > y

void plight(vec4 base, out vec4 outcol) {
  vec2 w = vec2(frame.x);
  vec2 r2 = vec2(frame.y);
  // get the corners
  /* float c[4]; */
  /* vec2 corner[4]; */
  /* corner[0] = r2; */
  /* corner[1] = vec2(size.x-r2.x, r2.y); */
  /* corner[2] = vec2(size.x-r2.x, size.y-r2.y); */
  /* corner[3] = vec2(r2.x, size.y-r2.y); */

  /* vec2 hv = step(modelVertex.xy, corner[0]); */
  /* c[0] = hv.x*hv.y; */
  /* hv = step(vec2(corner[1].x, modelVertex.y), vec2(modelVertex.x, r2.y)); */
  /* c[1] = hv.x*hv.y; */
  /* hv = step(corner[2], vec2(modelVertex.x, modelVertex.y)); */
  /* c[2] = hv.x*hv.y; */
  /* hv = step(vec2(modelVertex.x, corner[3].y), vec2(r2.x, modelVertex.y)); */
  /* c[3] = hv.x*hv.y; */

  /* // draw background */
  /* hv = step(modelVertex.xy, w) + step(size.xy-w, modelVertex.xy); */
  /* float onOff = min(hv.x+hv.y, 1.0); */
  /* onOff = 1-c[0]-c[1]-c[2]-c[3]-onOff; */
  /* outcol = mix(vec4(0,0,0,0), bodyColor, onOff); */

  /* // draw the frame */
  /* hv = step(modelVertex.xy, w) + step(size.xy-w, modelVertex.xy); */
  /* onOff = min(hv.x+hv.y, 1.0); */
  /* onOff = (1-c[0]-c[1]-c[2]-c[3])*onOff; */
  /* outcol += mix(vec4(0,0,0,0), frameColor, onOff); */

  /* // draw the corners */
  /* for(int i=0; i<4; ++i) { */
  /*   vec2 diff = modelVertex.xy - corner[i]; */
  /*   onOff = step(r2.x-w.x, length(diff)) * step(length(diff), r2.x); */
  /*   onOff *= c[i]*onOff; */
  /*   outcol = mix(outcol, frameColor, onOff); */
  /*   onOff = step(length(diff), r2.x-w.x); */
  /*   onOff *= c[i]*onOff; */
  /*   outcol = mix(outcol, bodyColor, onOff); */
  /* } */

  float d = boxDist(modelVertex.xy-0.5*(size.xy), 0.5*(size.xy-w), frame.y);
  float m = fillMask(d);
  outcol = mix(vec4(0,0,0,0), bodyColor, m);
  m = borderMask(d, w.x);
  outcol = mix(outcol, frameColor, m);
}


void main() {
    vec4 col = vec4(1.0);
    plight( vec4(1.0), col );
    gl_FragColor = col;
}
