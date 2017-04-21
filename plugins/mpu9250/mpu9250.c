#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#include <limits.h>
#include <math.h>
#include <string.h>

#include "MotionSensor.h"

#include <mat4/multiply.h>
#include <mat4/transpose.h>
#include <mat4/fromQuat.h>
#include <mat4/invert.h>

#include "mpu9250.h"

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

#define PLUGIN_NAME "mpu9250"
#define MPU_NAME "mpu9250"

static PLUGIN_HOST_T *lg_plugin_host = NULL;

static float lg_compass_min[3] = { -317.000000, -416.000000, -208.000000 };
//static float lg_compass_min[3] = { INT_MAX, INT_MAX, INT_MAX };
static float lg_compass_max[3] = { 221.000000, -67.000000, 98.000000 };
//static float lg_compass_max[3] = { -INT_MAX, -INT_MAX, -INT_MAX };
static VECTOR4D_T lg_compass = { };
static VECTOR4D_T lg_quat = { };
static VECTOR4D_T lg_quat_after_offse = { };
static float lg_north = 0;
static int lg_north_count = 0;

static void *threadFunc(void *data) {
	pthread_setname_np(pthread_self(), "MPU9250");

	do {
		ms_update();

		{ //compas : calibration
			float calib[3];
			float bias[3];
			float gain[3];
			for (int i = 0; i < 3; i++) {
				lg_compass_min[i] = MIN(lg_compass_min[i], compass[i]);
				lg_compass_max[i] = MAX(lg_compass_max[i], compass[i]);
				bias[i] = (lg_compass_min[i] + lg_compass_max[i]) / 2;
				gain[i] = (lg_compass_max[i] - lg_compass_min[i]) / 2;
				calib[i] = (compass[i] - bias[i])
						/ (gain[i] == 0 ? 1 : gain[i]);
			}
			float norm = sqrt(
					calib[0] * calib[0] + calib[1] * calib[1]
							+ calib[2] * calib[2]);
			for (int i = 0; i < 3; i++) {
				calib[i] /= norm;
			}
			//convert from mpu coodinate to opengl coodinate
			lg_compass.ary[0] = calib[1];
			lg_compass.ary[1] = -calib[0];
			lg_compass.ary[2] = -calib[2];
			lg_compass.ary[3] = 1.0;
		}
		{ //quat : convert from mpu coodinate to opengl coodinate
			lg_quat.ary[0] = quatanion[1];	//x
			lg_quat.ary[1] = quatanion[3];	//y : swap y and z
			lg_quat.ary[2] = -quatanion[2];	//z : swap y and z
			lg_quat.ary[3] = quatanion[0];	//w
		}
		{ //north
			float north = 0;

			float matrix[16];
			mat4_fromQuat(matrix, lg_quat.ary);
			mat4_invert(matrix, matrix);

			float compass_mat[16] = { };
			memcpy(compass_mat, lg_compass, sizeof(float) * 4);

			mat4_transpose(compass_mat, compass_mat);
			mat4_multiply(compass_mat, compass_mat, matrix);
			mat4_transpose(compass_mat, compass_mat);

			north = -atan2(compass_mat[2], compass_mat[0]) * 180 / M_PI;

			lg_north = (lg_north * lg_north_count + north)
					/ (lg_north_count + 1);
			lg_north_count++;
			if (lg_north_count > 1000) {
				lg_north_count = 1000;
			}
		}
		{ //calib
			QUATERNION_T quat_offset = quaternion_init();
			quat_offset = quaternion_multiply(quat_offset,
					quaternion_get_from_z(lg_offset_roll));
			quat_offset = quaternion_multiply(quat_offset,
					quaternion_get_from_x(lg_offset_pitch));
			quat_offset = quaternion_multiply(quat_offset,
					quaternion_get_from_y(lg_offset_yaw));
			*((QUATERNION_T*) lg_quat_after_offset) = quaternion_multiply(
					quat_offset, lg_quat); // Rv=RvRvo
		}

		usleep(5000);
	} while (1);
	return NULL;
}

static bool is_init = false;

