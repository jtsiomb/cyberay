uniform sampler2D tex;
uniform vec3 inv_gamma;
uniform float exposure;

void main()
{
	vec4 texel = texture2D(tex, gl_TexCoord[0].st);
	float s = 1.0 / texel.a;

	vec3 color = texel.rgb * vec3(s, s, s);

	color = vec3(1.0) - exp(-color * exposure);
	//color = color / (color + vec3(1.0));

	gl_FragColor = vec4(pow(color, inv_gamma), 1.0);
}
