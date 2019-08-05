#version 120

varying vec4 modelVertex;
uniform vec4 color;
/* uniform vec3 frame; */
/* uniform vec4 bodyColor; */
/* uniform vec4 frameColor; */


// step(x, y) 0 fi x > y

float sigmoid(float x, float scale) {
  return modelVertex.z + scale/(1+exp(-5*(x*2-1)));
}

void plight(vec4 base, out vec4 outcol) {
  float scale = 1-2*modelVertex.z;
  float w = modelVertex.w;
  float width = 1.5*modelVertex.z;
  float y_1 = sigmoid(w*(modelVertex.x-width), scale);
  float y_2 = sigmoid(w*(modelVertex.x+width), scale);
  vec2 v1 = vec2(modelVertex.x+width, (y_2));
  vec2 v2 = vec2(modelVertex.x-width, (y_1));
  vec2 v3 = (modelVertex.xy);
  v1 = v1-v2;
  v2 = v2-v3;
  v1 = v1 * dot(v1, v2) / dot(v1, v1);
  v1 = v2-v1;
  float d = sqrt(dot(v1, v1));
  float onOff = max((width-d) / width, 0);
  
  outcol = mix(vec4(0,0,0,0), base, onOff);
}


void main() {
    vec4 col = vec4(0);
    plight( color, col );
    gl_FragColor = col;
}
