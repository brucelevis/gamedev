uniform sampler2D sampler;

uniform vec2 lightLocation;
uniform vec3 lightColor;
uniform float amb;
// uniform float lightStrength;
//uniform float screenHeight;
void main() {
	float lightAdd = 1.0f;

	float dist = length(lightLocation - gl_FragCoord.xy);
	float attenuation=1.0/(1.0+0.01*dist+0.00000000001*dist*dist);

	//vec4 color = vec4(1.0f,1.0f,1.0f,1.0f);
	vec4 color = vec4(attenuation, attenuation, attenuation, 1.0f) * vec4(lightColor, 1.0f);
	//color = color + vec4((vec3(lightColor.r + amb, lightColor.g + amb, lightColor.b + amb)*0.25f),1.0f);

	vec2 coords = gl_TexCoord[0].st;
	vec4 tex = texture2D(sampler, coords);

	color += vec4(amb,amb,amb,1.0f+amb);
	gl_FragColor = tex * vec4(color)*tex.a;
}