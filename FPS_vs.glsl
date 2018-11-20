#version 430            
layout(location = 0) uniform mat4 PVM;
layout(location = 1) uniform mat4 M;
layout(location = 3) uniform int drawType;
layout(location = 4) uniform int pass;
layout(location = 6) uniform float time;

in vec3 pos_attrib;
in vec2 tex_coord_attrib;
in vec3 normal_attrib;

out vec2 tex_coord; 
out vec3 normal;
out vec4 wpos;

void main(void)
{
	tex_coord = tex_coord_attrib;
	normal = normal_attrib;

	if (pass == 1){
		wpos = M * vec4(pos_attrib, 0.0);
		gl_Position = PVM*vec4(pos_attrib, 1.0);
	}
	else if (pass == 2){
		if (drawType == 2) {
			//Idle animation for gun
			vec4 pos = PVM*vec4(pos_attrib, 1.0);
			pos.y += .006 * sin(2.5*time);
			gl_Position = pos;

			wpos = M * pos;
		}
		else {
			wpos = M * vec4(pos_attrib, 0.0);
			gl_Position = PVM*vec4(pos_attrib, 1.0);
		}
	}
}