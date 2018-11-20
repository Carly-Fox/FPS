#version 430
layout(location = 0) uniform mat4 PVM;
layout(location = 1) uniform mat4 M;
layout(location = 2) uniform mat4 M_Light;
layout(location = 3) uniform int drawType;
layout(location = 4) uniform int pass;
layout(location = 5) uniform sampler2D texture;

out vec4 fragcolor;           
in vec2 tex_coord;
in vec4 wpos;
in vec3 normal;
      
void main(void)
{
   if (pass == 1) {
	   if (drawType == 0) {
	     //Make everything black except targets
		fragcolor = vec4(0.0, 0.0, 0.0, 1.0); 
	   }
	   else {
		//Draw colored targets
		fragcolor = texture2D(texture, tex_coord);
	   }
   }
   else if (pass == 2) {
	   if (drawType == 3){
			//Draw textured crosshair
			fragcolor = texture2D(texture, tex_coord);
	   }
	   else if (drawType == 4) {
  	   		//Draw colored text for HUD
			fragcolor = vec4(1.0, 0.5, 0.0, 1.0);
	    }
	    else {
			//Draw everything else with textures and lighting
			vec4 texColor = texture2D(texture, tex_coord);
			
			//Ambient Light
			vec4 ambient = texColor * vec4(.4, .4, .4, 1.0);

			//Diffuse Light
			vec3 nw = normalize((M * vec4(normal, 0.0)).xyz);
			vec3 light = vec3(0.0, 2.0, 6.0);
			vec3 lw = normalize(((M_Light*vec4(light, 0.0)).xyz) - wpos.xyz);
			vec4 diffuse = texColor * vec4(0.6, 0.6, 0.6, 1.0) * max(0, dot(nw, lw));
			diffuse.w = 1.0;

			fragcolor = ambient + diffuse;
	    }
    }
}