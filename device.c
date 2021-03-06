#include <unistd.h>
#include <pthread.h>
#include <libovr_nsb/OVR.h>

#include "MotionSensor.h"

static Device *dev = NULL;
static float quat[4];

/////////////////////////////////////////////////////////////////////////////////////
// Continuous sample/update thread code
// select() chosen for portability
/////////////////////////////////////////////////////////////////////////////////////
void *_threadFunc(void *data) {
	Device *localDev = (Device *) data;

	while (localDev->runSampleThread) {
		// Try to sample the device for 1ms
		waitSampleDevice(localDev, 1000);

		//printf("\tQ:%+-10g %+-10g %+-10g %+-10g\n", localDev->Q[0], localDev->Q[1], localDev->Q[2], localDev->Q[3] );

		// Send a keepalive - this is too often.  Need to only send on keepalive interval
		sendSensorKeepAlive(localDev);
	}
	return 0;
}

void *threadFunc(void *data) {

	do {
		ms_update();

		usleep(5000);
	} while (1);
}

void _init_device() {

	ms_open();

	//do{
	//	ms_update();
	//	printf("%f,%f,%f,%f\n",  (float)_q[0] / (1<<30),  (float)_q[1] / (1<<30),  (float)_q[2] / (1<<30),  (float)_q[3] / (1<<30));
	//	usleep(5000);
	//}while(1);
	pthread_t f1_thread;
	pthread_create(&f1_thread, NULL, threadFunc, NULL);
}

void init_device() {
	//reset quat x=90 y=0 z=0
	{
		quat[0] = 0.70711;
		quat[1] = 0;
		quat[2] = 0;
		quat[3] = 0.70711;
	}

	dev = openRift(0, 0);

	if (dev == NULL) {
		printf("Could not locate Rift\n");
		printf(
				"Be sure you have read/write permission to the proper /dev/hidrawX device\n");
		return;
	}

	setKeepAliveInterval(dev, 1000);

	pthread_t f1_thread;
	dev->runSampleThread = TRUE;
	pthread_create(&f1_thread, NULL, _threadFunc, dev);
}

float *_get_quatanion() {
	quat[0] = -(float) _q[2] / (1 << 30);
	quat[1] = (float) _q[3] / (1 << 30);
	quat[2] = -(float) _q[1] / (1 << 30);
	quat[3] = (float) _q[0] / (1 << 30);
	return quat;
}

float *get_quatanion() {
	//update
	if (dev != NULL) {
		quat[0] = -dev->Q[0];
		quat[1] = dev->Q[1];
		quat[2] = -dev->Q[2];
		quat[3] = dev->Q[3];
	}
	return quat;
}

void set_quatanion(float *_quat) {
	for (int i = 0; i < 4; i++) {
		quat[i] = _quat[i];
	}
}
