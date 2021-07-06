uniform sampler2D tex;

void main()
{
	vec4 texel = texture2D(tex, gl_TexCoord[0].st);
	float s = 1.0 / texel.a;

	gl_FragColor = vec4(texel.rgb * vec3(s, s, s), 1.0);
}
