uniform sampler2D sampler;
uniform vec4 modulation;
uniform float saturation;
varying vec2 texcoord0;

void main()
{
	vec4 tex = texture2D(sampler, texcoord0);

    if (saturation != 1.0) {
        vec3 desaturated = tex.rgb * vec3( 0.30, 0.59, 0.11 );
        desaturated = vec3(dot( desaturated, tex.rgb ));
        tex.rgb = tex.rgb * vec3(saturation) + desaturated * vec3(1.0 - saturation);
    }

	float L = (17.8824 * tex.r) + (43.5161 * tex.g) + (4.11935 * tex.b);
	float M = (3.45565 * tex.r) + (27.1554 * tex.g) + (3.86714 * tex.b);
	float S = (0.0299566 * tex.r) + (0.184309 * tex.g) + (1.46709 * tex.b);

	//Protanopia
	float l = 0.0 * L + 2.02344 * M + -2.52581 * S;
	float m = 0.0 * L + 1.0 * M + 0.0 * S;
	float s = 0.0 * L + 0.0 * M + 1.0 * S;

	vec4 error;
	error.r = (0.0809444479 * l) + (-0.130504409 * m) + (0.116721066 * s);
	error.g = (-0.0102485335 * l) + (0.0540193266 * m) + (-0.113614708 * s);
	error.b = (-0.000365296938 * l) + (-0.00412161469 * m) + (0.693511405 * s);
	error.a = 1.0;
	vec4 diff = tex - error;
	vec4 correction;
	correction.r = 0.0;
	correction.g =  (diff.r * 0.7) + (diff.g * 1.0);
	correction.b =  (diff.r * 0.7) + (diff.b * 1.0);
	correction = tex + correction;

	gl_FragColor = correction * modulation;
}