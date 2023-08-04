#version 330

in vec3 in_Position;

uniform mat4 model;
uniform mat4 vp;

uniform sampler2D dischargeMap;
uniform sampler2D normalMap;
uniform sampler2D albedoMap;
uniform vec2 dimension;

out vec4 ex_Color;
out vec3 ex_Normal;
out vec3 ex_FragPos;

void main(void) {
	//Fragment Position in Model Space
	ex_FragPos = (model * vec4(in_Position, 1.0f)).xyz;
	ex_Normal = texture(normalMap, in_Position.xz/dimension).xyz;
	vec3 ex_Albedo = 1.0f-texture(albedoMap, in_Position.xz/dimension).xyz;

	//Fragment in Screen Space
	gl_Position = vp * vec4(ex_FragPos, 1.0f);

	float discharge = texture(dischargeMap, in_Position.xz/dimension).a;

	vec3 normal = 1.0f-ex_Normal*2.0f;

	float diffuse = dot(normalize(normal), normalize(vec3(1, 1, 1)));
	diffuse = clamp(diffuse, 0.1, 0.9);
	
	float light = 0.7 + 0.5*diffuse;

	ex_Normal = 1.0f-ex_Normal.zxy;
	//ex_Color = vec4(vec3(light), 1.0);
	ex_Color = vec4(light*ex_Albedo, 1.0f);
	//ex_Color = vec4(ex_Normal, 1.0f);
	ex_Color = mix(ex_Color, vec4(1,1,1,1), 0.6*discharge);
}
