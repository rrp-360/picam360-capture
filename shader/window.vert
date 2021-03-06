attribute vec4 vPosition;
varying vec4 position;
uniform float scale;
uniform float aspect_ratio;

void main(void) {
	position = vPosition;
	gl_Position = vec4(-vPosition.x / vPosition.z * scale,
			-vPosition.y / vPosition.z * scale * aspect_ratio, 1.0, 1.0); //negative is for jpeg coordinate
}