static void init() {
	if (is_init) {
		return;
	} else {
		is_init = true;
	}

	ms_open();

	//do{
	//	ms_update();
	//	printf("%f,%f,%f,%f\n",  (float)_q[0] / (1<<30),  (float)_q[1] / (1<<30),  (float)_q[2] / (1<<30),  (float)_q[3] / (1<<30));
	//	usleep(5000);
	//}while(1);
	pthread_t f1_thread;
	pthread_create(&f1_thread, NULL, threadFunc, NULL);
}

static VECTOR4D_T get_quatanion() {
	return lg_quat_after_offset;
}

static VECTOR4D_T get_compass() {
	return lg_compass;
}

static float get_temperature() {
	return temp;
}

static float get_north() {
	return lg_north;
}

static void release(void *user_data) {
	free(user_data);
}

static void command_handler(void *user_data, const char *_buff) {
	char buff[256];
	strncpy(buff, _buff, sizeof(buff));
	char *cmd;
	cmd = strtok(buff, " \n");
	if (cmd == NULL) {
		//do nothing
	} else if (strncmp(cmd, PLUGIN_NAME ".reset_compass_calib", sizeof(buff))
			== 0) {

		for (int i = 0; i < 3; i++) {
			lg_compass_min[i] = INT_MAX;
			lg_compass_max[i] = -INT_MAX;
		}
	}
}

static void event_handler(void *user_data, uint32_t node_id, uint32_t event_id) {
	switch (node_id) {
	case PICAM360_HOST_NODE_ID:
		break;
	default:
		break;
	}
}

static float lg_offset_pitch = 0;
static float lg_offset_yaw = 0;
static float lg_offset_roll = 0;

static void init_options(void *user_data, json_t *options) {
	lg_offset_pitch = json_number_value(
			json_object_get(options, PLUGIN_NAME ".offset_pitch"));
	lg_offset_yaw = json_number_value(
			json_object_get(options, PLUGIN_NAME ".offset_yaw"));
	lg_offset_roll = json_number_value(
			json_object_get(options, PLUGIN_NAME ".offset_roll"));

	for (int i = 0; i < 3; i++) {
		char buff[256];
		sprintf(buff, PLUGIN_NAME ".compass_min_%d", i);
		lg_compass_min[i] = json_number_value(json_object_get(options, buff));
		sprintf(buff, PLUGIN_NAME ".compass_max_%d", i);
		lg_compass_max[i] = json_number_value(json_object_get(options, buff));
	}

}

static void save_options(void *user_data, json_t *options) {
	json_object_set_new(options, PLUGIN_NAME ".offset_pitch",
			json_real(lg_offset_pitch));
	json_object_set_new(options, PLUGIN_NAME ".offset_yaw",
			json_real(lg_offset_yaw));
	json_object_set_new(options, PLUGIN_NAME ".offset_roll",
			json_real(lg_offset_roll));

	for (int i = 0; i < 3; i++) {
		char buff[256];
		sprintf(buff, PLUGIN_NAME ".compass_min_%d", i);
		json_object_set_new(options, buff, json_real(lg_compass_min[i]));
		sprintf(buff, PLUGIN_NAME ".compass_max_%d", i);
		json_object_set_new(options, buff, json_real(lg_compass_max[i]));
	}
}

static wchar_t *get_info(void *user_data) {
	return NULL;
}

void create_mpu9250(PLUGIN_HOST_T *plugin_host, PLUGIN_T **_plugin) {
	init();
	lg_plugin_host = plugin_host;

	{
		PLUGIN_T *plugin = (PLUGIN_T*) malloc(sizeof(PLUGIN_T));
		strcpy(plugin->name, PLUGIN_NAME);
		plugin->release = release;
		plugin->command_handler = command_handler;
		plugin->event_handler = event_handler;
		plugin->init_options = init_options;
		plugin->save_options = save_options;
		plugin->get_info = get_info;
		plugin->user_data = plugin;

		*_plugin = plugin;
	}
	{
		MPU_T *mpu = (MPU_T*) malloc(sizeof(MPU_T));
		strcpy(mpu->name, MPU_NAME);
		mpu->release = release;
		mpu->get_quatanion = get_quatanion;
		mpu->get_compass = get_compass;
		mpu->get_temperature = get_temperature;
		mpu->get_north = get_north;
		mpu->user_data = mpu;

		lg_plugin_host->add_mpu(mpu);
	}
}
