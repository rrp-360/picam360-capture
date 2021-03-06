varying vec2 tcoord;
uniform mat4 unif_matrix;
uniform sampler2D cam0_texture;
uniform sampler2D cam1_texture;
uniform float pixel_size;
uniform float split;
//options start
uniform float sharpness_gain;
uniform float cam0_offset_yaw;
uniform float cam0_offset_x;
uniform float cam0_offset_y;
uniform float cam0_horizon_r;
uniform float cam1_offset_yaw;
uniform float cam1_offset_x;
uniform float cam1_offset_y;
uniform float cam1_horizon_r;
//options end

const float overlap = 0.03;
const float M_PI = 3.1415926535;
const float color_offset = 0.15;
const float color_factor = 1.0 / (1.0 - color_offset);

void main(void) {
	float u = 0.0;
	float v = 0.0;
	vec4 pos = vec4(0.0, 0.0, 0.0, 1.0);
	float pitch_orig = -M_PI / 2.0 + M_PI * tcoord.y;
	float yaw_orig;
	if (split == 0.0) {
		yaw_orig = 2.0 * M_PI * tcoord.x - M_PI;
	} else {
		yaw_orig = 2.0 * M_PI * (tcoord.x / 2.0 + 0.5 * (split - 1.0)) - M_PI;
	}
	pos.x = cos(pitch_orig) * sin(yaw_orig); //yaw starts from z
	pos.y = sin(pitch_orig);
	pos.z = cos(pitch_orig) * cos(yaw_orig); //yaw starts from z
	pos = unif_matrix * pos;
	float pitch = asin(pos.y);
	float yaw = atan(pos.x, pos.z); //yaw starts from z

	vec4 fc0;
	vec4 fc1;
	float r = (M_PI / 2.0 - pitch) / M_PI;
	if (r < 0.5 + overlap) {
		float r2 = r;
		if (r2 >= 0.40) {
			r2 = pow(r2 - 0.4, 1.09) + 0.4;
		}
		float yaw2 = yaw + M_PI + cam0_offset_yaw;
		u = cam0_horizon_r * r2 * cos(yaw2) + 0.5 + cam0_offset_x;
		v = cam0_horizon_r * r2 * sin(yaw2) + 0.5 - cam0_offset_y; //cordinate is different
		if (u <= 0.0 || u > 1.0 || v <= 0.0 || v > 1.0) {
			u = 0.0;
			v = 0.0;
		}
		if (u == 0.0 && v == 0.0) {
			fc0 = vec4(0.0, 0.0, 0.0, 1.0);
		} else {
			vec4 fc;
			if (sharpness_gain == 0.0) {
				fc = texture2D(cam0_texture, vec2(u, v));
			} else {
				//sharpness
				float gain = sharpness_gain + r2;
				fc = texture2D(cam0_texture, vec2(u, v))
						* (1.0 + 4.0 * gain);
				fc -= texture2D(cam0_texture, vec2(u - 1.0 * pixel_size, v))
						* gain;
				fc -= texture2D(cam0_texture, vec2(u, v - 1.0 * pixel_size))
						* gain;
				fc -= texture2D(cam0_texture, vec2(u, v + 1.0 * pixel_size))
						* gain;
				fc -= texture2D(cam0_texture, vec2(u + 1.0 * pixel_size, v))
						* gain;
			}

			fc0 = fc;
		}
	}
	if (r > 0.5 - overlap) {
		float r2 = 1.0 - r;
		if (r2 >= 0.40) {
			r2 = pow(r2 - 0.4, 1.09) + 0.4;
		}
		float yaw2 = -yaw + M_PI + cam1_offset_yaw;
		u = cam1_horizon_r * r2 * cos(yaw2) + 0.5 + cam1_offset_x;
		v = cam1_horizon_r * r2 * sin(yaw2) + 0.5 - cam1_offset_y; //cordinate is different
		if (u <= 0.0 || u > 1.0 || v <= 0.0 || v > 1.0) {
			u = 0.0;
			v = 0.0;
		}
		if (u == 0.0 && v == 0.0) {
			fc1 = vec4(0.0, 0.0, 0.0, 1.0);
		} else {
			vec4 fc;
			if (sharpness_gain == 0.0) {
				fc = texture2D(cam1_texture, vec2(u, v));
			} else {
				//sharpness
				float gain = sharpness_gain + r2;
				fc = texture2D(cam1_texture, vec2(u, v))
						* (1.0 + 4.0 * gain);
				fc -= texture2D(cam1_texture, vec2(u - 1.0 * pixel_size, v))
						* gain;
				fc -= texture2D(cam1_texture, vec2(u, v - 1.0 * pixel_size))
						* gain;
				fc -= texture2D(cam1_texture, vec2(u, v + 1.0 * pixel_size))
						* gain;
				fc -= texture2D(cam1_texture, vec2(u + 1.0 * pixel_size, v))
						* gain;
			}

			fc1 = fc;
		}
	}
	if (r < 0.5 - overlap) {
		gl_FragColor = fc0;
	} else if (r < 0.5 + overlap) {
		gl_FragColor = (fc0 * ((0.5 + overlap) - r)
				+ fc1 * (r - (0.5 - overlap))) / (overlap * 2.0);
	} else {
		gl_FragColor = fc1;
	}
}

